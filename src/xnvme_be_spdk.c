// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_znd.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_SPDK_NAME "spdk"

#ifdef XNVME_BE_SPDK_ENABLED
#include <sys/types.h>
#include <unistd.h>
#include <rte_log.h>
#include <spdk/log.h>
#include <spdk/nvme_spec.h>

#include <xnvme_queue.h>
#include <xnvme_be.h>
#include <xnvme_be_spdk.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>
#include <libxnvme_spec.h>
#include <libxnvme_adm.h>

#define XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS 1
#define XNVME_BE_SPDK_AVLB_TRANSPORTS    3

#define XNVME_BE_SPDK_CREFS_LEN 100
static struct xnvme_be_spdk_ctrlr_ref g_cref[XNVME_BE_SPDK_CREFS_LEN];

/**
 * look for a cref for the given given ident
 *
 * @return On success, a pointer to the cref is returned. When none existsWhen a
 * ref is found, 1 is returned cref is assigned, 0 otherwise.
 */
static struct spdk_nvme_ctrlr *
_cref_lookup(struct xnvme_ident *id)
{
	for (int i = 0; i < XNVME_BE_SPDK_CREFS_LEN; ++i) {
		if (strncmp(g_cref[i].trgt, id->trgt, XNVME_IDENT_TRGT_LEN)) {
			continue;
		}

		if (!g_cref[i].ctrlr) {
			XNVME_DEBUG("FAILED: corrupted ctrlr in cref");
			return NULL;
		}
		if (g_cref[i].refcount < 1) {
			XNVME_DEBUG("FAILED: corrupted refcount");
			return NULL;
		}

		g_cref[i].refcount += 1;

		return g_cref[i].ctrlr;
	}

	return NULL;
}

/**
 * Insert the given controller
 *
 * @return On success, 0 is return. On error, negated ``errno`` is return to
 * indicate the error.
 */
static int
_cref_insert(struct xnvme_ident *ident, struct spdk_nvme_ctrlr *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_SPDK_CREFS_LEN; ++i) {
		if (g_cref[i].refcount) {
			continue;
		}

		g_cref[i].ctrlr = ctrlr;
		g_cref[i].refcount += 1;
		strncpy(g_cref[i].trgt, ident->trgt, XNVME_IDENT_TRGT_LEN);

		return 0;
	}

	return -ENOMEM;
}

int
_cref_deref(struct spdk_nvme_ctrlr *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_SPDK_CREFS_LEN; ++i) {
		if (g_cref[i].ctrlr != ctrlr) {
			continue;
		}

		if (g_cref[i].refcount < 1) {
			XNVME_DEBUG("FAILED: invalid refcount: %d",
				    g_cref[i].refcount);
			return -EINVAL;
		}

		g_cref[i].refcount -= 1;

		if (g_cref[i].refcount == 0) {
			XNVME_DEBUG("INFO: refcount: %d => detaching",
				    g_cref[i].refcount);
			spdk_nvme_detach(ctrlr);
			memset(&g_cref[i], 0, sizeof g_cref[i]);
		}

		return 0;
	}

	XNVME_DEBUG("FAILED: no tracking for %p", (void *)ctrlr);
	return -EINVAL;
}

static int g_xnvme_be_spdk_transport[] = {
#ifdef XNVME_BE_SPDK_TRANSPORT_PCIE_ENABLED
	SPDK_NVME_TRANSPORT_PCIE,
#endif
#ifdef XNVME_BE_SPDK_TRANSPORT_TCP_ENABLED
	SPDK_NVME_TRANSPORT_TCP,
#endif
#ifdef XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED
	SPDK_NVME_TRANSPORT_RDMA,
#endif
#ifdef XNVME_BE_SPDK_TRANSPORT_FC_ENABLED
	SPDK_NVME_TRANSPORT_FC,
#endif
};
static int g_xnvme_be_spdk_ntransport = sizeof g_xnvme_be_spdk_transport / \
					sizeof * g_xnvme_be_spdk_transport;

static int g_xnvme_be_spdk_env_is_initialized = 0;

static void
cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl);

/**
 * Choke stdout, this is used to choke the output from spdk_env_init
 */
static void
xnvme_stdout_choker(int action)
{
	static int XNVME_STDOUT_FD = 0;
	static FILE *XNVME_STDOUT_FNULL = NULL;

	switch (action) {
	case 1:
		XNVME_STDOUT_FD = dup(STDOUT_FILENO);

		fflush(stdout);
		XNVME_STDOUT_FNULL = fopen("/dev/null", "w");
		dup2(fileno(XNVME_STDOUT_FNULL), STDOUT_FILENO);
		break;
	case 0:
		dup2(XNVME_STDOUT_FD, STDOUT_FILENO);
		close(XNVME_STDOUT_FD);
		break;
	}
}

