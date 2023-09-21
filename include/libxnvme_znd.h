/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_znd.h
 */

#ifndef __LIBXNVME_ZND_H
/**
 * This include-guard is here despite header-guards being removed on all other
 * public API headers. This is because fio is using it and we do not wish to
 * break fio.
 */
#define __LIBXNVME_ZND_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @todo Document this
 * @todo Provide a generic xnvme_dev_get_ns_csi instead of this
 * @param dev
 * @return
 */
const struct xnvme_spec_znd_idfy_ns *
xnvme_znd_dev_get_ns(struct xnvme_dev *dev);

/**
 * @todo Document this
 * @todo Provide a generic xnvme_dev_get_ctrlr_csi instead of this
 * @param dev
 * @return
 */
const struct xnvme_spec_znd_idfy_ctrlr *
xnvme_znd_dev_get_ctrlr(struct xnvme_dev *dev);

/**
 * @todo Document this
 * @param dev
 * @return
 */
const struct xnvme_spec_znd_idfy_lbafe *
xnvme_znd_dev_get_lbafe(struct xnvme_dev *dev);

/**
 * Submit, and optionally wait for completion of, a Zone Management Receive
 *
 * @note When `opts | CMD_MODE_SYNC` then `ret` is filled with completion entry
 * upon return
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba Start LBA of the Zone to receive for
 * @param action the ::xnvme_spec_znd_cmd_mgmt_send_action
 * @param sf the ::xnvme_spec_znd_cmd_mgmt_recv_action_sf
 * @param partial partial request
 * @param dbuf pointer to data payload
 * @param dbuf_nbytes pointer to meta payload
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_znd_mgmt_recv(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba,
		    enum xnvme_spec_znd_cmd_mgmt_recv_action action,
		    enum xnvme_spec_znd_cmd_mgmt_recv_action_sf sf, uint8_t partial, void *dbuf,
		    uint32_t dbuf_nbytes);

/**
 * Fills 'zdescr' with the Zone on the given 'dev' that starts at 'slba'
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param slba The Zone Start LBA
 * @param zdescr Pointer to the ::xnvme_spec_znd_descr to fill
 *
 * @return On success, 0 is returned and 'zdescr' filled with matching Zone Descriptor. On error,
 * negative ``errno`` is returned to indicate the error, and the content of given 'zdescr' is
 * undefined.
 */
int
xnvme_znd_descr_from_dev(struct xnvme_dev *dev, uint64_t slba,
			 struct xnvme_spec_znd_descr *zdescr);

/**
 * Fills 'zdescr' with the first Zone on the given 'dev' in the given 'state'
 *
 * @param dev The device to receive descriptors from
 * @param state The state the Zone should be in
 * @param zdescr Pointer to the descriptor the function should fill
 *
 * @return On succes, 0 is returned and 'zdescr' filled with matching Zone
 * Descriptor. On error, negative ``errno`` is returned to indicate the error,
 * and the content of given 'zdescr' is undefined.
 */
int
xnvme_znd_descr_from_dev_in_state(struct xnvme_dev *dev, enum xnvme_spec_znd_state state,
				  struct xnvme_spec_znd_descr *zdescr);

/**
 * Ask the given device for how many zones in the given state via receive-action
 *
 * @param dev The device to stat
 * @param sfield Receive stats of the given ::xnvme_spec_znd_cmd_mgmt_recv_action_sf
 * @param nzones Pointer to store result in
 *
 * @return On Success, 0 is returned and 'nzones' filled with stat. On error,
 * negative ``errno`` is returned to indicate the error.
 */
int
xnvme_znd_stat(struct xnvme_dev *dev, enum xnvme_spec_znd_cmd_mgmt_recv_action_sf sfield,
	       uint64_t *nzones);

/**
 * Returns a list of changes since the last report was retrieved
 *
 * @note
 * Invoking this function clears the changed log
 *
 * @note
 * Caller is responsible for de-allocating the returned structure using
 * `xnvme_buf_free`
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and `errno` is set to indicate the error
 */
struct xnvme_spec_znd_log_changes *
xnvme_znd_log_changes_from_dev(struct xnvme_dev *dev);

