#ifndef __LIBXNVME_PP_H
#define __LIBXNVME_PP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

/**
 * Options for pretty-printer (``*_pr``, ``*_fpr``) functions
 *
 * @see libxnvme_pp.h
 *
 * @enum xnvme_pr
 */
enum xnvme_pr {
	XNVME_PR_DEF = 0x0,	///< XNVME_PR_DEF: Default options
	XNVME_PR_YAML = 0x1,	///< XNVME_PR_YAML: Print formatted as YAML
	XNVME_PR_TERSE = 0x2	///< XNVME_PR_TERSE: Print without formatting
};

/**
 * Prints the given backend attribute to the given output stream
 *
 * @param stream output stream used for printing
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_fpr(FILE *stream, const struct xnvme_be_attr *attr, int opts);

/**
 * Prints the given backend attribute to stdout
 *
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts);

/**
 * Prints the given backend attribute list to the given output stream
 *
 * @param stream output stream used for printing
 * @param list Pointer to the backend attribute list to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_list_fpr(FILE *stream, const struct xnvme_be_attr_list *list,
		       int opts);

/**
 * Prints the given backend attribute list to standard out
 *
 * @param list Pointer to the backend attribute list to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_list_pr(const struct xnvme_be_attr_list *list, int opts);

/**
 * Prints the given LBA to the given output stream
 *
 * @param stream output stream used for printing
 * @param lba the LBA to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_fpr(FILE *stream, uint64_t lba, enum xnvme_pr opts);

/**
 * Prints the given Logical Block Addresses (LBA) to stdout
 *
 * @param lba the LBA to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_pr(uint64_t lba, enum xnvme_pr opts);

/**
 * Prints the given list of Logical Block Addresses (LBAs)to the given output
 * stream
 *
 * @param stream output stream used for printing
 * @param lba Pointer to an array of LBAs to print
 * @param nlb Number of LBAs to print from the given list
 * @param opts printer options, see ::xnvme_pr
 */
int
xnvme_lba_fprn(FILE *stream, const uint64_t *lba, uint16_t nlb,
	       enum xnvme_pr opts);

/**
 * Print a list of Logical Block Addresses (LBAs)
 *
 * @param lba Pointer to an array of LBAs to print
 * @param nlb Length of the array in items
 * @param opts Printer options
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_lba_prn(const uint64_t *lba, uint16_t nlb, enum xnvme_pr opts);

/**
 * Prints the given ::xnvme_ident to the given output stream
 *
 * @param stream output stream used for printing
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_fpr(FILE *stream, const struct xnvme_ident *ident, int opts);

/**
 * Prints the given ::xnvme_ident to stdout
 *
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_pr(const struct xnvme_ident *ident, int opts);

/**
 * Prints the given ::xnvme_enumeration to the given output stream
 *
 * @param stream output stream used for printing
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_enumeration_fpr(FILE *stream, struct xnvme_enumeration *list, int opts);

int
xnvme_enumeration_fpp(FILE *stream, struct xnvme_enumeration *list, int opts);

int
xnvme_enumeration_pp(struct xnvme_enumeration *list, int opts);

/**
 * Prints the given ::xnvme_enumeration to stdout
 *
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_enumeration_pr(struct xnvme_enumeration *list, int opts);

/**
 * Prints the given ::xnvme_dev to the given output stream
 *
 * @param stream output stream used for printing
 * @param dev pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_dev_fpr(FILE *stream, const struct xnvme_dev *dev, int opts);

/**
 * Prints the given ::xnvme_dev to stdout
 *
 * @param dev pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_dev_pr(const struct xnvme_dev *dev, int opts);

/**
 * Prints the given ::xnvme_geo to the given output stream
 *
 * @param stream output stream used for printing
 * @param geo pointer to the the ::xnvme_geo to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_geo_fpr(FILE *stream, const struct xnvme_geo *geo, int opts);

/**
 * Prints the given ::xnvme_geo to stdout
 *
 * @param geo pointer to the the ::xnvme_geo to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_geo_pr(const struct xnvme_geo *geo, int opts);

/**
 * Prints a humanly readable representation the given #xnvme_cmd_ctx
 *
 * @param ctx Pointer to the #xnvme_cmd_ctx to print
 * @param UNUSED_opts Printer options
 */
void
xnvme_cmd_ctx_pr(const struct xnvme_cmd_ctx *ctx, int UNUSED_opts);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_PP_H */