static void
xnvme_stderr_choker(int action)
{
	static int XNVME_STDERR_FD = 0;
	static FILE *XNVME_STDERR_FNULL = NULL;

	switch (action) {
	case 1:
		XNVME_STDERR_FD = dup(STDERR_FILENO);

		fflush(stderr);
		XNVME_STDERR_FNULL = fopen("/dev/null", "w");
		dup2(fileno(XNVME_STDERR_FNULL), STDERR_FILENO);
		break;
	case 0:
		dup2(XNVME_STDERR_FD, STDERR_FILENO);
		close(XNVME_STDERR_FD);
		break;
	}
}

static void
xnvme_spdk_choker_on(void)
{
#ifdef XNVME_DEBUG_ENABLED
	spdk_log_set_print_level(SPDK_LOG_DEBUG);
	rte_log_set_global_level(RTE_LOG_DEBUG);
	return;
#endif

	xnvme_stdout_choker(1);
	xnvme_stderr_choker(1);

	spdk_log_set_print_level(SPDK_LOG_DISABLED);
	rte_log_set_global_level(RTE_LOG_EMERG);
}

static void
xnvme_spdk_choker_off(void)
{
#ifdef XNVME_DEBUG_ENABLED
	return;
#endif

	xnvme_stdout_choker(0);
	xnvme_stderr_choker(0);
}

static inline int
_spdk_env_init(struct spdk_env_opts *opts)
{
	struct spdk_env_opts _spdk_opts = { 0 };
	int err;

	if (g_xnvme_be_spdk_env_is_initialized) {
		XNVME_DEBUG("INFO: already initialized");
		err = 0;
		goto exit;
	}

	xnvme_spdk_choker_on();

	if (!opts) {
		opts = &_spdk_opts;

		spdk_env_opts_init(opts);
		opts->name = "xnvme";
		opts->shm_id = 0;
	}

	err = spdk_env_init(opts);
	if (err < 0) {
		XNVME_DEBUG("FAILED: spdk_env_init(), err: %d", err);
	}

	g_xnvme_be_spdk_env_is_initialized = err == 0;

	xnvme_spdk_choker_off();

exit:
	XNVME_DEBUG("INFO: spdk_env_is_initialized: %d",
		    g_xnvme_be_spdk_env_is_initialized);

	return err;
}

const char *
_spdk_nvmf_adrfam_str(enum spdk_nvmf_adrfam adrfam)
{
	switch (adrfam) {
	case SPDK_NVMF_ADRFAM_IPV4:
		return "IPv4";
	case SPDK_NVMF_ADRFAM_IPV6:
		return "IPv6";
	case SPDK_NVMF_ADRFAM_IB:
	case SPDK_NVMF_ADRFAM_FC:
	case SPDK_NVMF_ADRFAM_INTRA_HOST:
		return "";
	}

	return "";
}

/**
 * Construct a SPDK transport identifier from a xNVMe identifier
 *
 * A trtype must be supplied as xNVMe does not distinguish between TCP, RDMA,
 * etc. it only has the 'fab' prefix for Fabrics transports.
 *
 * TODO:
 * - construct trid for FC
 */
static inline int
_xnvme_be_spdk_ident_to_trid(const struct xnvme_ident *ident, struct spdk_nvme_transport_id *trid,
			     int trtype)
{
	char trid_str[1024] = { 0 }; // TODO: fix this size
	char addr[SPDK_NVMF_TRADDR_MAX_LEN + 1] = { 0 };
	int port = 0;
	int err;

	memset(trid, 0, sizeof(*trid));

	switch (trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		sprintf(trid_str, "trtype:%s traddr:%s",
			spdk_nvme_transport_id_trtype_str(trtype),
			ident->trgt);
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		if (sscanf(ident->trgt, "%[^:]:%d", addr, &port) != 2) {
			XNVME_DEBUG("FAILED: trgt: %s, trtype: %s",
				    ident->trgt,
				    spdk_nvme_transport_id_trtype_str(trtype));
			return -EINVAL;
		}

		snprintf(trid_str, sizeof(trid_str),
			 "trtype:%s adrfam:%s traddr:%s trsvcid:%d",
			 spdk_nvme_transport_id_trtype_str(trtype),
			 strstr(ident->opts, "?adrfam=IPv6") ? "IPv6" : "IPv4",
			 addr, port);
		break;

	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
		XNVME_DEBUG("FAILED: unsupported trtype: %s",
			    spdk_nvme_transport_id_trtype_str(trtype));
		return -ENOSYS;
	}

	err = spdk_nvme_transport_id_parse(trid, trid_str);
	if (err) {
		XNVME_DEBUG("FAILED: parsing trid from trgt: %s, err: %d",
			    ident->trgt, err);
		return err;
	}

	switch (trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		snprintf(trid->subnqn, sizeof trid->subnqn, "%s",
			 SPDK_NVMF_DISCOVERY_NQN);
		break;
	}

	return 0;
}

void *
xnvme_be_spdk_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes;
	void *buf;

	buf = spdk_dma_malloc(nbytes, alignment, phys);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}

	return buf;
}

