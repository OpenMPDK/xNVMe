// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

/**
 * Constructs an LBA range if --slba and --elba are not provided by CLI
 *  - Stored in [rng_slba, rng_elba]
 *
 * Allocates buffers to support transfers of mdts_naddr
 *  - wbuf -- to be used for writes
 *  - rbuf -- to be used for read
 */
static int
boilerplate(struct xnvme_cli *cli, uint8_t **wbuf, uint8_t **rbuf, size_t *buf_nbytes,
	    uint64_t *mdts_naddr, uint32_t *nsid, uint64_t *rng_slba, uint64_t *rng_elba)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint64_t rng_naddr;
	int err;

	*nsid = xnvme_dev_get_nsid(cli->args.device);

	*rng_slba = cli->args.slba;
	*rng_elba = cli->args.elba;

	*mdts_naddr = XNVME_MAX(XNVME_MIN(geo->mdts_nbytes / geo->lba_nbytes, 256), 1);
	*buf_nbytes = (*mdts_naddr) * geo->lba_nbytes;

	// Construct a range if none is given
	if (!(cli->given[XNVME_CLI_OPT_SLBA] && cli->given[XNVME_CLI_OPT_ELBA])) {
		*rng_slba = 0;
		*rng_elba = (1 << 28) / geo->lba_nbytes; // About 256MB
	}
	rng_naddr = *rng_elba - *rng_slba + 1;

	// Verify range
	if (*rng_elba <= *rng_slba) {
		err = -EINVAL;
		xnvme_cli_perr("Invalid range: [rng_slba,rng_elba]", err);
		return err;
	}
	// TODO: verify that the range is sufficiently large

	*wbuf = xnvme_buf_alloc(dev, *buf_nbytes);
	if (!*wbuf) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	*rbuf = xnvme_buf_alloc(dev, *buf_nbytes);
	if (!*rbuf) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		xnvme_buf_free(dev, *wbuf);
		*wbuf = NULL;
		return err;
	}

	xnvme_cli_pinf("nsid: 0x%x", *nsid);
	xnvme_cli_pinf("range: { slba: 0x%016lx, elba: 0x%016lx, naddr: %zu }", *rng_slba,
		       *rng_elba, rng_naddr);
	xnvme_cli_pinf("buf_nbytes: %zu", *buf_nbytes);
	xnvme_cli_pinf("mdts_naddr: %zu", *mdts_naddr);

	return 0;
}

