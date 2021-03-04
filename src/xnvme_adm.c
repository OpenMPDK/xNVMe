// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libxnvme.h>
#include <errno.h>

int
xnvme_adm_log(struct xnvme_cmd_ctx *ctx, uint8_t lid, uint8_t lsp, uint64_t lpo_nbytes,
	      uint32_t nsid, uint8_t rae, void *dbuf, uint32_t dbuf_nbytes)
{
	uint32_t numdw;

	if (!dbuf_nbytes) {
		XNVME_DEBUG("FAILED: !dbuf_nbytes");
		return -EINVAL;
	}
	if (lpo_nbytes & 0x3) {
		XNVME_DEBUG("FAILED: lpo_nbytes is not byte aligned");
		return -EINVAL;
	}

	// NOTE: yeah... well... we do not want the caller to rely on the
	// function to clear out the buffer... however... in case they forgot..
	// then it is done here
	memset(dbuf, 0, dbuf_nbytes);

	numdw = dbuf_nbytes / sizeof(uint32_t) - 1u;

	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_LOG;
	ctx->cmd.common.nsid = nsid;

	ctx->cmd.log.lid = lid;
	ctx->cmd.log.lsp = lsp;
	ctx->cmd.log.rae = rae;
	ctx->cmd.log.numdl = numdw & 0xFFFFu;
	ctx->cmd.log.numdu = (numdw >> 16) & 0xFFFFu;
	ctx->cmd.log.lpou = (uint32_t)(lpo_nbytes >> 32);
	ctx->cmd.log.lpol = (uint32_t)lpo_nbytes & 0xfffffff;

	// TODO: should we check ctrlr->lpa.edlp for ext. buf and lpo support?
	// TODO: add support for uuid?

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_idfy(struct xnvme_cmd_ctx *ctx, uint8_t cns, uint16_t cntid, uint8_t nsid,
	       uint16_t nvmsetid, uint8_t uuid, struct xnvme_spec_idfy *dbuf)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);

	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.idfy.cns = cns;
	ctx->cmd.idfy.cntid = cntid;
	ctx->cmd.idfy.nvmsetid = nvmsetid;
	ctx->cmd.idfy.uuid = uuid;

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_idfy_ctrlr(struct xnvme_cmd_ctx *ctx, struct xnvme_spec_idfy *dbuf)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);

	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx->cmd.idfy.cns = XNVME_SPEC_IDFY_CTRLR;

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_idfy_ctrlr_csi(struct xnvme_cmd_ctx *ctx, uint8_t csi, struct xnvme_spec_idfy *dbuf)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);

	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx->cmd.idfy.cns = XNVME_SPEC_IDFY_CTRLR_IOCS;
	ctx->cmd.idfy.csi = csi;

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_idfy_ns(struct xnvme_cmd_ctx *ctx, uint32_t nsid, struct xnvme_spec_idfy *dbuf)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);

	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx->cmd.common.nsid = (nsid) ? nsid : xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.idfy.cns = XNVME_SPEC_IDFY_NS;
	ctx->cmd.idfy.csi = 0x0;

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_idfy_ns_csi(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t csi,
		      struct xnvme_spec_idfy *dbuf)
{
	const size_t dbuf_nbytes = sizeof(*dbuf);

	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx->cmd.common.nsid = (nsid) ? nsid : xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.idfy.cns = XNVME_SPEC_IDFY_NS_IOCS;
	ctx->cmd.idfy.csi = csi;

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_gfeat(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t fid, uint8_t sel, void *dbuf,
		size_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_GFEAT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.gfeat.fid = fid;
	ctx->cmd.gfeat.sel = sel;

	// TODO: cdw14/uuid?

	return xnvme_cmd_pass_admin(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_adm_sfeat(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t fid, uint32_t feat, uint8_t save,
		const void *dbuf, size_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_ADM_OPC_SFEAT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.sfeat.fid = fid;
	ctx->cmd.sfeat.feat.val = feat;
	ctx->cmd.sfeat.save = save;

	return xnvme_cmd_pass_admin(ctx, (void *) dbuf, dbuf_nbytes, NULL, 0x0);
}