void *
xnvme_be_spdk_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes;
	void *rebuf;

	rebuf = spdk_dma_realloc(buf, nbytes, alignment, phys);
	if (!rebuf) {
		errno = ENOMEM;
		return NULL;
	}

	return rebuf;
}

void
xnvme_be_spdk_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	spdk_dma_free(buf);
}

int
xnvme_be_spdk_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf,
			  uint64_t *phys)
{
	*phys = spdk_vtophys(buf, NULL);
	if (SPDK_VTOPHYS_ERROR == *phys) {
		return -EIO;
	}
	return 0;
}

static inline int
submit_ioc(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_qpair *qpair, struct xnvme_cmd_ctx *ctx,
	   void *dbuf, uint32_t dbuf_nbytes, void *mbuf, spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
#ifdef XNVME_TRACE_ENABLED
	XNVME_DEBUG("Dumping IO command");
	xnvme_spec_cmd_pr(cmd, XNVME_PR_DEF);
#endif

	return spdk_nvme_ctrlr_cmd_io_raw_with_md(ctrlr, qpair, (struct spdk_nvme_cmd *)&ctx->cmd,
			dbuf, dbuf_nbytes, mbuf, cb_fn, cb_arg);
}

int
_spdk_nvme_transport_id_pr(const struct spdk_nvme_transport_id *trid)
{
	int wrtn = 0;

	wrtn += printf("trid:\n");
	wrtn += printf("  trstring: '%s'\n", trid->trstring);
	wrtn += printf("  trtype: 0x%x\n", trid->trtype);
	wrtn += printf("  adrfam: 0x%x\n", trid->adrfam);
	wrtn += printf("  traddr: '%s'\n", trid->traddr);
	wrtn += printf("  trsvcid: '%s'\n", trid->trsvcid);
	wrtn += printf("  subnqn: '%s'\n", trid->subnqn);
	wrtn += printf("  priority: 0x%x\n", trid->priority);

	return wrtn;
}

int
_spdk_nvme_transport_id_compare_weak(const struct spdk_nvme_transport_id *trid1,
				     const struct spdk_nvme_transport_id *trid2)
{
	struct spdk_nvme_transport_id one = *trid1;
	struct spdk_nvme_transport_id other = *trid2;

	memset(one.subnqn, 0, sizeof one.subnqn);
	memset(other.subnqn, 0, sizeof one.subnqn);

	return spdk_nvme_transport_id_compare(&one, &other);
}

/**
 * Attach to the device matching the device provided by the user, and only when
 * it is not already attached, e.g. attach when:
 *
 * (!attached) && (requested == probed)
 *
 * Also, setup controller-options
 */
static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *probed,
	 struct spdk_nvme_ctrlr_opts *ctrlr_opts)
{
	struct xnvme_dev *dev = cb_ctx;
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_transport_id req = { 0 };

	if (state->attached) {
		XNVME_DEBUG("SKIP: Already attached");
		return false;
	}
	if (_xnvme_be_spdk_ident_to_trid(&dev->ident, &req,
					 probed->trtype)) {
		XNVME_DEBUG("SKIP: !_xnvme_be_spdk_ident_to_trid()");
		return false;
	}
	if (_spdk_nvme_transport_id_compare_weak(probed, &req)) {
		XNVME_DEBUG("SKIP: mismatching trid prbed != ctx");

		_spdk_nvme_transport_id_pr(probed);
		_spdk_nvme_transport_id_pr(&req);

		return false;
	}

	// Setup controller options, general as well as trtype-specific
	ctrlr_opts->command_set = state->css ? state->css : SPDK_NVME_CC_CSS_IOCS;

	switch (req.trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		ctrlr_opts->use_cmb_sqs = state->cmb_sqs ? true : false;
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		ctrlr_opts->header_digest = 1;
		ctrlr_opts->data_digest = 1;
		ctrlr_opts->keep_alive_timeout_ms = 0;
		break;

	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
		XNVME_DEBUG("FAILED: unsupported trtype: %d", probed->trtype);
		return false;
	}

#ifdef XNVME_DEBUG_ENABLED
	_spdk_nvme_transport_id_pr(probed);
#endif

	return true;
}

/**
 * Sets up the state{ns, ctrlr, attached} given via the cb_ctx
 * detached if dev->nsid is not a match
 */
static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *XNVME_UNUSED(trid),
	  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *XNVME_UNUSED(opts))
{
	struct xnvme_dev *dev = cb_ctx;
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct spdk_nvme_ns *ns = NULL;

	XNVME_DEBUG("INFO: nsid: %d", dev->nsid);

	ns = spdk_nvme_ctrlr_get_ns(ctrlr, dev->nsid);
	if (!ns) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_ns(0x%x)", dev->nsid);
		spdk_nvme_detach(ctrlr);
		return;
	}
	if (!spdk_nvme_ns_is_active(ns)) {
		XNVME_DEBUG("FAILED: !spdk_nvme_ns_is_active(nsid:0x%x)",
			    dev->nsid);
		spdk_nvme_detach(ctrlr);
		return;
	}

	state->ns = ns;
	state->ctrlr = ctrlr;
	state->attached = 1;
}

