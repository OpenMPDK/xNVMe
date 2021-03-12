// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_YSNP_H
#define __INTERNAL_XNVME_YSNP_H
#define XNVME_YSNP(func) you_shall_not_pass_##func##_in_this_codebase

#undef strcpy
#define strcpy(x,y) XNVME_YSNP(strcpy)
#undef strcat
#define strcat(x,y) XNVME_YSNP(strcat)
#undef strncpy
#define strncpy(x,y,n) XNVME_YSNP(strncpy)
#undef strncat
#define strncat(x,y,n) XNVME_YSNP(strncat)

#undef sprintf
#undef vsprintf
#ifdef HAVE_VARIADIC_MACROS
#define sprintf(...) XNVME_YSNP(sprintf)
#define vsprintf(...) XNVME_YSNP(vsprintf)
#else
#define sprintf(buf,fmt,arg) XNVME_YSNP(sprintf)
#define vsprintf(buf,fmt,arg) XNVME_YSNP(vsprintf)
#endif

#endif /* XNVME_YSNP_H */
