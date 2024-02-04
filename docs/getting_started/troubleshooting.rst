.. _sec-gs-troubleshooting:

Troubleshooting
===============

User space
----------

In case you are having issues using SPDK backend then make sure you are
following the config section :ref:`sec-gs-system-config-userspace-config` and
if issues persist a solution might be found in the following subsections.

No devices found
~~~~~~~~~~~~~~~~

When running ``xnvme enum`` and the output-listing is empty, then there are no
devices. When running with ``vfio-pci``, this can occur when your devices are
sharing iommu-group with other devices which are still bound to in-kernel
drivers. This could be NICs, GPUs or other kinds of peripherals.

The division of devices into groups is not something that can be easily
switched, but you can try to manually unbind the other devices in the iommu
group from their kernel drivers.

If that is not an option then you can try to re-organize your physical
connectivity of deviecs, e.g. move devices around.

Lastly you can try using ``uio_pci_generic`` instead, this can most easily be
done by disabling iommu by adding the kernel option: ``iommu=off`` to the
kernel command-line and rebooting.

Memory Issues
~~~~~~~~~~~~~

If you see a message similar to the below while unbinding devices::

  Current user memlock limit: 16 MB

  This is the maximum amount of memory you will be
  able to use with DPDK and VFIO if run as current user.
  To change this, please adjust limits.conf memlock limit for current user.

  ## WARNING: memlock limit is less than 64MB
  ## DPDK with VFIO may not be able to initialize if run as current user.

Then go you should do as suggested, that is, adjust ``limits.conf``, for an
example, see :ref:`sec-gs-system-config-userspace-config`.

Build Errors
------------

If you are getting errors while attempting to configure and build **xNVMe**
then it is likely due to one of the following:

* You are building in an **offline** environment and only have a shallow
  source-archive or a git-repository without subprojects.

The full source-archive is made available with each release and downloadable
from the `GitHUB Release page <https://github.com/OpenMPDK/xNVMe/releases>`_
release page. It contains the xNVMe source code along with all the third-party
dependencies, namely: SPDK, liburing, libnvme, and fio.

**missing dependencies / toolchain**

* You are missing dependencies, see the :ref:`sec-building-toolchain` for
  installing these on FreeBSD and a handful of different Linux Distributions

The :ref:`sec-building-toolchain` section describes preferred ways of
installing libraries and tools. For example, on Ubuntu 18.04 it is preferred to
install ``meson`` via ``pip3`` since the version in the package registry is too
old for SPDK, thus if installed via the package manager then you will
experience build errors as the xNVMe build system starts building SPDK.

Once you have the full source of xNVMe, third-party library dependencies, and
setup the toolchain then run the following to ensure that the xNVMe repository
is clean from any artifacts left behind by previous build failures::

  make clobber

And then go back to the :ref:`sec-building` and follow the steps there.

.. note::
   When running ``make clobber`` then everything not comitted is "lost". Thus,
   if you are developing/modifying xNVMe, then make you commit of stash your
   changes before running it.

Known Build Issues
------------------

If the above did not sort out your build-issues, then you might be facing one
of the following known build-issues. If these do not apply to you, then please
post an issue on GitHUB describing your build environment and output from the
failed build.

When building **xNVMe** on **Alpine Linux** you might encounter some issues due
to musl standard library not being entirely compatible with **GLIBC** /
**BSD**.

The SPDK backend does not build on due to re-definition of ``STAILQ_*``
macros. As a work-around, then disable the SPDK backend::

  meson setup builddir -Dwith-spdk=false

The Linux backend support for ``io_uring`` fails on **Alpine Linux** due to a
missing definition in **musl** leading to this error message::

  include/liburing.h:195:17: error: unknown type name 'loff_t'; did you mean
  'off_t'?

As a work-around, then disable ``io_uring`` support::

  meson setup builddir -Dwith-liburing=false

See more details on changing the default build-configuration of **xNVMe** in
the section :ref:`sec-building-config`.