void
xnvme_be_spdk_state_term(struct xnvme_be_spdk_state *state)
{
	int err;

	if (!state) {
		return;
	}
	if (state->qpair) {
		spdk_nvme_ctrlr_free_io_qpair(state->qpair);
		err = pthread_mutex_destroy(&state->qpair_lock);
		if (err) {
			printf("UNHANDLED: pthread_mutex_destroy(): '%s'\n",
			       strerror(err));
		}
	}
	err = _cref_deref(state->ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: _cref_deref():, err: %d", err);
	}
}

void
xnvme_be_spdk_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_spdk_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

struct xnvme_be_spdk_enumerate_ctx {
	struct xnvme_enumeration *list;
	uint8_t cmb_sqs;
	uint8_t css;
};

/**
 * Always attach to supported transports such that `enumerate_attach_cb` can
 * grab namespaces
 *
 * And disable CMB sqs / cqs for now due to shared PMR / CMB
 */
static bool
enumerate_probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		   struct spdk_nvme_ctrlr_opts *ctrlr_opts)
{
	struct xnvme_be_spdk_enumerate_ctx *ectx = cb_ctx;

	// Setup controller options, general as well as trtype-specific
	ctrlr_opts->command_set = ectx->css ? ectx->css : SPDK_NVME_CC_CSS_IOCS;

	switch (trid->trtype) {
	case SPDK_NVME_TRANSPORT_PCIE:
		ctrlr_opts->use_cmb_sqs = ectx->cmb_sqs ? true : false;
		break;

	case SPDK_NVME_TRANSPORT_TCP:
	case SPDK_NVME_TRANSPORT_RDMA:
		ctrlr_opts->header_digest = 1;
		ctrlr_opts->data_digest = 1;
		ctrlr_opts->keep_alive_timeout_ms = 0;
		break;

	case SPDK_NVME_TRANSPORT_FC:
	case SPDK_NVME_TRANSPORT_CUSTOM:
		XNVME_DEBUG("FAILED: unsupported trtype: %d", trid->trtype);
		return false;
	}

	return true;
}

/**
 * TODO:
 * - Add setup of ?addrfam for TCP and RDMA transports
 * - Add '?addrfam' option for TCP  + RDMA transport
 */
static void
enumerate_attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *probed,
		    struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *ctrlr_opts)
{
	struct xnvme_be_spdk_enumerate_ctx *ectx = cb_ctx;
	const int num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
	int trtype = probed->trtype;

	for (int nsid = 1; nsid <= num_ns; ++nsid) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };
		struct spdk_nvme_ns *ns;

		if (!spdk_nvme_ctrlr_get_data(ctrlr)) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data");
			continue;
		}
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (!ns) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_ns(nsid:%d)",
				    nsid);
			continue;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("FAILED: skipping inactive nsid: %d", nsid);
			continue;
		}
		if (!spdk_nvme_ns_get_data(ns)) {
			XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data()");
			continue;
		}

		// Namespace looks good, construct xNVMe identifier, and ..
		switch (trtype) {
		case SPDK_NVME_TRANSPORT_PCIE:
			snprintf(uri, sizeof(uri), "pci:%s", probed->traddr);
			break;

		case SPDK_NVME_TRANSPORT_TCP:
		case SPDK_NVME_TRANSPORT_RDMA:
			snprintf(uri, sizeof(uri), "fab:%s:%s",
				 probed->traddr, probed->trsvcid);
			snprintf(uri + strlen(uri), sizeof(uri) - strlen(uri),
				 "?adrfam=%s",
				 _spdk_nvmf_adrfam_str(probed->adrfam));
			break;

		case SPDK_NVME_TRANSPORT_FC:
		case SPDK_NVME_TRANSPORT_CUSTOM:
			XNVME_DEBUG("SKIP: ENOSYS trtype: %s",
				    spdk_nvme_transport_id_trtype_str(trtype));
			continue;
		}
		snprintf(uri + strlen(uri), sizeof(uri) - strlen(uri),
			 "?nsid=%d", nsid);

		if (ectx->cmb_sqs) {
			snprintf(uri + strlen(uri), sizeof(uri) - strlen(uri),
				 "?cmb_sqs=1");
		}
		if (ectx->css) {
			snprintf(uri + strlen(uri), sizeof(uri) - strlen(uri),
				 "?css=%d", ctrlr_opts->command_set);
		}

		// .. add it to the enumeration.
		if (xnvme_ident_from_uri(uri, &ident)) {
			XNVME_DEBUG("FAILED: ident_from_uri, uri: %s", uri);
			continue;
		}
		if (xnvme_enumeration_append(ectx->list, &ident)) {
			XNVME_DEBUG("FAILED: adding ident");
		}
	}

	if (spdk_nvme_detach(ctrlr)) {
		XNVME_DEBUG("FAILED: spdk_nvme_detach()");
	}
}

