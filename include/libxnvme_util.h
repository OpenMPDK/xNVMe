/**
 * libxnvme_util: timers, string-formats, debug-printers and other utilities
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvme_util.h
 */
#ifndef __LIBXNVME_UTIL_H
#define __LIBXNVME_UTIL_H
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#define XNVME_UNIVERSAL_SECT_SH 9

/**
 * Macro wrapping C11 static assert, currently experiencing issues with the
 * asserts on FreeBSD, so trying this...
 */
#ifdef static_assert
#define XNVME_STATIC_ASSERT(cond, msg) static_assert(cond, msg);
#else
#define XNVME_STATIC_ASSERT(cond, msg)
#endif

/**
 * Macro to suppress warnings on unused arguments, thanks to stackoverflow.
 */
#ifdef __GNUC__
#define XNVME_UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define XNVME_UNUSED(x) UNUSED_ ## x
#endif

#define XNVME_I64_FMT \
	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"\
	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define XNVME_I64_TO_STR(val) \
	(val & (1ULL << 63) ? '1' : '0'), \
	(val & (1ULL << 62) ? '1' : '0'), \
	(val & (1ULL << 61) ? '1' : '0'), \
	(val & (1ULL << 60) ? '1' : '0'), \
	(val & (1ULL << 59) ? '1' : '0'), \
	(val & (1ULL << 58) ? '1' : '0'), \
	(val & (1ULL << 57) ? '1' : '0'), \
	(val & (1ULL << 56) ? '1' : '0'), \
	(val & (1ULL << 55) ? '1' : '0'), \
	(val & (1ULL << 54) ? '1' : '0'), \
	(val & (1ULL << 53) ? '1' : '0'), \
	(val & (1ULL << 52) ? '1' : '0'), \
	(val & (1ULL << 51) ? '1' : '0'), \
	(val & (1ULL << 50) ? '1' : '0'), \
	(val & (1ULL << 49) ? '1' : '0'), \
	(val & (1ULL << 48) ? '1' : '0'), \
	(val & (1ULL << 47) ? '1' : '0'), \
	(val & (1ULL << 46) ? '1' : '0'), \
	(val & (1ULL << 45) ? '1' : '0'), \
	(val & (1ULL << 44) ? '1' : '0'), \
	(val & (1ULL << 43) ? '1' : '0'), \
	(val & (1ULL << 42) ? '1' : '0'), \
	(val & (1ULL << 41) ? '1' : '0'), \
	(val & (1ULL << 40) ? '1' : '0'), \
	(val & (1ULL << 39) ? '1' : '0'), \
	(val & (1ULL << 38) ? '1' : '0'), \
	(val & (1ULL << 37) ? '1' : '0'), \
	(val & (1ULL << 36) ? '1' : '0'), \
	(val & (1ULL << 35) ? '1' : '0'), \
	(val & (1ULL << 34) ? '1' : '0'), \
	(val & (1ULL << 33) ? '1' : '0'), \
	(val & (1ULL << 32) ? '1' : '0'), \
	(val & (1ULL << 31) ? '1' : '0'), \
	(val & (1ULL << 30) ? '1' : '0'), \
	(val & (1ULL << 29) ? '1' : '0'), \
	(val & (1ULL << 28) ? '1' : '0'), \
	(val & (1ULL << 27) ? '1' : '0'), \
	(val & (1ULL << 26) ? '1' : '0'), \
	(val & (1ULL << 25) ? '1' : '0'), \
	(val & (1ULL << 24) ? '1' : '0'), \
	(val & (1ULL << 23) ? '1' : '0'), \
	(val & (1ULL << 22) ? '1' : '0'), \
	(val & (1ULL << 21) ? '1' : '0'), \
	(val & (1ULL << 20) ? '1' : '0'), \
	(val & (1ULL << 19) ? '1' : '0'), \
	(val & (1ULL << 18) ? '1' : '0'), \
	(val & (1ULL << 17) ? '1' : '0'), \
	(val & (1ULL << 16) ? '1' : '0'), \
	(val & (1ULL << 15) ? '1' : '0'), \
	(val & (1ULL << 14) ? '1' : '0'), \
	(val & (1ULL << 13) ? '1' : '0'), \
	(val & (1ULL << 12) ? '1' : '0'), \
	(val & (1ULL << 11) ? '1' : '0'), \
	(val & (1ULL << 10) ? '1' : '0'), \
	(val & (1ULL << 9) ? '1' : '0'), \
	(val & (1ULL << 8) ? '1' : '0'), \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