/**
 * Submit, and optionally wait for completion of, a Zone Management Send
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param zslba Start LBA of the Zone to manage
 * @param select_all Ignore zslba, command affects all LBAs
 * @param action Management the ::xnvme_spec_znd_cmd_mgmt_send_action to perform with zone at zslba
 * or all LBAs when select_all is true
 * @param action_so the ::xnvme_spec_znd_mgmt_send_action_so option
 * @param dbuf For action=ZND_SEND_DESCRIPTOR provide buffer
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_znd_mgmt_send(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t zslba, bool select_all,
		    enum xnvme_spec_znd_cmd_mgmt_send_action action,
		    enum xnvme_spec_znd_mgmt_send_action_so action_so, void *dbuf);

/**
 * Submit, and optionally wait for completion of, a Zone Append
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param zslba First LBA of the Zone to append to
 * @param nlb number of LBAs, this is zero-based value
 * @param dbuf pointer to data payload
 * @param mbuf pointer to meta payload
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_znd_append(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t zslba, uint16_t nlb,
		 const void *dbuf, const void *mbuf);

/**
 * Submit, and optionally wait for completion of, a Zone RWA Commit
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param lba Commit from ZRWA to Zone up-to and including the given 'lba'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_znd_zrwa_flush(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t lba);

/**
 * Encapsulation of Zone Descriptors and Zone Descriptor Extensions
 *
 * This is not the structure from the Specification, rather, this encapsulates
 * it and provides MACRO-accessors and helper-functions to retrieve
 * zone-descriptors and when supported, extensions.
 *
 * For simplicity then only use getter function when accessing the report entries:
 *
 * xnvme_znd_report_get_descr(report, index, **descr, *ext)
 *
 * However, when zdes_bytes is 0, then `descr` is accessible as a regular array
 * When zdes > 0.
 *
 * @struct xnvme_znd_report
 */
struct xnvme_znd_report {
	uint64_t nzones;       ///< Nr. of zones in device
	uint32_t zd_nbytes;    ///< Zone Descriptor in 'B'
	uint32_t zdext_nbytes; ///< Zone Descriptor Extension in 'B'

	uint64_t zslba;    ///< First Zone in the report
	uint64_t zelba;    ///< Last Zone in the report
	uint32_t nentries; ///< Nr. of entries in report

	uint8_t extended; ///< Whether this report has extensions
	uint8_t _pad[3];
	uint64_t zrent_nbytes; ///< Zone Report Entry Size in 'B'

	uint64_t report_nbytes;  ///< Size of this struct in bytes
	uint64_t entries_nbytes; ///< Size of the entries in bytes

	///< Array of structs, column format: `descr[,descr_ext]`
	uint8_t storage[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_znd_report) == 64, "Incorrect size")

/**
 * Access Zone Descriptors, by entry index, in the given znd_report
 *
 * NOTE: the MACRO is kept for backwards compatibility with the upstream fio
 * io-engine. It will be most likely be deprecated in the future.
 */
int
xnvme_znd_report_get_descr(const struct xnvme_znd_report *report, uint32_t index,
			   struct xnvme_spec_znd_descr **descr, void **ext);

#define XNVME_ZND_REPORT_DESCR(rprt, nth) \
	((struct xnvme_spec_znd_descr *)&(rprt->storage[nth * rprt->zrent_nbytes]))

/**
 * Prints the given ::xnvme_znd_report to the given stream
 *
 * @param stream output stream used for printing
 * @param report pointer to the the ::xnvme_znd_report to print
 * @param flags
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_znd_report_fpr(FILE *stream, const struct xnvme_znd_report *report, int flags);

/**
 * Prints the given ::xnvme_znd_report to stdout
 *
 * @param report
 * @param flags
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_znd_report_pr(const struct xnvme_znd_report *report, int flags);

/**
 * Retrieves a Zone Report from the namespace associated with the given `dev`,
 * reporting starting at the given `slba`, and limited to `limit` entries
 *
 * @note
 * Caller is responsible for de-allocating the returned structure using
 * xnvme_buf_virt_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param slba LBA of the first zone in the report
 * @param limit when 0 then provide a report for all zones start from slba.
 * Otherwise, provide a report for [slba, slba+limit]
 * @param extended When 0, the "regular" report is provided. When 1, then the
 * Extended Report is provided, if supported by device.
 *
 * @return On success, pointer to zoned report. On error, NULL is returned and
 * `errno` is set to indicate the error
 */
struct xnvme_znd_report *
xnvme_znd_report_from_dev(struct xnvme_dev *dev, uint64_t slba, size_t limit, uint8_t extended);

/**
 * Scan the 'report' for a zone in the given 'state' and store it in 'zlba'
 *
 * @param report The report to scan
 * @param state The zone-state to scan for
 * @param zlba Pointer to store zlba to
 * @param opts Optional seed random-number generator, when 0 then time(NULL) is
 * used
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_znd_report_find_arbitrary(const struct xnvme_znd_report *report,
				enum xnvme_spec_znd_state state, uint64_t *zlba, int opts);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_ZND_H */
