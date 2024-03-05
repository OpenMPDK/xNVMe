// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static long double
bytes2double(const uint8_t bytes[], int n)
{
	long double result = 0;

	for (int i = 0; i < n; i++) {
		result *= 256;
		result += bytes[n - 1 - i];
	}
	return result;
}

int
xnvme_spec_log_health_fpr(FILE *stream, const struct xnvme_spec_log_health_entry *log, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_health_entry:");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  crit_warn: %" PRIu8 "\n", log->crit_warn);
	wrtn += fprintf(stream, "  comp_temp: %d\n", log->comp_temp - 273u);
	wrtn += fprintf(stream, "  avail_spare: %" PRIu8 "\n", log->avail_spare);
	wrtn += fprintf(stream, "  avail_spare_thresh: %" PRIu8 "\n", log->avail_spare_thresh);
	wrtn += fprintf(stream, "  pct_used: %" PRIu8 "\n", log->pct_used);
	wrtn += fprintf(stream, "  eg_crit_warn_sum: %" PRIu8 "\n", log->eg_crit_warn_sum);
	wrtn += fprintf(stream, "  data_units_read: %.0Lf\n",
			bytes2double(log->data_units_read, 16));
	wrtn += fprintf(stream, "  data_units_written: %.0Lf\n",
			bytes2double(log->data_units_written, 16));
	wrtn += fprintf(stream, "  host_read_cmds: %.0Lf\n",
			bytes2double(log->host_read_cmds, 16));
	wrtn += fprintf(stream, "  host_write_cmds: %.0Lf\n",
			bytes2double(log->host_write_cmds, 16));
	wrtn += fprintf(stream, "  ctrlr_busy_time: %.0Lf\n",
			bytes2double(log->ctrlr_busy_time, 16));
	wrtn += fprintf(stream, "  pwr_cycles: %.0Lf\n", bytes2double(log->pwr_cycles, 16));
	wrtn += fprintf(stream, "  pwr_on_hours: %.0Lf\n", bytes2double(log->pwr_on_hours, 16));
	wrtn += fprintf(stream, "  unsafe_shutdowns: %.0Lf\n",
			bytes2double(log->unsafe_shutdowns, 16));
	wrtn += fprintf(stream, "  nr_err_logs: %.0Lf\n", bytes2double(log->nr_err_logs, 16));
	wrtn += fprintf(stream, "  warn_comp_temp_time: %" PRIu32 "\n", log->warn_comp_temp_time);
	wrtn += fprintf(stream, "  crit_comp_temp_time: %" PRIu32 "\n", log->crit_comp_temp_time);
	for (int i = 0; i < 8; ++i) {
		wrtn += fprintf(stream, "  temp_sens%u: %d\n", i + 1,
				log->temp_sens[i] ? log->temp_sens[i] - 273u : 0);
	}
	wrtn += fprintf(stream, "  tmt1tc: %" PRIu32 "\n", log->tmt1tc);
	wrtn += fprintf(stream, "  tmt2tc: %" PRIu32 "\n", log->tmt2tc);
	wrtn += fprintf(stream, "  tttmt1: %" PRIu32 "\n", log->tttmt1);
	wrtn += fprintf(stream, "  tttmt2: %" PRIu32 "\n", log->tttmt2);

	return wrtn;
}

int
xnvme_spec_log_health_pr(const struct xnvme_spec_log_health_entry *log, int opts)
{
	return xnvme_spec_log_health_fpr(stdout, log, opts);
}

static inline int
log_erri_entry_fpr_yaml(FILE *stream, const struct xnvme_spec_log_erri_entry *entry, int indent,
			const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*secnt: %" PRIu64 "%s", indent, "", entry->ecnt, sep);
	wrtn += fprintf(stream, "%*ssqid: %" PRIu16 "%s", indent, "", entry->sqid, sep);
	wrtn += fprintf(stream, "%*scid: %" PRIu16 "%s", indent, "", entry->cid, sep);
	wrtn += fprintf(stream, "%*sstatus: %#" PRIx16 "%s", indent, "", entry->status.val, sep);
	wrtn += fprintf(stream, "%*seloc: %#" PRIx16 "%s", indent, "", entry->eloc, sep);
	wrtn += fprintf(stream, "%*slba: %#" PRIx64 "%s", indent, "", entry->lba, sep);
	wrtn += fprintf(stream, "%*snsid: %" PRIu32 "%s", indent, "", entry->nsid, sep);
	wrtn += fprintf(stream, "%*sven_si: %#" PRIx8 "%s", indent, "", entry->ven_si, sep);
	wrtn += fprintf(stream, "%*strtype: %#" PRIx8 "%s", indent, "", entry->trtype, sep);
	wrtn += fprintf(stream, "%*scmd_si: %#" PRIx64 "%s", indent, "", entry->cmd_si, sep);
	wrtn += fprintf(stream, "%*strtype_si: %#" PRIx16, indent, "", entry->trtype_si);

	return wrtn;
}