/**
 * TODO
 *
 * - Fix size of trid_str
 * - Make use of '?addrfam' for TCP  + RDMA transport when available in sys_uri
 * - Add enumerate FC / construction of identifiers
 * - Consider how to support enumerating custom transports
 */
int
xnvme_be_spdk_enumerate(struct xnvme_enumeration *list, const char *sys_uri, int opts)
{
	struct xnvme_be_spdk_enumerate_ctx ectx = { 0 };
	char addr[SPDK_NVMF_TRADDR_MAX_LEN + 1] = { 0 };
	struct xnvme_ident ident = { 0 };
	int port = 0;

	if (_spdk_env_init(NULL)) {
		XNVME_DEBUG("FAILED: _spdk_env_init()");
		return -ESRCH;
	}
	if (sys_uri) {
		if (xnvme_ident_from_uri(sys_uri, &ident)) {
			XNVME_DEBUG("FAILED: ident_from_uri, uri: %s", sys_uri);
			return -EINVAL;
		}
		if (sscanf(ident.trgt, "%[^:]:%d", addr, &port) != 2) {
			XNVME_DEBUG("FAILED: invalid sys_uri: %s", sys_uri);
			return -EINVAL;
		}
	}

	ectx.list = list;
	ectx.cmb_sqs = false;
	ectx.css = opts & 0x7;

	for (int t = 0; t < g_xnvme_be_spdk_ntransport; ++t) {
		int trtype = g_xnvme_be_spdk_transport[t];
		struct spdk_nvme_transport_id trid = { 0 };
		char trid_str[1024] = { 0 };
		int err;

		XNVME_DEBUG("INFO: trtype: %s, # %d of %d",
			    spdk_nvme_transport_id_trtype_str(trtype),
			    t + 1, g_xnvme_be_spdk_ntransport);

		switch (trtype) {
		case SPDK_NVME_TRANSPORT_PCIE:
			trid.trtype = trtype;
			break;

		case SPDK_NVME_TRANSPORT_TCP:
		case SPDK_NVME_TRANSPORT_RDMA:
			if (!sys_uri) {
				XNVME_DEBUG("SKIP: !sys_uri");
				continue;
			}

			snprintf(trid_str, sizeof trid_str,
				 "trtype:%s adrfam:%s traddr:%s trsvcid:%d",
				 spdk_nvme_transport_id_trtype_str(trtype),
				 strstr(ident.opts, "?adrfam=IPv6") ? "IPv6" : "IPv4",
				 addr, port);

			err = spdk_nvme_transport_id_parse(&trid, trid_str);
			if (err) {
				XNVME_DEBUG("FAILED: id_parse(): %s, err: %d",
					    trid_str, err);
				continue;
			}
			snprintf(trid.subnqn, sizeof trid.subnqn, "%s",
				 SPDK_NVMF_DISCOVERY_NQN);
			break;

		case SPDK_NVME_TRANSPORT_FC:
		case SPDK_NVME_TRANSPORT_CUSTOM:
			XNVME_DEBUG("SKIP: enum. of trtype: %s not implemented",
				    spdk_nvme_transport_id_trtype_str(trtype));
			continue;
		}

		err = spdk_nvme_probe(&trid, &ectx, enumerate_probe_cb,
				      enumerate_attach_cb, NULL);
		if (err) {
			XNVME_DEBUG("FAILED: spdk_nvme_probe(), err: %d", err);
		}
	}

	return 0;
}

/**
 * Determine the following:
 *
 * - Determine device-type	(setup: dev->dtype)
 *
 * - Identify controller	(setup: dev->ctrlr)
 *
 * - Identify namespace		(setup: dev->ns)
 * - Identify namespace-id	(setup: dev->nsid)
 * - Determine Command Set	(setup: dev->csi)
 *
 * TODO: fixup for XNVME_DEV_TYPE_NVME_CONTROLLER
 */
