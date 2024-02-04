.. _sec-gs-system-config:

Backends and System Config
==========================

**xNVMe** relies on certain Operating System Kernel features and infrastructure
that must be available and correctly configured. This subsection goes through
what is used on Linux and how check whether is it available.

Backends
--------

The purpose of **xNVMe** backends are to provide an instrumental runtime
supporting the **xNVMe** API in a single library with **batteries included**.

That is, it comes with the essential third-party libraries bundled into the
**xNVMe** library. Thus, you get a single C API to program against and a single
library to link with. And similarly for the command-line tools; a single binary
to communicating with devices via the I/O stacks that available on the system.

To inspect the libraries which **xNVMe** is build against and the
supported/enabled backends then invoke:

.. literalinclude:: xnvme_library-info.cmd
   :language: bash

It should produce output similar to:

.. literalinclude:: xnvme_library-info.out
   :language: bash

The ``xnvme_3p`` part of the output informs about the third-party projects
which **xNVMe** was built against, and in the case of libraries, the version it
has bundled.

Although a single API and a single library is provided by **xNVMe**, then
runtime and system configuration dependencies remain. The following subsections
describe how to instrument **xNVMe** to utilize the different kernel interfaces
and user space drivers.

Kernel
------

Linux Kernel version 5.9 or newer is currently preferred as it has all the
features which **xNVMe** utilizes. This section also gives you a brief overview
of the different I/O paths and APIs which the **xNVMe** API unifies access to.

NVMe Driver and IOCTLs
~~~~~~~~~~~~~~~~~~~~~~

The default for **xNVMe** is to communicate with devices via the operating
system NVMe driver IOCTLs, specifically on Linux the following are used:

* ``NVME_IOCTL_ID``
* ``NVME_IOCTL_IO_CMD``
* ``NVME_IOCTL_ADMIN_CMD``
* ``NVME_IOCTL_IO64_CMD``
* ``NVME_IOCTL_ADMIN64_CMD``

In case the ``*64_CMD`` IOCTLs are not available then **xNVMe** falls back to
using the non-64bit equivalents. The 64 vs 32 completion result mostly affect
commands such as Zone Append. You can check that this interface is behaving as
expected by running:

.. literalinclude:: xnvme_info_default.cmd
   :language: bash

Which you yield output equivalent to:

.. literalinclude:: xnvme_info_default.out
   :language: bash
   :lines: 1-10

This tells you that **xNVMe** can communicate with the given device identifier
and it informs you that it utilizes **nvme_ioctl** for synchronous command
execution and it uses **thr** for asynchronous command execution.  Since IOCTLs
are inherently synchronous then **xNVMe** mimics asynchronous behavior over
IOCTLs to support the asynchronous primitives provided by the **xNVMe** API.

Block Layer
~~~~~~~~~~~

In case your device is **not** an NVMe device, then the NVMe IOCTLs won't be
available. **xNVMe** will then try to utilize the Linux Block Layer and treat
a given block device as a NVMe device via shim-layer for NVMe admin commands
such as identify and get-features.

A brief example of checking this:

.. literalinclude:: xnvme_info_block.cmd
   :language: bash

Yielding:

.. literalinclude:: xnvme_info_block.out
   :language: bash

Block Zoned IOCTLs
~~~~~~~~~~~~~~~~~~

Building on the Linux Block model, then the Zoned Block Device model is also
utilized, specifically the following IOCTLs:

* ``BLK_ZONE_REP_CAPACITY``
* ``BLKCLOSEZONE``
* ``BLKFINISHZONE``
* ``BLKOPENZONE``
* ``BLKRESETZONE``
* ``BLKGETNRZONES``
* ``BLKREPORTZONE``

When available, then **xNVMe** can make use of the above IOCTLs. This is
mostly useful when developing/testing using Linux Null Block devices.
And similar for a Zoned NULL Block instance:

.. literalinclude:: xnvme_info_zoned.cmd
   :language: bash

Yielding:

.. literalinclude:: xnvme_info_zoned.out
   :language: bash

Async I/O via ``libaio``
~~~~~~~~~~~~~~~~~~~~~~~~

When AIO is available then the NVMe NVM Commands for **read** and **write** are
sent over the Linux AIO interface. Doing so improves command-throughput at
higher queue-depths when compared to sending the command over via the NVMe
driver ioctl().

One can explicitly tell **xNVMe** to utilize ``libaio`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_io_async_read_libaio.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_io_async_read_libaio.out
   :language: bash
   :lines: 1-12

Async. I/O via ``io_uring``
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**xNVMe** utilizes the Linux **io_uring** interface, its support for
feature-probing the **io_uring** interface and the **io_uring** opcodes:

* ``IORING_OP_READ``
* ``IORING_OP_WRITE``

When available, then **xNVMe** can send the NVMe NVM Commands for **read** and
**write** via the Linux **io_uring** interface. Doing so improves
command-throughput at all io-depths when compared to sending the command via
NVMe Driver IOCTLs and libaio. It also leverages the **io_uring** interface to
enabling I/O polling and kernel-side submission polling.

One can explicitly tell **xNVMe** to utilize ``io_uring`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_io_async_read_io_uring.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_io_async_read_io_uring.out
   :language: bash
   :lines: 1-12

.. _sec-gs-system-config-userspace:

User Space
----------

Linux provides the **Userspace I/O** (`uio`_) and **Virtual Function I/O**
`vfio`_ frameworks to write user space I/O drivers. Both interfaces work by
binding a given device to an in-kernel stub-driver. The stub-driver in turn
exposes device-memory and device-interrupts to user space. Thus enabling the
implementation of device drivers entirely in user space.

Although Linux provides a capable NVMe Driver with flexible IOCTLs, then a user
space NVMe driver serves those who seek the lowest possible per-command
processing overhead or wants full control over NVMe command construction,
including command-payloads.

Fortunately, you do not need to go and write an user space NVMe driver since a
highly efficient, mature and well-maintained driver already exists. Namely, the
NVMe driver provided by the **Storage Platform Development Kit** (_`SPDK`).

Another great fortune is that **xNVMe** bundles the SPDK NVMe Driver with the
**xNVMe** library. So, if you have built and installed **xNVMe** then the
**SPDK** NVMe Driver is readily available to **xNVMe**.

The following subsections goes through a configuration checklist, then shows
how to bind and unbind drivers, and lastly how to utilize non-devfs device
identifiers by enumerating the system and inspecting a device.

.. _sec-gs-system-config-userspace-config:

Config
~~~~~~

What remains is checking your system configuration, enabling IOMMU for use by
the ``vfio-pci`` driver, and possibly falling back to the ``uio_pci_generic``
driver in case ``vfio-pci`` is not working out.  ``vfio`` is preferred as
hardware support for IOMMU allows for isolation between devices.

1) Verify that your CPU supports virtualization / VT-d and that it is enabled
   in your board BIOS.

2) Enable your kernel for an intel CPU then provide the kernel option
   ``intel_iommu=on``.  If you have a non-Intel CPU then consult documentation
   on enabling VT-d / IOMMU for your CPU.

3) Increase limits, open ``/etc/security/limits.conf`` and add:

::

  *    soft memlock unlimited
  *    hard memlock unlimited
  root soft memlock unlimited
  root hard memlock unlimited

Once you have gone through these steps, and rebooted, then this command:

.. literalinclude:: 200_dmesg.cmd
   :language: bash

Should output:

.. literalinclude:: 200_dmesg.out
   :language: bash

And this command:

.. literalinclude:: 300_find.cmd
   :language: bash

Should have output similar to:

.. literalinclude:: 300_find.out
   :language: bash

Unbinding and binding
~~~~~~~~~~~~~~~~~~~~~