#define XNVME_I32_FMT \
	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c" \
	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define XNVME_I32_TO_STR(val) \
	(val & (1ULL << 31) ? '1' : '0'), \
	(val & (1ULL << 30) ? '1' : '0'), \
	(val & (1ULL << 29) ? '1' : '0'), \
	(val & (1ULL << 28) ? '1' : '0'), \
	(val & (1ULL << 27) ? '1' : '0'), \
	(val & (1ULL << 26) ? '1' : '0'), \
	(val & (1ULL << 25) ? '1' : '0'), \
	(val & (1ULL << 24) ? '1' : '0'), \
	(val & (1ULL << 23) ? '1' : '0'), \
	(val & (1ULL << 22) ? '1' : '0'), \
	(val & (1ULL << 21) ? '1' : '0'), \
	(val & (1ULL << 20) ? '1' : '0'), \
	(val & (1ULL << 19) ? '1' : '0'), \
	(val & (1ULL << 18) ? '1' : '0'), \
	(val & (1ULL << 17) ? '1' : '0'), \
	(val & (1ULL << 16) ? '1' : '0'), \
	(val & (1ULL << 15) ? '1' : '0'), \
	(val & (1ULL << 14) ? '1' : '0'), \
	(val & (1ULL << 13) ? '1' : '0'), \
	(val & (1ULL << 12) ? '1' : '0'), \
	(val & (1ULL << 11) ? '1' : '0'), \
	(val & (1ULL << 10) ? '1' : '0'), \
	(val & (1ULL << 9) ? '1' : '0'), \
	(val & (1ULL << 8) ? '1' : '0'), \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

#define XNVME_I16_FMT	"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"

#define XNVME_I16_TO_STR(val) \
	(val & (1ULL << 15) ? '1' : '0'), \
	(val & (1ULL << 14) ? '1' : '0'), \
	(val & (1ULL << 13) ? '1' : '0'), \
	(val & (1ULL << 12) ? '1' : '0'), \
	(val & (1ULL << 11) ? '1' : '0'), \
	(val & (1ULL << 10) ? '1' : '0'), \
	(val & (1ULL << 9) ? '1' : '0'), \
	(val & (1ULL << 8) ? '1' : '0'), \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

#define XNVME_I8_FMT	"%c%c%c%c%c%c%c%c"

#define XNVME_I8_TO_STR(val) \
	(val & (1ULL << 7) ? '1' : '0'), \
	(val & (1ULL << 6) ? '1' : '0'), \
	(val & (1ULL << 5) ? '1' : '0'), \
	(val & (1ULL << 4) ? '1' : '0'), \
	(val & (1ULL << 3) ? '1' : '0'), \
	(val & (1ULL << 2) ? '1' : '0'), \
	(val & (1ULL << 1) ? '1' : '0'), \
	(val & (1ULL << 0) ? '1' : '0')

static inline uint64_t
XNVME_ILOG2(uint64_t x)
{
	uint64_t val = 0;

	while (x >>= 1) {
		val++;
	}

	return val;
}

/**
* Calculate the minimum of the given `x` and `y`
*
* @param x
* @param y
* @return The maximum of `x` and `y`
*/
static inline int
XNVME_MIN(int x, int y)
{
	return x < y ? x : y;
}

/**
* Calculate the minimum of the given `x` and `y`
*
* @param x
* @param y
* @return The maximum of `x` and `y`
*/
static inline uint64_t
XNVME_MIN_U64(uint64_t x, uint64_t y)
{
	return x < y ? x : y;
}

/**
* Calculate the minimum of the given `x` and `y`
*
* @param x
* @param y
* @return The maximum of `x` and `y`
*/
static inline int64_t
XNVME_MIN_S64(int64_t x, int64_t y)
{
	return x < y ? x : y;
}

/**
 * Calculate the maximum of the given `x` and `y`
 *
 * @param x
 * @param y
 * @return The maximum of `x` and `y`
 */
static inline int
XNVME_MAX(int x, int y)
{
	return x > y ? x : y;
}

static inline uint64_t
_xnvme_timer_clock_sample(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_nsec + ts.tv_sec * 1e9;
}