int
xnvme_be_spdk_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	struct xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	const struct spdk_nvme_ctrlr_data *ctrlr_data;
	const struct spdk_nvme_ns_data *ns_data;
	int err;

	dev->dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	dev->csi = XNVME_SPEC_CSI_NVM;		// Assume NVM

	// Allocate buffers for idfy
	idfy_ctrlr = spdk_dma_malloc(sizeof(*idfy_ctrlr), 0x1000, NULL);
	if (!idfy_ctrlr) {
		XNVME_DEBUG("FAILED: spdk_dma_malloc()");
		err = -ENOMEM;
		goto exit;
	}
	idfy_ns = spdk_dma_malloc(sizeof(*idfy_ns), 0x1000, NULL);
	if (!idfy_ns) {
		XNVME_DEBUG("FAILED: spdk_dma_malloc()");
		err = -ENOMEM;
		goto exit;
	}

	// Retrieve and store ctrl and ns
	ctrlr_data = spdk_nvme_ctrlr_get_data(state->ctrlr);
	if (!ctrlr_data) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_data()");
		err = -ENOMEM;
		goto exit;
	}
	ns_data = spdk_nvme_ns_get_data(state->ns);
	if (!ns_data) {
		XNVME_DEBUG("FAILED: spdk_nvme_ns_get_data()");
		err = -ENOMEM;
		goto exit;
	}
	memcpy(&dev->id.ctrlr, ctrlr_data, sizeof(dev->id.ctrlr));
	memcpy(&dev->id.ns, ns_data, sizeof(dev->id.ns));

	//
	// Determine command-set / namespace type by probing
	//

	// Attempt to identify Zoned Namespace
	{
		struct xnvme_spec_znd_idfy_ns *zns = (void *)idfy_ns;

		memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
		ctx = xnvme_cmd_ctx_from_dev(dev);

		err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_ZONED, idfy_ctrlr);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !id-ctrlr-zns");
			goto not_zns;
		}

		memset(idfy_ns, 0, sizeof(*idfy_ns));
		ctx = xnvme_cmd_ctx_from_dev(dev);

		err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_ZONED, idfy_ns);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !id-ns-zns");
			goto not_zns;
		}

		if (!zns->lbafe[0].zsze) {
			goto not_zns;
		}

		memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
		memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));
		dev->csi = XNVME_SPEC_CSI_ZONED;

		XNVME_DEBUG("INFO: looks like csi(ZNS)");
		goto exit;

not_zns:
		XNVME_DEBUG("INFO: failed idfy with csi(ZNS)");
	}

	// Attempt to identify LBLK Namespace
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);

	err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_NVM, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: not csi-specific id-NVM");
		XNVME_DEBUG("INFO: falling back to NVM assumption");
		err = 0;
		goto exit;
	}
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

exit:
	xnvme_buf_free(dev, idfy_ctrlr);
	xnvme_buf_free(dev, idfy_ns);

	return err;
}

/**
 * - Intialize SPDK environment
 * - Parse options from dev->ident
 * - Attach to controller matching dev->ident
 * - create sync-io-qpair
 * - create lock protecting sync-io-qpair
 */