int
xnvme_spec_log_erri_entry_fpr(FILE *stream, const struct xnvme_spec_log_erri_entry *entry,
			      int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_erri_entry:");

	if (!entry) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += log_erri_entry_fpr_yaml(stream, entry, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_log_erri_entry_pr(const struct xnvme_spec_log_erri_entry *entry, int opts)
{
	return xnvme_spec_log_erri_entry_fpr(stdout, entry, opts);
}

int
xnvme_spec_log_erri_fpr(FILE *stream, const struct xnvme_spec_log_erri_entry *log, int limit,
			int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_erri:\n");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (int i = 0; i < limit; ++i) {
		// TODO: consider whether it should show or not... perhaps
		// optional?
		// Skip printing invalid entries, that is, ewrtn == 0
		// if (!log[i].ewrtn) {
		//	break;
		//}

		wrtn += fprintf(stream, "  - {");
		wrtn += log_erri_entry_fpr_yaml(stream, &log[i], 0, ", ");
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
xnvme_spec_log_erri_pr(const struct xnvme_spec_log_erri_entry *log, int limit, int opts)
{
	return xnvme_spec_log_erri_fpr(stdout, log, limit, opts);
}

int
xnvme_spec_log_fdp_conf_fpr(FILE *stream, const struct xnvme_spec_log_fdp_conf *log, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_fdp_conf:");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  ncfg: %" PRIu16 "\n", log->ncfg);
	wrtn += fprintf(stream, "  version: %" PRIu8 "\n", log->version);
	wrtn += fprintf(stream, "  size: %" PRIu32 "\n", log->size);

	for (int i = 0; i <= log->ncfg; ++i) {
		wrtn += fprintf(stream, "  config_desc: %d\n", i);
		wrtn += fprintf(stream, "  ds: %" PRIu16 "\n", log->conf_desc[i].ds);
		wrtn += fprintf(stream, "  fdp attributes: {");
		wrtn += fprintf(stream, "    rgif: %" PRIu8, log->conf_desc[i].fdpa.rgif);
		wrtn += fprintf(stream, "    fdpvwc: %" PRIu8, log->conf_desc[i].fdpa.fdpvwc);
		wrtn += fprintf(stream, "    fdpcv: %" PRIu8, log->conf_desc[i].fdpa.fdpcv);
		wrtn += fprintf(stream, "    val: %#" PRIx8, log->conf_desc[i].fdpa.val);
		wrtn += fprintf(stream, "  }\n");
		wrtn += fprintf(stream, "  vss: %" PRIu8 "\n", log->conf_desc[i].vss);
		wrtn += fprintf(stream, "  nrg: %" PRIu32 "\n", log->conf_desc[i].nrg);
		wrtn += fprintf(stream, "  nruh: %" PRIu16 "\n", log->conf_desc[i].nruh);
		wrtn += fprintf(stream, "  maxpids: %" PRIu16 "\n", log->conf_desc[i].maxpids);
		wrtn += fprintf(stream, "  nns: %" PRIu32 "\n", log->conf_desc[i].nns);
		wrtn += fprintf(stream, "  runs: %" PRIu64 "\n", log->conf_desc[i].runs);
		wrtn += fprintf(stream, "  erutl: %" PRIu32 "\n", log->conf_desc[i].erutl);

		for (int j = 0; j < log->conf_desc[i].nruh; j++) {
			wrtn += fprintf(stream, "   - ruht[%d]: %" PRIu8 "\n", j,
					log->conf_desc[i].ruh_desc[j].ruht);
		}
	}

	return wrtn;
}

int
xnvme_spec_log_fdp_conf_pr(const struct xnvme_spec_log_fdp_conf *log, int opts)
{
	return xnvme_spec_log_fdp_conf_fpr(stdout, log, opts);
}

int
xnvme_spec_log_fdp_stats_fpr(FILE *stream, const struct xnvme_spec_log_fdp_stats *log, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_fdp_stats:");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	// TODO: present these better
	wrtn += fprintf(stream, "  hbmw: [%" PRIu64 ", %" PRIu64 "]\n", log->hbmw[0],
			log->hbmw[1]);
	wrtn += fprintf(stream, "  mbmw: [%" PRIu64 ", %" PRIu64 "]\n", log->mbmw[0],
			log->mbmw[1]);
	wrtn += fprintf(stream, "  mbe: [%" PRIu64 ", %" PRIu64 "]\n", log->mbe[0], log->mbe[1]);

	return wrtn;
}

int
xnvme_spec_log_fdp_stats_pr(const struct xnvme_spec_log_fdp_stats *log, int opts)
{
	return xnvme_spec_log_fdp_stats_fpr(stdout, log, opts);
}

static inline int
log_fdp_event_fpr_yaml(FILE *stream, const struct xnvme_spec_fdp_event *event, int indent,
		       const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*stype: %" PRIu8 "%s", indent, "", event->type, sep);
	wrtn += fprintf(stream, "%*sfdpef: %#" PRIx8 "%s", indent, "", event->fdpef.val, sep);
	wrtn += fprintf(stream, "%*spid: %" PRIu16 "%s", indent, "", event->pid, sep);
	wrtn += fprintf(stream, "%*stimestamp: %" PRIu64 "%s", indent, "", event->timestamp, sep);
	wrtn += fprintf(stream, "%*snsid: %" PRIu32 "%s", indent, "", event->nsid, sep);
	// TODO: Event type specific field
	wrtn += fprintf(stream, "%*srgid: %" PRIu16 "%s", indent, "", event->rgid, sep);
	wrtn += fprintf(stream, "%*sruhid: %" PRIu16 "%s", indent, "", event->ruhid, sep);

	return wrtn;
}

int
xnvme_spec_log_fdp_events_fpr(FILE *stream, const struct xnvme_spec_log_fdp_events *log, int limit,
			      int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_fdp_events:\n");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "  nevents: %" PRIu32 "\n", log->nevents);

	for (int i = 0; i < limit; ++i) {
		struct xnvme_spec_fdp_event event;
		event = log->event[i];
		wrtn += fprintf(stream, "  - {");
		wrtn += log_fdp_event_fpr_yaml(stream, &event, 0, ", ");
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
xnvme_spec_log_fdp_events_pr(const struct xnvme_spec_log_fdp_events *log, int limit, int opts)
{
	return xnvme_spec_log_fdp_events_fpr(stdout, log, limit, opts);
}

int
xnvme_spec_log_ruhu_fpr(FILE *stream, const struct xnvme_spec_log_ruhu *log, int limit, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_log_ruhu:\n");

	if (!log) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "  nruh: %" PRIu16 "\n", log->nruh);

	for (int i = 0; i < limit; ++i) {
		wrtn += fprintf(stream, "  - ruhu_desc[%d]:  %#" PRIx8 "\n", i,
				log->ruhu_desc[i].ruha);
	}

	return wrtn;
}

int
xnvme_spec_log_ruhu_pr(const struct xnvme_spec_log_ruhu *log, int limit, int opts)
{
	return xnvme_spec_log_ruhu_fpr(stdout, log, limit, opts);
}

int
xnvme_spec_ruhs_fpr(FILE *stream, const struct xnvme_spec_ruhs *ruhs, int limit, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_ruhs:\n");

	if (!ruhs) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "  nruhsd: %" PRIu16 "\n", ruhs->nruhsd);

	for (int i = 0; i < limit; ++i) {
		wrtn += fprintf(stream, "  - ruhs_desc[%d] : {", i);
		wrtn += fprintf(stream, " pi: %" PRIu16, ruhs->desc[i].pi);
		wrtn += fprintf(stream, " ruhi: %" PRIu16, ruhs->desc[i].ruhi);
		wrtn += fprintf(stream, " earutr: %" PRIu32, ruhs->desc[i].earutr);
		wrtn += fprintf(stream, " ruamw: %" PRIu64 "", ruhs->desc[i].ruamw);
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
xnvme_spec_ruhs_pr(const struct xnvme_spec_ruhs *ruhs, int limit, int opts)
{
	return xnvme_spec_ruhs_fpr(stdout, ruhs, limit, opts);
}

int
xnvme_spec_idfy_ctrlr_fpr(FILE *stream, const struct xnvme_spec_idfy_ctrlr *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_idfy_ctrlr:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  vid: %#" PRIx16 "\n", idfy->vid);
	wrtn += fprintf(stream, "  ssvid: %#" PRIx16 "\n", idfy->ssvid);
	wrtn += fprintf(stream, "  sn: '%-.*s'\n",
			XNVME_MIN(XNVME_SPEC_CTRLR_SN_LEN,
				  strnlen((const char *)idfy->sn, XNVME_SPEC_CTRLR_SN_LEN)),
			idfy->sn);
	wrtn += fprintf(stream, "  mn: '%-.*s'\n",
			XNVME_MIN(XNVME_SPEC_CTRLR_MN_LEN,
				  strnlen((const char *)idfy->mn, XNVME_SPEC_CTRLR_MN_LEN)),
			idfy->mn);
	wrtn += fprintf(stream, "  fr: '%-.*s'\n",
			XNVME_MIN(XNVME_SPEC_CTRLR_FR_LEN,
				  strnlen((const char *)idfy->fr, XNVME_SPEC_CTRLR_FR_LEN)),
			idfy->fr);

	wrtn += fprintf(stream, "  rab: %" PRIu8 "\n", idfy->rab);
	wrtn += fprintf(stream, "  ieee: %02" PRIx8 "%02" PRIx8 "%02" PRIx8 "\n", idfy->ieee[2],
			idfy->ieee[1], idfy->ieee[0]);
	wrtn += fprintf(stream, "  cmic: %#" PRIx8 "\n", idfy->cmic.val);
	wrtn += fprintf(stream, "  mdts: %" PRIu8 "\n", idfy->mdts);
	wrtn += fprintf(stream, "  cntlid: %#" PRIx16 "\n", idfy->cntlid);
	wrtn += fprintf(stream, "  ver: %#" PRIx32 "\n", idfy->ver.val);
	wrtn += fprintf(stream, "  rtd3r: %#" PRIx32 "\n", idfy->rtd3r);
	wrtn += fprintf(stream, "  rtd3e: %#" PRIx32 "\n", idfy->rtd3e);
	wrtn += fprintf(stream, "  oaes: %#" PRIx32 "\n", idfy->oaes.val);
	wrtn += fprintf(stream, "  ctratt: %#" PRIx32 "\n", idfy->ctratt.val);
	wrtn += fprintf(stream, "  rrls: %#" PRIx16 "\n", idfy->rrls);
	wrtn += fprintf(stream, "  cntrltype: %#" PRIx8 "\n", idfy->cntrltype);
	// TODO: fguid
	wrtn += fprintf(stream, "  crdt1: %#" PRIx16 "\n", idfy->crdt1);
	wrtn += fprintf(stream, "  crdt2: %#" PRIx16 "\n", idfy->crdt2);
	wrtn += fprintf(stream, "  crdt3: %#" PRIx16 "\n", idfy->crdt3);
	wrtn += fprintf(stream, "  nvmsr: %#" PRIx8 "\n", idfy->nvmsr.val);
	wrtn += fprintf(stream, "  vwci: %#" PRIx8 "\n", idfy->vwci.val);
	wrtn += fprintf(stream, "  mec: %#" PRIx8 "\n", idfy->mec.val);
	wrtn += fprintf(stream, "  oacs: %#" PRIx16 "\n", idfy->oacs.val);
	wrtn += fprintf(stream, "  acl: %" PRIu8 "\n", idfy->acl);
	wrtn += fprintf(stream, "  aerl: %" PRIu8 "\n", idfy->aerl);
	wrtn += fprintf(stream, "  frmw: %#" PRIx8 "\n", idfy->frmw.val);

	wrtn += fprintf(stream, "  lpa: {");
	wrtn += fprintf(stream, " ns_smart: %" PRIu8 ",", idfy->lpa.ns_smart);
	wrtn += fprintf(stream, " celp: %" PRIu8 ",", idfy->lpa.celp);
	wrtn += fprintf(stream, " edlp: %" PRIu8 ",", idfy->lpa.edlp);
	wrtn += fprintf(stream, " telemetry: %" PRIu8 ",", idfy->lpa.telemetry);
	wrtn += fprintf(stream, " pel: %" PRIu8 ",", idfy->lpa.pel);
	wrtn += fprintf(stream, " mel: %" PRIu8 ",", idfy->lpa.mel);
	wrtn += fprintf(stream, " tel_da4: %" PRIu8 ",", idfy->lpa.tel_da4);
	wrtn += fprintf(stream, " val: %#" PRIx8, idfy->lpa.val);
	wrtn += fprintf(stream, "}\n");

	wrtn += fprintf(stream, "  elpe: %" PRIu8 "\n", idfy->elpe);
	wrtn += fprintf(stream, "  npss: %" PRIu8 "\n", idfy->npss);
	wrtn += fprintf(stream, "  avscc: %#" PRIx8 "\n", idfy->avscc.val);
	wrtn += fprintf(stream, "  apsta: %#" PRIx8 "\n", idfy->apsta.val);
	wrtn += fprintf(stream, "  wctemp: %" PRIu16 "\n", idfy->wctemp);
	wrtn += fprintf(stream, "  cctemp: %" PRIu16 "\n", idfy->cctemp);
	wrtn += fprintf(stream, "  mtfa: %" PRIu16 "\n", idfy->mtfa);
	wrtn += fprintf(stream, "  hmpre: %" PRIu32 "\n", idfy->hmpre);
	wrtn += fprintf(stream, "  hmmin: %" PRIu32 "\n", idfy->hmmin);
	// TODO: present these better
	wrtn += fprintf(stream, "  tnvmcap: [%" PRIu64 ", %" PRIu64 "]\n", idfy->tnvmcap[0],
			idfy->tnvmcap[1]);
	wrtn += fprintf(stream, "  unvmcap: [%" PRIu64 ", %" PRIu64 "]\n", idfy->unvmcap[0],
			idfy->unvmcap[1]);
	wrtn += fprintf(stream, "  rpmbs: %#" PRIx32 "\n", idfy->rpmbs.val);
	wrtn += fprintf(stream, "  edstt: %" PRIu16 "\n", idfy->edstt);
	wrtn += fprintf(stream, "  dsto: %" PRIu8 "\n", idfy->dsto.val);
	wrtn += fprintf(stream, "  fwug: %" PRIu8 "\n", idfy->fwug);
	wrtn += fprintf(stream, "  kas: %" PRIu16 "\n", idfy->kas);
	wrtn += fprintf(stream, "  hctma: %#" PRIx16 "\n", idfy->hctma.val);
	wrtn += fprintf(stream, "  mntmt: %" PRIu16 "\n", idfy->mntmt);
	wrtn += fprintf(stream, "  mxtmt: %" PRIu16 "\n", idfy->mxtmt);
	wrtn += fprintf(stream, "  sanicap: %#" PRIx32 "\n", idfy->sanicap.val);
	wrtn += fprintf(stream, "  hmminds: %" PRIu32 "\n", idfy->hmminds);
	wrtn += fprintf(stream, "  hmmaxd: %" PRIu16 "\n", idfy->hmmaxd);
	wrtn += fprintf(stream, "  nsetidmax: %" PRIu16 "\n", idfy->nsetidmax);
	wrtn += fprintf(stream, "  endgidmax: %" PRIu16 "\n", idfy->endgidmax);
	wrtn += fprintf(stream, "  anatt: %" PRIu8 "\n", idfy->anatt);
	wrtn += fprintf(stream, "  anacap: %#" PRIx8 "\n", idfy->anacap.val);
	wrtn += fprintf(stream, "  anagrpmax: %" PRIu32 "\n", idfy->anagrpmax);
	wrtn += fprintf(stream, "  nanagrpid: %" PRIu32 "\n", idfy->nanagrpid);
	wrtn += fprintf(stream, "  pels: %" PRIu32 "\n", idfy->pels);
	wrtn += fprintf(stream, "  domain_identifier: %" PRIu16 "\n", idfy->domain_identifier);
	// TODO: present these better
	wrtn += fprintf(stream, "  megcap: [%" PRIu64 ", %" PRIu64 "]\n", idfy->megcap[0],
			idfy->megcap[1]);
	wrtn += fprintf(stream, "  sqes: %#" PRIx8 "\n", idfy->sqes.val);
	wrtn += fprintf(stream, "  cqes: %#" PRIx8 "\n", idfy->cqes.val);
	wrtn += fprintf(stream, "  maxcmd: %" PRIu16 "\n", idfy->maxcmd);
	wrtn += fprintf(stream, "  nn: %" PRIu32 "\n", idfy->nn);
	wrtn += fprintf(stream, "  oncs:\n");
	wrtn += fprintf(stream, "    compare: %" PRIu16 "\n", idfy->oncs.compare);
	wrtn += fprintf(stream, "    write_unc: %" PRIu16 "\n", idfy->oncs.write_unc);
	wrtn += fprintf(stream, "    dsm: %" PRIu16 "\n", idfy->oncs.dsm);
	wrtn += fprintf(stream, "    write_zeroes: %" PRIu16 "\n", idfy->oncs.write_zeroes);
	wrtn += fprintf(stream, "    set_features_save: %" PRIu16 "\n",
			idfy->oncs.set_features_save);
	wrtn += fprintf(stream, "    reservations: %" PRIu16 "\n", idfy->oncs.reservations);
	wrtn += fprintf(stream, "    timestamp: %" PRIu16 "\n", idfy->oncs.timestamp);
	wrtn += fprintf(stream, "    verify: %" PRIu16 "\n", idfy->oncs.verify);
	wrtn += fprintf(stream, "    copy: %" PRIu16 "\n", idfy->oncs.copy);
	wrtn += fprintf(stream, "  fuses: %#" PRIx16 "\n", idfy->fuses);
	wrtn += fprintf(stream, "  fna: %#" PRIx8 "\n", idfy->fna.val);
	wrtn += fprintf(stream, "  vwc: %#" PRIx8 "\n", idfy->vwc.val);
	wrtn += fprintf(stream, "  awun: %" PRIu16 "\n", idfy->awun);
	wrtn += fprintf(stream, "  awupf: %" PRIu16 "\n", idfy->awupf);
	wrtn += fprintf(stream, "  nvscc: %" PRIu8 "\n", idfy->nvscc);
	wrtn += fprintf(stream, "  acwu: %d" PRIu16 "\n", idfy->acwu);
	wrtn += fprintf(stream, "  cdfs: %#" PRIx16 "\n", idfy->cdfs.val);
	wrtn += fprintf(stream, "  cdfs:\n");
	wrtn += fprintf(stream, "    format0: %" PRIu16 "\n", idfy->cdfs.format0);
	wrtn += fprintf(stream, "    format1: %" PRIu16 "\n", idfy->cdfs.format1);
	wrtn += fprintf(stream, "  sgls: %#" PRIx32 "\n", idfy->sgls.val);
	wrtn += fprintf(stream, "  mnan: %" PRIu32 "\n", idfy->mnan);
	// TODO: present these better
	wrtn += fprintf(stream, "  maxdna: [%" PRIu64 ", %" PRIu64 "]\n", idfy->maxdna[0],
			idfy->maxdna[1]);
	wrtn += fprintf(stream, "  maxcna: %" PRIu32 "\n", idfy->maxcna);
	wrtn += fprintf(stream, "  subnqn: '%-.*s'\n", (int)sizeof(idfy->subnqn), idfy->subnqn);

	// TODO: add print for remaining fields
	return wrtn;
}

int
xnvme_spec_idfy_ctrlr_pr(const struct xnvme_spec_idfy_ctrlr *idfy, int opts)
{
	return xnvme_spec_idfy_ctrlr_fpr(stdout, idfy, opts);
}

int
xnvme_spec_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_idfy_ns *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_idfy_ns:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nsze: %" PRIu64 "\n", idfy->nsze);
	wrtn += fprintf(stream, "  ncap: %" PRIu64 "\n", idfy->ncap);
	wrtn += fprintf(stream, "  nuse: %" PRIu64 "\n", idfy->nuse);
	wrtn += fprintf(stream, "  nlbaf: %" PRIu8 "\n", idfy->nlbaf);

	wrtn += fprintf(stream, "  nsfeat:\n");
	wrtn += fprintf(stream, "    thin_prov: %" PRIu8 "\n", idfy->nsfeat.thin_prov);
	wrtn += fprintf(stream, "    ns_atomic_write_unit: %" PRIu8 "\n",
			idfy->nsfeat.ns_atomic_write_unit);
	wrtn += fprintf(stream, "    dealloc_or_unwritten_error: %" PRIu8 "\n",
			idfy->nsfeat.dealloc_or_unwritten_error);
	wrtn += fprintf(stream, "    guid_never_reused: %" PRIu8 "\n",
			idfy->nsfeat.guid_never_reused);
	wrtn += fprintf(stream, "    optimal_performance: %" PRIu8 "\n",
			idfy->nsfeat.optimal_performance);
	wrtn += fprintf(stream, "    reserved1: %" PRIu8 "\n", idfy->nsfeat.reserved1);

	wrtn += fprintf(stream, "  flbas:\n");
	wrtn += fprintf(stream, "    format_lsb: %" PRIu8 "\n", idfy->flbas.format);
	wrtn += fprintf(stream, "    extended: %" PRIu8 "\n", idfy->flbas.extended);
	wrtn += fprintf(stream, "    format_msb: %" PRIu8 "\n", idfy->flbas.format_msb);
	wrtn += fprintf(stream, "    reserved2: %" PRIu8 "\n", idfy->flbas.reserved2);

	wrtn += fprintf(stream, "  mc:\n");
	wrtn += fprintf(stream, "    extended: %" PRIu8 "\n", idfy->mc.extended);
	wrtn += fprintf(stream, "    pointer: %" PRIu8 "\n", idfy->mc.pointer);
	wrtn += fprintf(stream, "    reserved3: %" PRIu8 "\n", idfy->mc.reserved3);

	wrtn += fprintf(stream, "  dpc: %#" PRIx8 "\n", idfy->dpc.val);
	wrtn += fprintf(stream, "  dps: %#" PRIx8 "\n", idfy->dps.val);
	wrtn += fprintf(stream, "  nsrescap: %#" PRIx8 "\n", idfy->nsrescap.val);

	wrtn += fprintf(stream, "  fpi: %#" PRIx8 "\n", idfy->fpi.val);
	wrtn += fprintf(stream, "  dlfeat: %#" PRIx8 "\n", idfy->dlfeat.val);

	wrtn += fprintf(stream, "  nawun: %#" PRIx16 "\n", idfy->nawun);
	wrtn += fprintf(stream, "  nawupf: %#" PRIx16 "\n", idfy->nawupf);
	wrtn += fprintf(stream, "  nacwu: %#" PRIx16 "\n", idfy->nacwu);
	wrtn += fprintf(stream, "  nabsn: %#" PRIx16 "\n", idfy->nabsn);
	wrtn += fprintf(stream, "  nabspf: %#" PRIx16 "\n", idfy->nabspf);
	wrtn += fprintf(stream, "  noiob: %#" PRIx16 "\n", idfy->noiob);
	wrtn += fprintf(stream, "  nvmcap:\n");
	wrtn += fprintf(stream, "    - %" PRIu64 "\n", idfy->nvmcap[0]);
	wrtn += fprintf(stream, "    - %" PRIu64 "\n", idfy->nvmcap[1]);
	wrtn += fprintf(stream, "  npwg: %#" PRIx16 "\n", idfy->npwg);
	wrtn += fprintf(stream, "  npwa: %#" PRIx16 "\n", idfy->npwa);
	wrtn += fprintf(stream, "  npdg: %#" PRIx16 "\n", idfy->npdg);
	wrtn += fprintf(stream, "  npda: %#" PRIx16 "\n", idfy->npda);
	wrtn += fprintf(stream, "  nows: %#" PRIx16 "\n", idfy->nows);
	wrtn += fprintf(stream, "  mssrl: %" PRIu16 "\n", idfy->mssrl);
	wrtn += fprintf(stream, "  mcl: %" PRIu32 "\n", idfy->mcl);
	wrtn += fprintf(stream, "  msrc: %" PRIu8 "\n", idfy->msrc);
	wrtn += fprintf(stream, "  anagrpid: %#" PRIx32 "\n", idfy->anagrpid);
	wrtn += fprintf(stream, "  nsattr: %#" PRIx8 "\n", idfy->nsattr);
	wrtn += fprintf(stream, "  nvmsetid: %#" PRIx16 "\n", idfy->nvmsetid);
	wrtn += fprintf(stream, "  endgid: %#" PRIx16 "\n", idfy->endgid);
	wrtn += fprintf(stream, "  nguid: [");
	for (int i = 0; i < 16; ++i) {
		if (i) {
			wrtn += fprintf(stream, ", ");
		}
		wrtn += fprintf(stream, "%#" PRIx8, idfy->nguid[i]);
	}
	wrtn += fprintf(stream, "]\n");

	wrtn += fprintf(stream, "  eui64: %" PRIu64 "\n", idfy->eui64);
	wrtn += fprintf(stream, "  # ms: meta-data-size-in-nbytes\n");
	wrtn += fprintf(stream, "  # ds: data-size in 2^ds bytes\n");
	wrtn += fprintf(stream, "  # rp: relative performance 00b, 1b 10b, 11b\n");

	wrtn += fprintf(stream, "  lbaf:\n");
	for (int i = 0; i <= idfy->nlbaf; ++i) {
		wrtn += fprintf(stream,
				"    - {ms: %" PRIu16 ", ds: %" PRIu8 ", rp: %" PRIu8 "}\n",
				idfy->lbaf[i].ms, idfy->lbaf[i].ds, idfy->lbaf[i].rp);
	}

	return wrtn;
}

int
xnvme_spec_idfy_ns_pr(const struct xnvme_spec_idfy_ns *idfy, int opts)
{
	return xnvme_spec_idfy_ns_fpr(stdout, idfy, opts);
}

int
xnvme_spec_idfy_cs_fpr(FILE *stream, const struct xnvme_spec_idfy_cs *idfy, int opts)
{
	int count = 0;
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_idfy_cs:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (int i = 0; i < XNVME_SPEC_IDFY_CS_IOCSC_LEN; ++i) {
		const struct xnvme_spec_cs_vector *iocscv = &idfy->iocsc[i];

		if (!iocscv->val) {
			continue;
		}

		wrtn += fprintf(stream, "\n");
		wrtn += fprintf(stream, "  - { ");
		wrtn += fprintf(stream, "iocsci: %d, ", i);
		wrtn += fprintf(stream, "val: 0x%" PRIx64 ", ", iocscv->val);
		wrtn += fprintf(stream, "nvm: %d, ", iocscv->nvm);
		wrtn += fprintf(stream, "kv: %d, ", iocscv->kv);
		wrtn += fprintf(stream, "zns: %d", iocscv->zns);
		wrtn += fprintf(stream, " }");

		count++;
	}

	wrtn += fprintf(stream, count ? "\n" : " ~\n");

	return wrtn;
}

int
xnvme_spec_idfy_cs_pr(const struct xnvme_spec_idfy_cs *idfy, int opts)
{
	return xnvme_spec_idfy_cs_fpr(stdout, idfy, opts);
}

int
xnvme_spec_cmd_fpr(FILE *stream, struct xnvme_spec_cmd *cmd, int opts)
{
	int wrtn = 0;
	uint32_t *cdw = (void *)cmd;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_cmd:");

	if (!cmd) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	switch (opts) {
	case 0x0:
		for (int i = 0; i < 16; ++i) {
			wrtn += fprintf(stream, "  - '" XNVME_I32_FMT "'\n",
					XNVME_I32_TO_STR(cdw[i]));
		}
		break;

	case 0x1:
		for (int i = 0; i < 16; ++i) {
			wrtn += fprintf(stream, "  - %#04" PRIx32 "\n", cdw[i]);
		}
		break;

	case 0x2:
		for (int i = 0; i < 16; ++i) {
			wrtn += fprintf(stream, "  cdw%01d: %#04" PRIx32 "\n", i, cdw[i]);
		}
		break;

	case 0x3:
		wrtn += fprintf(stream, "  opcode: %#02" PRIx16 "\n", cmd->common.opcode);
		wrtn += fprintf(stream, "  fuse: %#" PRIx16 "\n", cmd->common.fuse);
		wrtn += fprintf(stream, "  rsvd: %#" PRIx16 "\n", cmd->common.rsvd);
		wrtn += fprintf(stream, "  psdt: %#" PRIx16 "\n", cmd->common.psdt);
		wrtn += fprintf(stream, "  cid: %#04" PRIx16 "\n", cmd->common.cid);
		wrtn += fprintf(stream, "  nsid: %#04" PRIx32 "\n", cmd->common.nsid);
		wrtn += fprintf(stream, "  cdw02: %#04" PRIx32 "\n", cdw[2]);
		wrtn += fprintf(stream, "  cdw03: %#04" PRIx32 "\n", cdw[3]);
		wrtn += fprintf(stream, "  mptr: %#08" PRIx64 "\n", cmd->common.mptr);
		wrtn += fprintf(stream, "  prp1: %#08" PRIx64 "\n", cmd->common.dptr.prp.prp1);
		wrtn += fprintf(stream, "  prp2: %#08" PRIx64 "\n", cmd->common.dptr.prp.prp2);
		wrtn += fprintf(stream, "  cdw10: %#04" PRIx32 "\n", cdw[10]);
		wrtn += fprintf(stream, "  cdw11: %#04" PRIx32 "\n", cdw[11]);
		wrtn += fprintf(stream, "  cdw12: %#04" PRIx32 "\n", cdw[12]);
		wrtn += fprintf(stream, "  cdw13: %#04" PRIx32 "\n", cdw[13]);
		wrtn += fprintf(stream, "  cdw14: %#04" PRIx32 "\n", cdw[14]);
		wrtn += fprintf(stream, "  cdw15: %#04" PRIx32 "\n", cdw[15]);
		break;
	}

	return wrtn;
}

int
xnvme_spec_cmd_pr(struct xnvme_spec_cmd *cmd, int opts)
{
	return xnvme_spec_cmd_fpr(stdout, cmd, opts);
}

int
xnvme_spec_feat_fpr(FILE *stream, uint8_t fid, struct xnvme_spec_feat feat, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	switch (fid) {
	case XNVME_SPEC_FEAT_ERROR_RECOVERY:
		wrtn += fprintf(stream, "feat: {dulbe: %" PRIx32 ", tler: %" PRIx32 ")\n",
				feat.error_recovery.dulbe, feat.error_recovery.tler);
		return wrtn;

	case XNVME_SPEC_FEAT_TEMP_THRESHOLD:
		wrtn += fprintf(stream,
				"feat: {"
				"tmpth: %" PRIu32 ", tmpsel: 0x%" PRIx32 ", thsel: 0x%" PRIx32
				"}\n",
				feat.temp_threshold.tmpth, feat.temp_threshold.tmpsel,
				feat.temp_threshold.thsel);
		return wrtn;

	case XNVME_SPEC_FEAT_NQUEUES:
		wrtn += fprintf(stream, "feat: { nsqa: %" PRIu32 ", ncqa: %" PRIu32 " }\n",
				feat.nqueues.nsqa, feat.nqueues.ncqa);
		return wrtn;

	case XNVME_SPEC_FEAT_FDP_MODE:
		wrtn += fprintf(stream, "feat: { fdpe: %" PRIu32 ", fdpci: %" PRIu32 " }\n",
				feat.fdp_mode.fdpe, feat.fdp_mode.fdpci);
		return wrtn;

	case XNVME_SPEC_FEAT_FDP_EVENTS:
		wrtn += fprintf(stream, "nevents: %" PRIu32 " }\n", feat.val);
		return wrtn;

	case XNVME_SPEC_FEAT_ARBITRATION:
	case XNVME_SPEC_FEAT_PWR_MGMT:
	case XNVME_SPEC_FEAT_LBA_RANGETYPE:
	default:
		wrtn += fprintf(stream, "# ENOSYS: printer not implemented for fid: %" PRIx8, fid);
	}

	return wrtn;
}

int
xnvme_spec_feat_pr(uint8_t fid, struct xnvme_spec_feat feat, int opts)
{
	return xnvme_spec_feat_fpr(stdout, fid, feat, opts);
}

int
xnvme_spec_feat_fdp_events_fpr(FILE *stream, void *buf, struct xnvme_spec_feat feat, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	for (uint32_t i = 0; i < feat.val; i++) {
		struct xnvme_spec_fdp_event_desc *desc =
			&((struct xnvme_spec_fdp_event_desc *)buf)[i];
		wrtn += fprintf(stream, "{ type: %#" PRIx8 ", event enabled: %" PRIu8 " }\n",
				desc->type, desc->fdpeta.ee);
		desc += sizeof(struct xnvme_spec_fdp_event_desc);
	}

	return wrtn;
}

int
xnvme_spec_feat_fdp_events_pr(void *buf, struct xnvme_spec_feat feat, int opts)
{
	return xnvme_spec_feat_fdp_events_fpr(stdout, buf, feat, opts);
}

static int
lblk_scopy_fmt_zero_yaml(FILE *stream, const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
			 int indent, const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*sslba: 0x%016" PRIx64 "%s", indent, "", entry->slba, sep);

	wrtn += fprintf(stream, "%*snlb: %" PRIu32 "%s", indent, "", entry->nlb, sep);

	wrtn += fprintf(stream, "%*seilbrt: 0x%08" PRIx32 "%s", indent, "", entry->eilbrt, sep);

	wrtn += fprintf(stream, "%*selbatm: 0x%08" PRIx32 "%s", indent, "", entry->elbatm, sep);

	wrtn += fprintf(stream, "%*selbat: 0x%08" PRIx32, indent, "", entry->elbat);

	return wrtn;
}

int
xnvme_spec_nvm_scopy_fmt_zero_fpr(FILE *stream, const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
				  int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_nvm_scopy_fmt_zero:");
	if (!entry) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += lblk_scopy_fmt_zero_yaml(stream, entry, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_nvm_scopy_fmt_zero_pr(const struct xnvme_spec_nvm_scopy_fmt_zero *entry, int opts)
{
	return xnvme_spec_nvm_scopy_fmt_zero_fpr(stdout, entry, opts);
}

int
xnvme_spec_nvm_scopy_source_range_fpr(FILE *stream,
				      const struct xnvme_spec_nvm_scopy_source_range *srange,
				      uint8_t nr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_nvm_scopy_source_range:");
	if (!srange) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nranges: %" PRIu8 "\n", (uint8_t)(nr + 1));
	wrtn += fprintf(stream, "  nr: %" PRIu8 "\n", nr);
	wrtn += fprintf(stream, "  entries:\n");

	for (int i = 0; i < XNVME_SPEC_NVM_SCOPY_NENTRY_MAX; ++i) {
		if (i > nr) {
			break;
		}

		wrtn += fprintf(stream, "  - { ");
		wrtn += lblk_scopy_fmt_zero_yaml(stream, &srange->entry[i], 0, ", ");
		wrtn += fprintf(stream, " }\n");
	}

	return wrtn;
}

int
xnvme_spec_nvm_scopy_source_range_pr(const struct xnvme_spec_nvm_scopy_source_range *srange,
				     uint8_t nr, int opts)
{
	return xnvme_spec_nvm_scopy_source_range_fpr(stdout, srange, nr, opts);
}

// TODO: add yaml file and call it
int
xnvme_spec_nvm_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_nvm_idfy_ctrlr:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  vsl: %" PRIu8 "\n", idfy->vsl);
	wrtn += fprintf(stream, "  wzsl: %" PRIu8 "\n", idfy->wzsl);
	wrtn += fprintf(stream, "  wusl: %" PRIu8 "\n", idfy->wusl);
	wrtn += fprintf(stream, "  dmrl: %" PRIu8 "\n", idfy->dmrl);
	wrtn += fprintf(stream, "  dmrsl: %" PRIu32 "\n", idfy->dmrsl);
	wrtn += fprintf(stream, "  dmsl: %" PRIu64 "\n", idfy->dmsl);

	return wrtn;
}

int
xnvme_spec_nvm_idfy_ctrlr_pr(struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts)
{
	return xnvme_spec_nvm_idfy_ctrlr_fpr(stdout, idfy, opts);
}

int
xnvme_spec_nvm_idfy_ns_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ns *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_nvm_idfy_ns:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  mcl: %" PRIu32 "\n", idfy->mcl);
	wrtn += fprintf(stream, "  mssrl: %" PRIu16 "\n", idfy->mssrl);
	wrtn += fprintf(stream, "  msrc: %" PRIu8 "\n", idfy->msrc);

	return wrtn;
}

int
xnvme_spec_nvm_idfy_ns_pr(struct xnvme_spec_nvm_idfy_ns *idfy, int opts)
{
	return xnvme_spec_nvm_idfy_ns_fpr(stdout, idfy, opts);
}

int
xnvme_spec_drecv_idfy_fpr(FILE *stream, struct xnvme_spec_idfy_dir_rp *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_idfy_dir_rp:");
	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  directives_supported:\n");
	wrtn += fprintf(stream, "    identify: %" PRIu8 "\n", idfy->directives_supported.identify);
	wrtn += fprintf(stream, "    streams: %" PRIu8 "\n", idfy->directives_supported.streams);
	wrtn += fprintf(stream, "    data placement: %" PRIu8 "\n",
			idfy->directives_supported.data_placement);
	wrtn += fprintf(stream, "  directives_enabled:\n");
	wrtn += fprintf(stream, "    identify: %" PRIu8 "\n", idfy->directives_enabled.identify);
	wrtn += fprintf(stream, "    streams: %" PRIu8 "\n", idfy->directives_enabled.streams);
	wrtn += fprintf(stream, "    data_placement: %" PRIu8 "\n",
			idfy->directives_enabled.data_placement);
	wrtn += fprintf(stream, "  directives_persistence:\n");
	wrtn += fprintf(stream, "    identify: %" PRIu8 "\n",
			idfy->directives_persistence.identify);
	wrtn += fprintf(stream, "    streams: %" PRIu8 "\n", idfy->directives_persistence.streams);
	wrtn += fprintf(stream, "    data_placement: %" PRIu8 "\n",
			idfy->directives_persistence.data_placement);

	return wrtn;
}

int
xnvme_spec_drecv_idfy_pr(struct xnvme_spec_idfy_dir_rp *idfy, int opts)
{
	return xnvme_spec_drecv_idfy_fpr(stdout, idfy, opts);
}

int
xnvme_spec_drecv_srp_fpr(FILE *stream, struct xnvme_spec_streams_dir_rp *srp, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_streams_dir_rp:");
	if (!srp) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  msl: %" PRIu16 "\n", srp->msl);
	wrtn += fprintf(stream, "  nssa: %" PRIu16 "\n", srp->nssa);
	wrtn += fprintf(stream, "  nsso: %" PRIu16 "\n", srp->nsso);
	wrtn += fprintf(stream, "  multi_host: %" PRIu8 "\n", srp->nssc.bits.multi_host);
	wrtn += fprintf(stream, "  sws: %" PRIu32 "\n", srp->sws);
	wrtn += fprintf(stream, "  sgs: %" PRIu16 "\n", srp->sgs);
	wrtn += fprintf(stream, "  nsa: %" PRIu16 "\n", srp->nsa);
	wrtn += fprintf(stream, "  nso: %" PRIu16 "\n", srp->nso);

	return wrtn;
}

int
xnvme_spec_drecv_srp_pr(struct xnvme_spec_streams_dir_rp *srp, int opts)
{
	return xnvme_spec_drecv_srp_fpr(stdout, srp, opts);
}

int
xnvme_spec_drecv_sgs_fpr(FILE *stream, struct xnvme_spec_streams_dir_gs *sgs, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_streams_dir_rp:");
	if (!sgs) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  open_sc: %" PRIu16 "\n", sgs->open_sc);
	for (int osc = 0; osc < sgs->open_sc; ++osc) {
		wrtn += fprintf(stream, "  - sid[%d]: %" PRIu16 "\n", osc, sgs->sid[osc]);
	}

	return wrtn;
}

int
xnvme_spec_drecv_sgs_pr(struct xnvme_spec_streams_dir_gs *sgs, int opts)
{
	return xnvme_spec_drecv_sgs_fpr(stdout, sgs, opts);
}

int
xnvme_spec_drecv_sar_fpr(FILE *stream, struct xnvme_spec_alloc_resource sar, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_alloc_resource:");
	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nsa: %" PRIu32 "\n", sar.bits.nsa);

	return wrtn;
}

int
xnvme_spec_drecv_sar_pr(struct xnvme_spec_alloc_resource sar, int opts)
{
	return xnvme_spec_drecv_sar_fpr(stdout, sar, opts);
}

int
xnvme_spec_znd_descr_fpr_yaml(FILE *stream, const struct xnvme_spec_znd_descr *descr, int indent,
			      const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*szslba: 0x%016" PRIx64 "%s", indent, "", descr->zslba, sep);

	wrtn += fprintf(stream, "%*swp: 0x%016" PRIx64 "%s", indent, "", descr->wp, sep);

	wrtn += fprintf(stream, "%*szcap: %" PRIu64 "%s", indent, "", descr->zcap, sep);

	wrtn += fprintf(stream, "%*szt: %#" PRIx8 "%s", indent, "", descr->zt, sep);

	wrtn += fprintf(stream, "%*szs: %*s%s", indent, "", 17,
			xnvme_spec_znd_state_str(descr->zs), sep);

	wrtn += fprintf(stream, "%*sza: '0b" XNVME_I8_FMT "'", indent, "",
			XNVME_I8_TO_STR(descr->za.val));

	return wrtn;
}

int
xnvme_spec_znd_descr_fpr(FILE *stream, const struct xnvme_spec_znd_descr *descr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_znd_descr:");
	if (!descr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += xnvme_spec_znd_descr_fpr_yaml(stream, descr, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_znd_descr_pr(const struct xnvme_spec_znd_descr *descr, int opts)
{
	return xnvme_spec_znd_descr_fpr(stdout, descr, opts);
}

int
xnvme_spec_znd_log_changes_fpr(FILE *stream, const struct xnvme_spec_znd_log_changes *changes,
			       int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_znd_log_changes:");
	if (!changes) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nidents: %" PRIu16 "\n", changes->nidents);

	if (changes->nidents) {
		return wrtn;
	}

	wrtn += fprintf(stream, "  idents:");
	if (!changes->nidents) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (uint16_t idx = 0; idx < changes->nidents; ++idx) {
		wrtn += fprintf(stream, "    - 0x%016" PRIx64 "\n", changes->idents[idx]);
	}

	return wrtn;
}

int
xnvme_spec_znd_log_changes_pr(const struct xnvme_spec_znd_log_changes *changes, int opts)
{
	return xnvme_spec_znd_log_changes_fpr(stdout, changes, opts);
}

int
xnvme_spec_znd_report_hdr_fpr(FILE *stream, const struct xnvme_spec_znd_report_hdr *hdr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_spec_znd_report_hdr:");
	if (!hdr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nzones: %" PRIu64 "\n", hdr->nzones);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_znd_report_hdr_pr(const struct xnvme_spec_znd_report_hdr *hdr, int opts)
{
	return xnvme_spec_znd_report_hdr_fpr(stdout, hdr, opts);
}

int
xnvme_spec_znd_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_znd_idfy_ctrlr:");
	if (!zctrlr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  zasl: %" PRIu8 "\n", zctrlr->zasl);

	return wrtn;
}

int
xnvme_spec_znd_idfy_ctrlr_pr(struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts)
{
	return xnvme_spec_znd_idfy_ctrlr_fpr(stdout, zctrlr, opts);
}

int
xnvme_spec_znd_idfy_lbafe_fpr(FILE *stream, struct xnvme_spec_znd_idfy_lbafe *zonef, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	if (!zonef) {
		wrtn += fprintf(stream, "~");
		return wrtn;
	}

	wrtn += fprintf(stream, "{ zsze: %" PRIu64 ", zdes: %" PRIu8 " }", zonef->zsze,
			zonef->zdes);

	return wrtn;
}

int
xnvme_spec_znd_idfy_ns_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ns *zns, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_znd_idfy_ns:");
	if (!zns) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  zoc: { vzcap: %" PRIu16 ", zae: %" PRIu16 " }\n",
			zns->zoc.bits.vzcap, zns->zoc.bits.zae);

	wrtn += fprintf(stream, "  ozcs:\n");
	wrtn += fprintf(stream, "    val: 0x%" PRIx16 "\n", zns->ozcs.val);
	wrtn += fprintf(stream, "    razb: %" PRIu16 "\n", zns->ozcs.bits.razb);
	wrtn += fprintf(stream, "    zrwasup: %" PRIu16 "\n", zns->ozcs.bits.zrwasup);

	wrtn += fprintf(stream, "  mar: %" PRIu32 "\n", zns->mar);
	wrtn += fprintf(stream, "  mor: %" PRIu32 "\n", zns->mor);

	wrtn += fprintf(stream, "  rrl: %" PRIu32 "\n", zns->rrl);
	wrtn += fprintf(stream, "  frl: %" PRIu32 "\n", zns->frl);

	wrtn += fprintf(stream, "  lbafe:\n");
	for (int nfmt = 0; nfmt < 16; ++nfmt) {
		wrtn += fprintf(stream, "  - ");
		wrtn += xnvme_spec_znd_idfy_lbafe_fpr(stream, &zns->lbafe[nfmt], opts);
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "  numzrwa: %" PRIu32 "\n", zns->numzrwa);
	wrtn += fprintf(stream, "  zrwas: %" PRIu16 "\n", zns->zrwas);
	wrtn += fprintf(stream, "  zrwafg: %" PRIu16 "\n", zns->zrwafg);

	wrtn += fprintf(stream, "  zrwacap:\n");
	wrtn += fprintf(stream, "    val: 0x%" PRIx8 "\n", zns->zrwacap.val);
	wrtn += fprintf(stream, "    expflushsup: %" PRIu8 "\n", zns->zrwacap.bits.expflushsup);

	return wrtn;
}

int
xnvme_spec_znd_idfy_ns_pr(struct xnvme_spec_znd_idfy_ns *zns, int opts)
{
	return xnvme_spec_znd_idfy_ns_fpr(stdout, zns, opts);
}

int
xnvme_spec_kvs_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_kvs_idfy_ns *idfy, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "xnvme_spec_kvs_idfy_ns:");

	if (!idfy) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nsze: %" PRIu64 "\n", idfy->nsze);
	wrtn += fprintf(stream, "  nuse: %" PRIu64 "\n", idfy->nuse);
	wrtn += fprintf(stream, "  nkvf: %" PRIu8 "\n", idfy->nkvf);

	wrtn += fprintf(stream, "  kvf:\n");
	for (int i = 0; i < idfy->nkvf; i++) {
		wrtn += fprintf(stream, "  - kml: %" PRIu16 "\n", idfy->kvf[i].kml);
		wrtn += fprintf(stream, "    vml: %" PRIu32 "\n", idfy->kvf[i].vml);
		wrtn += fprintf(stream, "    mnk: %" PRIu32 "\n", idfy->kvf[i].mnk);
	}
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_spec_kvs_idfy_ns_pr(const struct xnvme_spec_kvs_idfy_ns *idfy, int opts)
{
	return xnvme_spec_kvs_idfy_ns_fpr(stdout, idfy, opts);
}