With the system configured then you can use the ``xnvme-driver`` script to bind
and unbind devices. The ``xnvme-driver`` script is a merge of the **SPDK**
``setup.sh`` script and its dependencies.

By running the command below **8GB** of hugepages will be configured, the
Kernel NVMe driver unbound, and ``vfio-pci`` bound to the device:

.. literalinclude:: 010_xnvme_driver.cmd
   :language: bash

The command above should produce output similar to:

.. literalinclude:: 010_xnvme_driver.out
   :language: bash

To unbind from ``vfio-pci`` and back to the Kernel NVMe driver, then run:

.. literalinclude:: 500_xnvme_driver_reset.cmd
   :language: bash

Should output similar to:

.. literalinclude:: 500_xnvme_driver_reset.out
   :language: bash

.. _sec-device-identifiers:

Device Identifiers
~~~~~~~~~~~~~~~~~~

Since the Kernel NVMe driver is unbound from the device, then the kernel no
longer know that the PCIe device is an NVMe device, thus, it no longer lives in
Linux devfs, that is, no longer available in ``/dev`` as e.g. ``/dev/nvme0n1``.

Instead of the filepath in devfs, then you use PCI ids and xNVMe options.

As always, use the ``xnvme`` cli tool to enumerate devices:

.. literalinclude:: 400_xnvme_enum.cmd
   :language: bash

.. literalinclude:: 400_xnvme_enum.out
   :language: bash

Notice that multiple URIs using the same PCI id but with different **xNVMe**
``?opts=<val>``. This is provided as a means to tell **xNVMe** that you want to
use the NVMe controller at ``0000:03:00.0`` and the namespace identified by
``nsid=1``.

.. literalinclude:: 410_xnvme_info.cmd
   :language: bash

.. literalinclude:: 410_xnvme_info.out
   :language: bash


Similarly, when using the API, then you would use these URIs instead of
filepaths::

  ...
  struct xnvme_dev *dev = xnvme_dev_open("pci:0000:01:00.0?nsid=1");
  ...

Windows Kernel
--------------

Windows 10 or later version is currently preferred as it has all the features
which **xNVMe** utilizes. This section also gives you a brief overview of the
different I/O paths and APIs which the **xNVMe** API unifies access to.

NVMe Driver and IOCTLs
~~~~~~~~~~~~~~~~~~~~~~

The default for **xNVMe** is to communicate with devices via the operating
system NVMe driver IOCTLs, specifically on Windows the following are used:

* ``IOCTL_STORAGE_QUERY_PROPERTY``
* ``IOCTL_STORAGE_SET_PROPERTY``
* ``IOCTL_STORAGE_REINITIALIZE_MEDIA``
* ``IOCTL_SCSI_PASS_THROUGH_DIRECT``

You can check that this interface is behaving as expected by running:

.. literalinclude:: xnvme_win_info_default.cmd
   :language: bash

Which should yield output equivalent to:

.. literalinclude:: xnvme_win_info_default.out
   :language: bash
   :lines: 1-12

This tells you that **xNVMe** can communicate with the given device identifier
and it informs you that it utilizes **nvme_ioctl** for synchronous command
execution and it uses **iocp** for asynchronous command execution. This method
can be used for raw devices via **\\.\PhysicalDrive<disk number>** device path.

Below mentioned commands are currently supported by **xNVMe** using IOCTL path:

* ``Admin Commands``
   * ``Get Log Page``
   * ``Identify``
   * ``Get Feature``
   * ``Format NVM``

* ``I/O Commands``
   * ``Read``
   * ``Write``

NVMe Driver and Regular File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**xNVMe** can communicate with File System mounted devices via the operating
system generic APIs like **ReadFile** and **WriteFile** operations. This method
can be used to do operation on Regular Files.

You can check that this interface is behaving as expected by running:

.. literalinclude:: xnvme_win_info_fs.cmd
   :language: bash

