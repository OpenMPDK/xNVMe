.. SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
..
.. SPDX-License-Identifier: BSD-3-Clause

=============================
 xNVMe third-party directory
=============================

Take a look at ``.reuse/dep5`` as well and produce a SBOM via the
REUSE-software tool::

  reuse spdx

This file and directory contain information about the software on which xNVMe
depend. The licenses and links to project web-pages and sources are provided
and a brief summary of how xNVMe uses the third-party software.

REPOS/toolbox/xnvme-driver.sh
  Summary: This script is based on the setup.sh and its associated scripts from
  SPDK. It is added here in a single-file for easy distribution with xNVMe.

  License: BSD-3-Clause
  https://github.com/spdk/spdk/blob/master/LICENSE

  Repos: https://github.com/spdk/spdk
  Website: https://spdk.io/

SYSTEM:freebsd:/
  Summary: Headers for the FreeBSD Kernel NVMe driver IOCTLs are used by the
  ``be/fbsd``, then headers for backend. These headers are not distributed with
  xNVMe but rather expected to be available on the system where xNVMe is built.

  License: BSD-3-Clause

  Repos: https://svnweb.freebsd.org/
  Website: https://www.freebsd.org/

SYSTEM::linux:/
  Summary: Headers for the Linux Kernel NVMe driver IOCTLs are used by the
  ``be/linux`` backend. These headers are not distributed with xNVMe but rather
  expected to be available on the system where xNVMe is built.

  License: GPL-2.0 WITH Linux-syscall-note
  https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/nvme_ioctl.h?h=v5.3-rc8

  Repos: https://github.com/spdk/spdk
  Website: https://www.linuxfoundation.org/
