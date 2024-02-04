.. _sec-backends:

==========
 Backends
==========

``xNVMe`` hides the implementation of operating system interaction from the
user. That is, the implementation of the ``xnvme_*`` **C API**, is delegated at
runtime to a backend implementation.

Devices are associated with a backend when they are "opened" via a device
identifier on the such as::

  /dev/nvme0n1
  /dev/nvme0ns1
  0000:01:00.0
  \\.\PhysicalDrive1

The library can inform you which backend is in affect, e.g.:

.. literalinclude:: xnvme_be_cli.cmd
   :language: bash

.. literalinclude:: xnvme_be_cli.out
   :language: bash

The valid combinations of interfaces and backends are listed below:

+----------------------------+-------------------------------------------------------------------+
|                            | Backends                                                          |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| Async interfaces           | ``fbsd`` | ``linux`` | libvfn  | ``spdk`` | ``windows`` | ``mac`` |
+============================+==========+===========+=========+==========+=============+=========+
| io_uring                   | no       | **yes**   | no      | no       | no          | no      |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| io_uring_cmd               | no       | **yes**   | no      | no       | no          | no      |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| libaio                     | no       | **yes**   | no      | no       | no          | no      |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| kqueue                     | **yes**  | no        | no      | no       | no          | no      |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| POSIX aio                  | **yes**  | **yes**   | no      | no       | no          | **yes** |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| NVMe Driver (vfio-pci)     | no       | no        | **yes** | **yes**  | no          | no      |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| NVMe Driver (uio-generic)  | no       | no        | no      | **yes**  | no          | no      |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| emu                        | **yes**  | **yes**   | **yes** | **yes**  | **yes**     | **yes** |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| thrpool                    | **yes**  | **yes**   | **yes** | **yes**  | **yes**     | **yes** |
+----------------------------+----------+-----------+---------+----------+-------------+---------+
| nil                        | **yes**  | **yes**   | **yes** | **yes**  | **yes**     | **yes** |
+----------------------------+----------+-----------+---------+----------+-------------+---------+


xNVMe compatibility
=======================================



.. list-table:: xNVMe compatibility of NVMe command table
    :widths: 80 30
    :header-rows: 1

    * - Command
      - Ramdisk
    * - Compare
      - ✗
    * - Copy
      - ✓
    * - Dataset Management
      - ✗
    * - Flush
      - ✗
    * - Verify
      - ✗
    * - Write
      - ✗
    * - Wrote Uncorrectable
      - ✗
    * - Write Zeroes
      - ✗
    * - Read
      - ✗ 

Appendix
--------

[1] http://www.cs.cmu.edu/~riedel/ftp/SIO/API/cmu-cs-96-193.pdf
[2] https://www.nextplatform.com/2017/09/11/whats-bad-posix-io/
[3] https://blog.linuxplumbersconf.org/2009/slides/Anthony-Liguori-qemu-block.pdf

.. toctree::
   :hidden:

   xnvme_be_fbsd
   xnvme_be_linux
   xnvme_be_spdk/index
   xnvme_be_intf
   xnvme_be_windows