/**
 * Encapsulation of a basic wall-clock timer, start/stop clock
 *
 * @struct xnvme_timer
 */
struct xnvme_timer {
	uint64_t start;
	uint64_t stop;
};

/**
 * Start the given timer
 *
 * @param t
 * @return clock sample in nano seconds
 */
static inline uint64_t
xnvme_timer_start(struct xnvme_timer *t)
{
	t->start = _xnvme_timer_clock_sample();
	return t->start;
}

/**
 * Stop the given timer
 *
 * @param t
 * @return clock sample in nano seconds
 */
static inline uint64_t
xnvme_timer_stop(struct xnvme_timer *t)
{
	t->stop = _xnvme_timer_clock_sample();
	return t->stop;
}

/**
 * Get the elapsed time in seconds
 *
 * @param t
 * @return elapsed time in seconds as a floating point number
 */
static inline double
xnvme_timer_elapsed_secs(struct xnvme_timer *t)
{
	return (t->stop - t->start) / (double)1e9;
}

/**
 * Get the elapsed time in seconds
 *
 * @param t
 * @return elapsed time in seconds as a floating point number
 */
static inline double
xnvme_timer_elapsed(struct xnvme_timer *t)
{
	return xnvme_timer_elapsed_secs(t);
}

/**
 * Get the elapsed time in milliseconds
 *
 * @param t
 * @return elapsed time in milliseconds as a floating point number
 */
static inline double
xnvme_timer_elapsed_msecs(struct xnvme_timer *t)
{
	return (t->stop - t->start) / (double)1e6;
}

/**
 * Get the elapsed time in microseconds
 *
 * @param t
 * @return elapsed time in microseconds as a floating point number
 */
static inline double
xnvme_timer_elapsed_usecs(struct xnvme_timer *t)
{
	return (t->stop - t->start) / (double)1e3;
}

/**
 * Get the elapsed time in nanoseconds
 *
 * @param t
 * @return elapsed time in nanoseconds
 */
static inline uint64_t
xnvme_timer_elapsed_nsecs(struct xnvme_timer *t)
{
	return t->stop - t->start;
}

/**
 * Print the elapsed time in seconds as a floating point number
 *
 * @param t
 * @param prefix
 */
static inline void
xnvme_timer_pr(struct xnvme_timer *t, const char *prefix)
{
	printf("%s: {elapsed: %lf}\n", prefix, xnvme_timer_elapsed(t));
}

/**
 * Print the elapsed time in seconds and the associated data rate in MB/s
 *
 * @param t
 * @param prefix
 * @param nbytes
 */
static inline void
xnvme_timer_bw_pr(struct xnvme_timer *t, const char *prefix, size_t nbytes)
{
	double secs = xnvme_timer_elapsed_secs(t);
	double mb = nbytes / (double)1048576;

	printf("%s: {elapsed: %.4f, mb: %.2f, mbsec: %.2f}\n",
	       prefix, secs, mb, mb / secs);
}

static inline int
xnvme_is_pow2(uint32_t val)
{
	return val && !(val & (val - 1));
}

#ifdef XNVME_DEBUG_ENABLED

#define XNVME_DEBUG_FCALL(x) x

#define __FILENAME__ strrchr("/" __FILE__, '/') + 1

#define XNVME_DEBUG(...) \
	fprintf(stderr, "# DBG:%s:%s-%d: " FIRST(__VA_ARGS__) "\n" , \
		__FILENAME__, __func__, __LINE__ REST(__VA_ARGS__)); \
	fflush(stderr);

#define FIRST(...) FIRST_HELPER(__VA_ARGS__, throwaway)
#define FIRST_HELPER(first, ...) first

#define REST(...) REST_HELPER(NUM(__VA_ARGS__), __VA_ARGS__)
#define REST_HELPER(qty, ...) REST_HELPER2(qty, __VA_ARGS__)
#define REST_HELPER2(qty, ...) REST_HELPER_##qty(__VA_ARGS__)
#define REST_HELPER_ONE(first)
#define REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define NUM(...) \
	SELECT_10TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE,\
		    TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define SELECT_10TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10

#else
#define XNVME_DEBUG(...)
#define XNVME_DEBUG_FCALL(x)
#endif

#endif /* __LIBXNVME_UTIL_H */
