// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

int
xnvme_adm_format(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t lbafl, uint8_t lbafu,
		 uint8_t mset, uint8_t ses, uint8_t pi, uint8_t pil)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_FMT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.format.lbafl = lbafl;
	ctx->cmd.format.lbafu = lbafu;
	ctx->cmd.format.mset = mset;
	ctx->cmd.format.pi = pi;
	ctx->cmd.format.pil = pil;
	ctx->cmd.format.ses = ses;

	return xnvme_cmd_pass_admin(ctx, NULL, 0, NULL, 0);
}

int
xnvme_nvm_sanitize(struct xnvme_cmd_ctx *ctx, uint8_t sanact, uint8_t ause, uint32_t ovrpat,
		   uint8_t owpass, uint8_t oipbp, uint8_t nodas)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_SANITIZE;
	ctx->cmd.sanitize.sanact = sanact;
	ctx->cmd.sanitize.ause = ause;
	ctx->cmd.sanitize.owpass = owpass;
	ctx->cmd.sanitize.oipbp = oipbp;
	ctx->cmd.sanitize.nodas = nodas;
	ctx->cmd.sanitize.ause = ause;
	ctx->cmd.sanitize.ovrpat = ovrpat;

	return xnvme_cmd_pass_admin(ctx, NULL, 0, NULL, 0);
}

int
xnvme_nvm_read(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf)
{
	size_t dbuf_nbytes = dbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}

int
xnvme_nvm_write(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		const void *dbuf, const void *mbuf)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, cmbuf, mbuf_nbytes);
}

int
xnvme_nvm_write_uncorrectable(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba,
			      uint16_t nlb)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

int
xnvme_nvm_write_zeroes(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_ZEROES;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.write_zeroes.slba = slba;
	ctx->cmd.write_zeroes.nlb = nlb;

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

void
xnvme_prep_nvm(struct xnvme_cmd_ctx *ctx, uint8_t opcode, uint32_t nsid, uint64_t slba,
	       uint16_t nlb)
{
	ctx->cmd.common.opcode = opcode;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;
}

int
xnvme_nvm_dsm(struct xnvme_cmd_ctx *ctx, uint32_t nsid, struct xnvme_spec_dsm_range *dsm_range,
	      uint8_t nr, bool ad, bool idw, bool idr)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_DATASET_MANAGEMENT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.dsm.nr = nr;
	ctx->cmd.dsm.ad = ad;
	ctx->cmd.dsm.idw = idw;
	ctx->cmd.dsm.idr = idr;

	return xnvme_cmd_pass(ctx, (void *)dsm_range, sizeof(*dsm_range) * (nr + 1), NULL, 0);
}

int
xnvme_nvm_scopy(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt)
{
	size_t ranges_nbytes = 0;

	if (copy_fmt & XNVME_NVM_SCOPY_FMT_ZERO) {
		ranges_nbytes = (nr + 1) * sizeof(*ranges);
	}
	if (copy_fmt & XNVME_NVM_SCOPY_FMT_SRCLEN) {
		ranges_nbytes = (nr + 1) * sizeof(struct xnvme_spec_nvm_cmd_scopy_fmt_srclen);
	}

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_SCOPY;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.scopy.sdlba = sdlba;
	ctx->cmd.scopy.nr = nr;

	// TODO: consider the remaining command-fields

	return xnvme_cmd_pass(ctx, ranges, ranges_nbytes, NULL, 0);
}

int
xnvme_nvm_mgmt_recv(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t mo, uint16_t mos, void *dbuf,
		    uint32_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_IO_MGMT_RECV;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.mgmt.mgmt_recv.mo = mo;
	ctx->cmd.mgmt.mgmt_recv.mos = mos;
	ctx->cmd.mgmt.mgmt_recv.numd = dbuf_nbytes / sizeof(uint32_t) - 1u;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_nvm_mgmt_send(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t mo, uint16_t mos, void *dbuf,
		    uint32_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_IO_MGMT_SEND;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.mgmt.mgmt_send.mo = mo;
	ctx->cmd.mgmt.mgmt_send.mos = mos;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_nvm_compare(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		  void *dbuf, void *mbuf)
{
	size_t dbuf_nbytes = dbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_COMPARE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.compare.slba = slba;
	ctx->cmd.compare.nlb = nlb;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}