int
xnvme_be_spdk_state_init(struct xnvme_dev *dev)
{
	struct xnvme_be_spdk_state *state = (void *)dev->be.state;
	uint32_t nsid;
	uint32_t cmb_sqs = 0x0;
	uint32_t css = 0x0;
	uint32_t shm_id = 0x0;
	int err;

	struct spdk_env_opts env_opts;

	if (!xnvme_ident_opt_to_val(&dev->ident, "nsid", &nsid)) {
		XNVME_DEBUG("!xnvme_ident_opt_to_val(opt:nsid)");
		return -EINVAL;
	}
	dev->nsid = nsid;

	// Parse options from dev->ident
	// Note, these are user "requests" they might not be met. If they are
	// not met, then state will change to reflect the values in effect.
	if (!xnvme_ident_opt_to_val(&dev->ident, "cmb_sqs", &cmb_sqs)) {
		XNVME_DEBUG("!xnvme_ident_opt_to_val(opt:cmb_sqs)");
	}
	if (!xnvme_ident_opt_to_val(&dev->ident, "css", &css)) {
		XNVME_DEBUG("!xnvme_ident_opt_to_val(opt:css)");
	}
	if (!xnvme_ident_opt_to_val(&dev->ident, "shm_id", &shm_id)) {
                XNVME_DEBUG("!xnvme_ident_opt_to_val(opt:shm_id)");
        }
	state->cmb_sqs = cmb_sqs ? true : false;
	state->css = css & 0x7;		// Assign only the relevant bits

	XNVME_DEBUG("INFO: dev->nsid: %d, state->cmb_sqs: %d, state->css: %d",
		    nsid, state->cmb_sqs, state->css);

	spdk_env_opts_init(&env_opts);
	if (strcmp(dev->ident.schm, "fab") == 0) {
		env_opts.shm_id = -1;
		env_opts.no_pci = true;
	}

	env_opts.shm_id = shm_id;
	err = _spdk_env_init(&env_opts);
	if (err) {
		XNVME_DEBUG("FAILED: _spdk_env_init(), err: %d", err);
		return err;
	}

	state->ctrlr = _cref_lookup(&dev->ident);
	if (state->ctrlr) {
		struct spdk_nvme_ns *ns;

		ns = spdk_nvme_ctrlr_get_ns(state->ctrlr, dev->nsid);
		if (!ns) {
			XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_get_ns(0x%x)", dev->nsid);
			if (_cref_deref(state->ctrlr)) {
				XNVME_DEBUG("FAILED: _cref_deref");
			}
			goto probe;
		}
		if (!spdk_nvme_ns_is_active(ns)) {
			XNVME_DEBUG("FAILED: !spdk_nvme_ns_is_active(nsid:0x%x)",
				    dev->nsid);
			if (_cref_deref(state->ctrlr)) {
				XNVME_DEBUG("FAILED: _cref_deref");
			}
			goto probe;
		}

		state->ns = ns;
		state->attached = 1;
		XNVME_DEBUG("INFO: re-using previously attached controller");
	}
probe:
	// Probe for device matching dev->ident
	for (int i = 0; !state->attached; ++i) {
		if (XNVME_BE_SPDK_MAX_PROBE_ATTEMPTS == i) {
			XNVME_DEBUG("FAILED: max attempts exceeded");
			return -ENXIO;
		}

		for (int t = 0; t < g_xnvme_be_spdk_ntransport; ++t) {
			int trtype = g_xnvme_be_spdk_transport[t];
			struct spdk_nvme_transport_id trid = { 0 };

			XNVME_DEBUG("INFO: trtype: %s, # %d of %d",
				    spdk_nvme_transport_id_trtype_str(trtype),
				    t + 1, g_xnvme_be_spdk_ntransport);

			if (_xnvme_be_spdk_ident_to_trid(&dev->ident, &trid, trtype)) {
				XNVME_DEBUG("SKIP/FAILED: ident_to_trid()");
				continue;
			}

			err = spdk_nvme_probe(&trid, dev, probe_cb, attach_cb, NULL);
			if ((err) || (!state->attached)) {
				XNVME_DEBUG("FAILED: probe a:%d, e:%d, i:%d",
					    state->attached, err, i);
			}
		}
	}

	// Setup IO qpair lock for SYNC commands
	err = pthread_mutex_init(&state->qpair_lock, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: pthread_mutex_init(): '%s'",
			    strerror(err));
		return -err;
	}

	// Setup IO qpair for SYNC commands
	state->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, NULL, 0);
	if (!state->qpair) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_alloc_io_qpair()");
		return -ENOMEM;
	}

	err = _cref_insert(&dev->ident, state->ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: _cref_insert(), err: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_spdk_dev_from_ident(const struct xnvme_ident *ident, struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_spdk;

	err = xnvme_be_spdk_state_init(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_spdk_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_spdk_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_spdk_dev_idfy()");
		xnvme_be_spdk_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_spdk_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	return err;
}

/**
 * Command Queue for asynchronous command submission and completion
 *
 * The context is not thread-safe and the intent is that the user must
 * initialize the opaque #xnvme_async_ctx via xnvme_async_init() pr. thread,
 * which is then delegated to the backend, in this case XNVME_BE_SPDK, which
 * then initialized a struct containing what it needs for a submission /
 * completion path, in the case of XNVME_BE_SPDK, then a qpair is needed and
 * thus allocated and de-allocated by:
 *
 * The XNVME_BE_SPDK specific context is a SPDK qpair and it is carried inside:
 *
 * xnvme_queue->be_rsvd
 */
int
xnvme_be_spdk_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_spdk *queue = (void *)(q);
	struct xnvme_be_spdk_state *state = (void *)queue->base.dev->be.state;
	struct spdk_nvme_io_qpair_opts qopts = { 0 };

	spdk_nvme_ctrlr_get_default_io_qpair_opts(state->ctrlr, &qopts, sizeof(qopts));

	qopts.io_queue_size = XNVME_MAX(queue->base.capacity, qopts.io_queue_size);
	qopts.io_queue_requests = qopts.io_queue_size * 2;

	queue->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, &qopts, sizeof(qopts));
	if (!queue->qpair) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_alloc_io_qpair()");
		return -ENOMEM;
	}

	return 0;
}

int
xnvme_be_spdk_queue_term(struct xnvme_queue *q)
{
	struct xnvme_queue_spdk *queue = (void *)q;
	int err;

	if (!queue->qpair) {
		XNVME_DEBUG("FAILED: !queue->qpair");
		return -EINVAL;
	}

	err = spdk_nvme_ctrlr_free_io_qpair(queue->qpair);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_free_io_qpair(%p), errno: %s",
			    (void *)queue->qpair, strerror(errno));
		return err;
	}

	return err;
}

int
xnvme_be_spdk_queue_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_spdk *queue = (void *)q;
	int err;

	err = spdk_nvme_qpair_process_completions(queue->qpair, max);
	if (err < 0) {
		XNVME_DEBUG("FAILED: spdk_nvme_qpair_process_completion(), err: %d", err);
	}

	return err;
}

int
xnvme_be_spdk_queue_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		int err;

		err = xnvme_be_spdk_queue_poke(queue, 0);
		if (err < 0) {
			XNVME_DEBUG("FAILED: xnvme_be_spdk_queue_poke");
			return err;
		}

		acc += err;
	}

	return acc;
}

static void
cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct xnvme_cmd_ctx *ctx = cb_arg;
	uint8_t *completed = &ctx->be_rsvd[0];

	ctx->cpl = *(const struct xnvme_spec_cpl *)cpl;
	*completed = 1;
}

static void
cmd_async_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct xnvme_cmd_ctx *ctx = cb_arg;

	ctx->async.queue->base.outstanding -= 1;
	ctx->cpl = *(const struct xnvme_spec_cpl *)cpl;
	ctx->async.cb(ctx, ctx->async.cb_arg);
}