static int
read_and_compare_lba_range(uint8_t *rbuf, const uint8_t *cbuf, const uint64_t rng_slba,
			   const uint64_t nlb, const uint64_t mdts_naddr,
			   const struct xnvme_geo *geo, struct xnvme_dev *dev, uint32_t nsid,
			   uint64_t *compared_bytes)
{
	int err = 0;
	uint64_t read_bytes = 0;
	*compared_bytes = 0;
	xnvme_cli_pinf("Reading and comparing in LBA range [%ld,%ld]", rng_slba, rng_slba + nlb);

	for (uint64_t read_lbs = 0, r_nlb = 0; read_lbs < nlb; read_lbs += r_nlb) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		r_nlb = XNVME_MIN(mdts_naddr, nlb - read_lbs);

		err = xnvme_nvm_read(&ctx, nsid, rng_slba + read_lbs, r_nlb - 1, rbuf, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_pinf("xnvme_nvm_read(): {err: 0x%x, slba: 0x%016lx}", err,
				       rng_slba + read_lbs);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}

		read_bytes = r_nlb * geo->lba_nbytes;
		if (xnvme_buf_diff(cbuf, rbuf, read_bytes)) {
			xnvme_buf_diff_pr(cbuf, rbuf, read_bytes, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
		*compared_bytes += read_bytes;
	}

exit:
	return err;
}

static int
fill_lba_range_and_write_buffer_with_character(uint8_t *wbuf, size_t buf_nbytes, uint64_t rng_slba,
					       uint64_t rng_elba, uint64_t mdts_naddr,
					       struct xnvme_dev *dev, const struct xnvme_geo *geo,
					       uint32_t nsid, char character,
					       uint64_t *written_bytes)
{
	int err;

	memset(wbuf, character, buf_nbytes);

	*written_bytes = 0;
	for (uint64_t slba = rng_slba; slba <= rng_elba; slba += mdts_naddr) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		uint64_t nlb = XNVME_MIN(rng_elba - slba, mdts_naddr - 1);
		*written_bytes += (1 + nlb) * geo->lba_nbytes;

		err = xnvme_nvm_write(&ctx, nsid, slba, nlb, wbuf, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_pinf("xnvme_nvm_write(): {err: 0x%x, slba: 0x%016lx}", err,
				       slba);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	return 0;

exit:
	return err;
}

/**
 * 0) Fill wbuf with '!'
 * 1) Write the entire LBA range [slba, elba] using wbuf
 * 2) Fill wbuf with a repeating sequence of letters A to Z
 * 3) Scatter the content of wbuf within [slba,elba]
 * 4) Read, with exponential stride, within [slba,elba] using rbuf
 * 5) Verify that the content of rbuf is the same as wbuf
 */
static int
sub_io(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid;
	uint64_t rng_slba, rng_elba, mdts_naddr;
	size_t buf_nbytes;
	uint8_t *wbuf = NULL, *rbuf = NULL;
	int err;
	uint64_t written_bytes = 0;

	err = boilerplate(cli, &wbuf, &rbuf, &buf_nbytes, &mdts_naddr, &nsid, &rng_slba,
			  &rng_elba);
	if (err) {
		xnvme_cli_perr("boilerplate()", err);
		goto exit;
	}

	xnvme_cli_pinf("Writing '!' to LBA range [slba,elba]");
	err = fill_lba_range_and_write_buffer_with_character(wbuf, buf_nbytes, rng_slba, rng_elba,
							     mdts_naddr, dev, geo, nsid, '!',
							     &written_bytes);
	if (err) {
		xnvme_cli_perr("fill_lba_range_and_write_buffer_with_character()", err);
		goto exit;
	}

	xnvme_cli_pinf("Writing payload scattered within LBA range [slba,elba]");
	err = xnvme_buf_fill(wbuf, buf_nbytes, "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	for (uint64_t count = 0; count < mdts_naddr; ++count) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t wbuf_ofz = count * geo->lba_nbytes;
		uint64_t slba = rng_slba + count * 4;

		err = xnvme_nvm_write(&ctx, nsid, slba, 0, wbuf + wbuf_ofz, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_pinf("xnvme_nvm_write(): "
				       "{err: 0x%x, slba: 0x%016lx}",
				       err, slba);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvme_cli_pinf("Read scattered payload within LBA range [slba,elba]");

	xnvme_buf_clear(rbuf, buf_nbytes);
	for (uint64_t count = 0; count < mdts_naddr; ++count) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t rbuf_ofz = count * geo->lba_nbytes;
		uint64_t slba = rng_slba + count * 4;

		err = xnvme_nvm_read(&ctx, nsid, slba, 0, rbuf + rbuf_ofz, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_pinf("xnvme_nvm_read(): "
				       "{err: 0x%x, slba: 0x%016lx}",
				       err, slba);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvme_cli_pinf("Comparing wbuf and rbuf");
	if (xnvme_buf_diff(wbuf, rbuf, buf_nbytes)) {
		xnvme_buf_diff_pr(wbuf, rbuf, buf_nbytes, XNVME_PR_DEF);
		goto exit;
	}

exit:
	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);

	return err;
}

/**
 * 0) Fill wbuf with '!'
 *
 * 1) Write the entire LBA range [slba, elba] using wbuf
 *
 * 2) Fill wbuf with a repeating sequence of letters A to Z
 *
 * 3) Scatter the content of wbuf within [slba,elba]
 *
 * 4) Read, with exponential stride, within [slba,elba] using rbuf
 *
 * 5) Verify that the content of rbuf is the same as wbuf
 */
static int
test_scopy(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid;
	uint64_t rng_slba, rng_elba, xfer_naddr;
	size_t buf_nbytes;
	uint8_t *wbuf = NULL, *rbuf = NULL;

	struct xnvme_spec_nvm_scopy_source_range *sranges = NULL; // For the copy-payload
	struct xnvme_spec_nvm_idfy_ns *nvm = NULL;
	uint64_t sdlba = 0;
	enum xnvme_nvm_scopy_fmt copy_fmt;
	int err;
	uint64_t written_bytes = 0;

	err = boilerplate(cli, &wbuf, &rbuf, &buf_nbytes, &xfer_naddr, &nsid, &rng_slba,
			  &rng_elba);
	if (err) {
		xnvme_cli_perr("boilerplate()", err);
		goto exit;
	}

	nvm = (void *)xnvme_dev_get_ns(dev);
	xnvme_spec_nvm_idfy_ns_pr(nvm, XNVME_PR_DEF);

	if (nvm->msrc) {
		xfer_naddr = XNVME_MIN(XNVME_MIN((uint64_t)nvm->msrc + 1, xfer_naddr), nvm->mcl);
	}
	buf_nbytes = xfer_naddr * geo->nbytes;

	sranges = xnvme_buf_alloc(dev, sizeof(*sranges));
	if (!sranges) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(sranges, 0, sizeof(*sranges));

	// Copy to the end of [slba,elba]
	sdlba = rng_elba - xfer_naddr;

	// NVMe-struct copy format
	copy_fmt = XNVME_NVM_SCOPY_FMT_ZERO;

	xnvme_spec_nvm_idfy_ctrlr_pr((struct xnvme_spec_nvm_idfy_ctrlr *)xnvme_dev_get_ctrlr(dev),
				     XNVME_PR_DEF);
	xnvme_spec_nvm_idfy_ns_pr((struct xnvme_spec_nvm_idfy_ns *)xnvme_dev_get_ns(dev),
				  XNVME_PR_DEF);

	err = fill_lba_range_and_write_buffer_with_character(wbuf, buf_nbytes, rng_slba, rng_elba,
							     xfer_naddr, dev, geo, nsid, '!',
							     &written_bytes);
	if (err) {
		xnvme_cli_perr("fill_lba_range_and_write_buffer_with_character()", err);
		goto exit;
	}

	xnvme_cli_pinf("Writing payload scattered within LBA range [slba,elba]");
	xnvme_buf_fill(wbuf, buf_nbytes, "anum");
	for (uint64_t count = 0; count < xfer_naddr; ++count) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t wbuf_ofz = count * geo->lba_nbytes;
		uint64_t slba = rng_slba + count * 4;

		sranges->entry[count].slba = slba;
		sranges->entry[count].nlb = 0;

		err = xnvme_nvm_write(&ctx, nsid, slba, 0, wbuf + wbuf_ofz, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_write()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		uint8_t nr = xfer_naddr - 1;

		xnvme_cli_pinf("scopy sranges to sdlba: 0x%016lx", sdlba);
		xnvme_spec_nvm_scopy_source_range_pr(sranges, nr, XNVME_PR_DEF);

		err = xnvme_nvm_scopy(&ctx, nsid, sdlba, sranges->entry, nr, copy_fmt);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_cmd_scopy()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}

		xnvme_cli_pinf("read sdlba: 0x%016lx", sdlba);
		memset(rbuf, 0, buf_nbytes);
		err = xnvme_nvm_read(&ctx, nsid, sdlba, nr, rbuf, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}

		xnvme_cli_pinf("Comparing wbuf and rbuf");
		if (xnvme_buf_diff(wbuf, rbuf, buf_nbytes)) {
			xnvme_buf_diff_pr(wbuf, rbuf, buf_nbytes, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
	}

exit:
	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);
	xnvme_buf_free(dev, sranges);

	return err;
}

/**
 * 0) Fill wbuf with '!'
 * 1) Write the entire LBA range [slba, elba] using wbuf
 * 2) Make sure that we wrote '!'
 * 3) Execute the write zeroes command
 * 4) Fill wbuf with 0
 * 5) Read, with exponential stride, within [slba,elba] using rbuf
 * 6) Verify that the content of rbuf is the same as wbuf
 */
static int
test_write_zeroes(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid;
	uint64_t rng_slba, rng_elba, mdts_naddr, nlb;
	size_t buf_nbytes;
	uint8_t *wbuf = NULL, *rbuf = NULL;
	int err;
	uint64_t compared_bytes = 0;
	uint64_t written_bytes = 0;

	err = boilerplate(cli, &wbuf, &rbuf, &buf_nbytes, &mdts_naddr, &nsid, &rng_slba,
			  &rng_elba);
	nlb = rng_elba - rng_slba;
	if (err) {
		xnvme_cli_perr("boilerplate()", err);
		goto exit;
	}

	err = fill_lba_range_and_write_buffer_with_character(wbuf, buf_nbytes, rng_slba, rng_elba,
							     mdts_naddr, dev, geo, nsid, '!',
							     &written_bytes);
	if (err) {
		xnvme_cli_perr("fill_lba_range_and_write_buffer_with_character()", err);
		goto exit;
	}
	xnvme_cli_pinf("Written bytes %ld with !", written_bytes);

	xnvme_buf_clear(rbuf, buf_nbytes);
	err = read_and_compare_lba_range(rbuf, wbuf, rng_slba, nlb, mdts_naddr, geo, dev, nsid,
					 &compared_bytes);
	if (err) {
		xnvme_cli_perr("read_and_compare_lba_range()", err);
		goto exit;
	}
	xnvme_cli_pinf("Compared %ld bytes to !", compared_bytes);

	for (uint64_t slba = rng_slba; slba < rng_elba; slba += mdts_naddr) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		uint16_t nlb = XNVME_MIN((rng_elba - slba) * geo->lba_nbytes, UINT16_MAX);

		err = xnvme_nvm_write_zeroes(&ctx, nsid, slba, nlb);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_write_zeroes()", err);
			err = err ? err : -EIO;
			goto exit;
		}
	}
	xnvme_cli_pinf("Wrote zeroes to LBA range [%ld,%ld]", rng_slba, rng_elba);

	// Set the rbuf to != 0 so we know that we read zeroes
	memset(rbuf, 'a', buf_nbytes);
	xnvme_buf_clear(wbuf, buf_nbytes);
	err = read_and_compare_lba_range(rbuf, wbuf, rng_slba, nlb, mdts_naddr, geo, dev, nsid,
					 &compared_bytes);
	if (err) {
		xnvme_cli_perr("read_and_compare_lba_range()", err);
		goto exit;
	}
	xnvme_cli_pinf("Compared %ld bytes to zero", compared_bytes);

exit:
	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);

	return err;
}

/**
 * 0) Fill wbuff with '!'
 * 1) Write the entire LBA range [slba, elba] using wbuf
 * 2) Read the entire LBA range and compare to wbuff
 * 3) Execute write uncorrectable command for LBA range
 * 4) Read the entire LBA range and verify that error is returned
 */
static int
test_write_uncorrectable(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid;
	uint64_t rng_slba, rng_elba, mdts_naddr, nlb;
	size_t buf_nbytes;
	uint8_t *wbuf = NULL, *rbuf = NULL;
	int err;
	uint64_t compared_bytes = 0;
	uint64_t written_bytes = 0;
	bool entered_uncorrectable_loop = false;

	err = boilerplate(cli, &wbuf, &rbuf, &buf_nbytes, &mdts_naddr, &nsid, &rng_slba,
			  &rng_elba);
	nlb = rng_elba - rng_slba;
	if (err) {
		xnvme_cli_perr("boilerplate()", err);
		goto exit;
	}

	/* Fill lbas with '!' */
	err = fill_lba_range_and_write_buffer_with_character(wbuf, buf_nbytes, rng_slba, rng_elba,
							     mdts_naddr, dev, geo, nsid, '!',
							     &written_bytes);
	if (err) {
		xnvme_cli_perr("fill_lba_range_and_write_buffer_with_character()", err);
		goto exit;
	}

	xnvme_buf_clear(rbuf, buf_nbytes);
	err = read_and_compare_lba_range(rbuf, wbuf, rng_slba, nlb, mdts_naddr, geo, dev, nsid,
					 &compared_bytes);
	if (err) {
		xnvme_cli_perr("read_and_compare_lba_range()", err);
		goto exit;
	}
	xnvme_cli_pinf("Compared %ld bytes to !", compared_bytes);

	/* Set all lbas to uncorrectable */
	xnvme_cli_pinf("Setting to LBA range [slba,elba] as uncorrectable");
	entered_uncorrectable_loop = true;

	for (uint64_t execed_lbs = 0, r_nlb = 0; execed_lbs < nlb; execed_lbs += r_nlb) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		r_nlb = XNVME_MIN(mdts_naddr, nlb - execed_lbs);

		err = xnvme_nvm_write_uncorrectable(&ctx, nsid, rng_slba + execed_lbs, r_nlb - 1);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_write_uncorrectable()", err);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	/* Make sure that all lbas return an error when read */
	xnvme_cli_pinf("Reading from LBA range [slba,elba]");
	for (uint64_t count = 0; count < mdts_naddr; ++count) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t rbuf_ofz = count * geo->lba_nbytes;
		uint64_t slba = rng_slba + count * 4;
		int read_err;

		read_err = xnvme_nvm_read(&ctx, nsid, slba, 0, rbuf + rbuf_ofz, NULL);
		if (!read_err) {
			err = -EIO;
			xnvme_cli_perr("inefective xnvme_nvm_write_uncorrectable", err);
			goto exit;
		}
	}

exit:
	if (entered_uncorrectable_loop) {
		xnvme_cli_pinf("Writing zeros to lba range to reset unocrrectable bit");
		int recover_err = fill_lba_range_and_write_buffer_with_character(
			wbuf, buf_nbytes, rng_slba, rng_elba, mdts_naddr, dev, geo, nsid, 0,
			&written_bytes);
		if (recover_err) {
			xnvme_cli_perr("fill_lba_range_and_write_buffer_with_character()",
				       recover_err);
		}
	}

	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"io",
		"Basic Verification of being able to read, what was written",
		"Basic Verification of being able to read, what was written",
		sub_io,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"scopy",
		"Basic Verification of the Simple-Copy Command",
		"Basic Verification of the Simple-Copy Command",
		test_scopy,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"write_zeroes",
		"Basic verification of the write zeros command",
		"Basic verification of the write zeros command",
		test_write_zeroes,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"write_uncorrectable",
		"Basic verification of the write uncorrectable command",
		"Basic verification of the write uncorrectable command",
		test_write_uncorrectable,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	}};

static struct xnvme_cli g_cli = {
	.title = "Basic LBLK Verification",
	.descr_short = "Basic LBLK Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