Which should yield output equivalent to:

.. literalinclude:: xnvme_win_info_fs.out
   :language: bash
   :lines: 1-12

This tells you that **xNVMe** can communicate with the given regular file
and it informs you that it utilizes **nvme_ioctl** for synchronous command
execution and it uses **iocp** for asynchronous command execution. This method
can be used for file operations via **<driver name>:\<file name>** path.

Async I/O via ``iocp``
~~~~~~~~~~~~~~~~~~~~~~

When AIO is available then the NVMe NVM Commands for **read** and **write** are
sent over the Windows IOCP interface. Doing so improves command-throughput at
higher queue-depths when compared to sending the command via the NVMe driver
ioctl().

One can explicitly tell **xNVMe** to utilize ``iocp`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_win_io_async_read_iocp.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_win_io_async_read_iocp.out
   :language: bash


Async I/O via ``iocp_th``
~~~~~~~~~~~~~~~~~~~~~~~~~

Similar to ``iocp`` interface, only difference is separate poller is used to
fetch the completed IOs.

One can explicitly tell **xNVMe** to utilize ``iocp_th`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_win_io_async_read_iocp_th.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_win_io_async_read_iocp_th.out
   :language: bash

Async I/O via ``io_ring``
~~~~~~~~~~~~~~~~~~~~~~~~~

**xNVMe** utilizes the Windows **io_ring** interface, its support for
feature-probing the **io_ring** interface and the **io_ring** opcodes:

When available, then **xNVMe** can send the **io_ring** specific request using
**IORING_HANDLE_REF** and **IORING_BUFFER_REF** structure for **read** and
**write** via Windows **io_ring** interface. Doing so improves command-throughput
at all io-depths when compared to sending the command via NVMe Driver IOCTLs.

One can explicitly tell **xNVMe** to utilize ``io_ring`` for async I/O by
encoding it in the device identifier, like so:

.. literalinclude:: xnvme_win_io_async_read_io_ring.cmd
   :language: bash

Yielding the output:

.. literalinclude:: xnvme_win_io_async_read_io_ring.out
   :language: bash
   :lines: 1-12


Building SPDK backend on Windows
--------------------------------

**SPDK** can be used as a backend for **xNVMe**, to leverage this interface
first user need to compile the **SPDK** as a subproject with **xNVMe** its
depend on Windows Platform Development Kit (WPDK) that help to enables
applications based on the Storage Performance Development Kit (SPDK) to
build and run as native Windows executables.

Prerequisite’s to compile the ``SPDK``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Refer below mentioned script and link to install the dependencies packages,

* MinGW cross compiler and libraries
   this can be installed by running below script,

   .. literalinclude:: ../../toolbox/pkgs/windows-2022.bat
      :language: batch

* `Windows Subsystem of Linux (WSL)`_

.. Windows Subsystem of Linux (WSL): https://github.com/wpdk/wpdk/blob/master/doc/wsl.md

Compilation and Installation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Configure, build and install using helper batch script ``build.bat``:

.. literalinclude:: build_windows.rst
   :language: bash
   :lines: 2-

Runtime Prerequisite’s
~~~~~~~~~~~~~~~~~~~~~~

Refer below mentioned DPDK links to resolve runtime dependencies,

* `Grant Lock pages in memory Privilege`_

.. Grant Lock pages in memory Privilege: https://doc.dpdk.org/guides/windows_gsg/run_apps.html#grant-lock-pages-in-memory-privilege

* `Download DPDK kmods-driver`_

.. DPDK kmods-driver: https://git.dpdk.org/dpdk-kmods

* `Install virt2phys`_

.. Install virt2phys: https://doc.dpdk.org/guides/windows_gsg/run_apps.html#virt2phys

* `Install NetUIO`_

.. Install NetUIO: https://doc.dpdk.org/guides/windows_gsg/run_apps.html#NetUIO