// TODO: consider whether 'mbuf_nbytes' is needed here
// TODO: consider whether 'opts' is needed here
int
xnvme_be_spdk_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_spdk *queue = (void *)ctx->async.queue;
	struct xnvme_be_spdk_state *state = (void *)queue->base.dev->be.state;
	int err;

	// TODO: do something with mbuf?

	// Early exit when queue is full
	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	queue->base.outstanding += 1;
	err = submit_ioc(state->ctrlr, queue->qpair, ctx, dbuf, dbuf_nbytes, mbuf, cmd_async_cb,
			 ctx);
	if (err) {
		queue->base.outstanding -= 1;
		XNVME_DEBUG("FAILED: submission failed");
		return err;
	}

	return 0;
}

// TODO: consider whether 'mbuf_nbytes' is needed here
// TODO: consider whether 'opts' is needed here
int
xnvme_be_spdk_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			  size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	struct spdk_nvme_qpair *qpair = state->qpair;
	pthread_mutex_t *qpair_lock = &state->qpair_lock;
	uint8_t *completed = &ctx->be_rsvd[0];

	int err;

	*completed = 0;

	pthread_mutex_lock(qpair_lock);
	err = submit_ioc(state->ctrlr, qpair, ctx, dbuf, dbuf_nbytes, mbuf, cmd_sync_cb, ctx);
	pthread_mutex_unlock(qpair_lock);
	if (err) {
		XNVME_DEBUG("FAILED: submit_ioc(), err: %d", err);
		return err;
	}

	while (!*completed) {
		pthread_mutex_lock(qpair_lock);
		spdk_nvme_qpair_process_completions(qpair, 0);
		pthread_mutex_unlock(qpair_lock);
	}

	*completed = 0;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_ctx_cpl_status()");
		return -EIO;
	}

	return 0;
}

// TODO: consider whether 'opts' should be used for anything here...
static inline int
cmd_admin_submit(struct spdk_nvme_ctrlr *ctrlr, struct xnvme_cmd_ctx *ctx, void *dbuf,
		 uint32_t dbuf_nbytes, void *mbuf, uint32_t mbuf_nbytes, spdk_nvme_cmd_cb cb_fn,
		 void *cb_arg)
{
	ctx->cmd.common.mptr = (uint64_t)mbuf ? (uint64_t)mbuf : ctx->cmd.common.mptr;

	if (mbuf_nbytes && mbuf) {
		ctx->cmd.common.ndm = mbuf_nbytes / 4;
	}

#ifdef XNVME_TRACE_ENABLED
	XNVME_DEBUG("Dumping ADMIN command");
	xnvme_spec_cmd_pr(cmd, 0x0);
#endif

	return spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, (struct spdk_nvme_cmd *)&ctx->cmd,
					     dbuf, dbuf_nbytes, cb_fn, cb_arg);
}

int
xnvme_be_spdk_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t mbuf_nbytes)
{
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	uint8_t *completed = &ctx->be_rsvd[0];

	*completed = 0;

	if (cmd_admin_submit(state->ctrlr, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, cmd_sync_cb,
			     ctx)) {
		XNVME_DEBUG("FAILED: cmd_admin_submit");
		return -EIO;
	}

	while (!*completed) {	// Wait for completion-indicator
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);
	}

	*completed = 0;

	// check for errors
	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_ctx_cpl_status()");
		return -EIO;
	}

	return 0;
}
#endif

static const char *g_schemes[] = {
	"pci",
	"pcie",
	"fab",
};

struct xnvme_be xnvme_be_spdk = {
#ifdef XNVME_BE_SPDK_ENABLED
	.sync = {
		.cmd_io = xnvme_be_spdk_sync_cmd_io,
		.cmd_admin = xnvme_be_spdk_sync_cmd_admin,
		.enabled = 1,
		.id = "nvme_driver"
	},
	.async = {
		.cmd_io = xnvme_be_spdk_async_cmd_io,
		.poke = xnvme_be_spdk_queue_poke,
		.wait = xnvme_be_spdk_queue_wait,
		.init = xnvme_be_spdk_queue_init,
		.term = xnvme_be_spdk_queue_term,
		.enabled = 1,
		.id = "nvme_driver"
	},
	.mem = {
		.buf_alloc = xnvme_be_spdk_buf_alloc,
		.buf_vtophys = xnvme_be_spdk_buf_vtophys,
		.buf_realloc = xnvme_be_spdk_buf_realloc,
		.buf_free = xnvme_be_spdk_buf_free,
	},
	.dev = {
		.enumerate = xnvme_be_spdk_enumerate,
		.dev_from_ident = xnvme_be_spdk_dev_from_ident,
		.dev_close = xnvme_be_spdk_dev_close,
	},
#else
	.async = XNVME_BE_NOSYS_QUEUE,
	.sync = XNVME_BE_NOSYS_SYNC,
	.mem = XNVME_BE_NOSYS_MEM,
	.dev = XNVME_BE_NOSYS_DEV,
#endif
	.attr = {
		.name = XNVME_BE_SPDK_NAME,
#ifdef XNVME_BE_SPDK_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};
