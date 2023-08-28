// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_REGISTRY_H
#define __INTERNAL_XNVME_BE_REGISTRY_H

extern struct xnvme_be xnvme_be_spdk;
extern struct xnvme_be xnvme_be_linux;
extern struct xnvme_be xnvme_be_fbsd;
extern struct xnvme_be xnvme_be_macos;
extern struct xnvme_be xnvme_be_macos_driverkit;
extern struct xnvme_be xnvme_be_ramdisk;
extern struct xnvme_be xnvme_be_windows;
extern struct xnvme_be xnvme_be_vfio;

#endif /* __INTERNAL_XNVME_BE_REGISTRY_H */
