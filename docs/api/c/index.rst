.. _sec-api-c:

########
 C API
########

A brief overview of the C API provided by xNVMe follows.

**xnvme**

The :ref:`sec-c-api-xnvme` API is the foundational API of **xNVMe**, providing
device enumeration, memory handling, asynchronous primitives and the
fundamental command-interface for communicating with devices.

The :ref:`sec-c-api-xnvme-header` is available for inspection in its raw form.

**Block (NVM)**

Building on top of the functionality of :ref:`sec-c-api-xnvme` then the
:ref:`sec-c-api-xnvme_nvm` API provides helpers and structures specific to the
NVMe command-set provided by NVM/Logical/Conventional Namespaces. That is the
usual read/write, and optional commands such as the Simple-Copy-Command (SCC).

The :ref:`sec-c-api-xnvme_nvm-header` is available for inspection in its raw form.

**Zoned (ZNS)**

Building on top of the functionality of :ref:`sec-c-api-xnvme` then the
:ref:`sec-c-api-xnvme_znd` API provides helpers and structures specific to NVMe
command-set provided by Zoned Namespaces.

The :ref:`sec-c-api-xnvme_znd-header` is available for inspection in its raw form.

**xnvme_cli**

The :ref:`sec-c-api-xnvme_cli` API provides functionality to create
command-line-interfaces for **xNVMe** related applications. The **xNVMe**
examples, tools, and tests use the library to provide a coherent command-line
interface with bash-completion-scripts and man-pages.

The :ref:`sec-c-api-xnvme_cli-header` is available for inspection in its raw
form.

.. toctree::
   :hidden:

   xnvme
   xnvme_ident
   xnvme_opts
   xnvme_file
   xnvme_cmd
   xnvme_adm
   xnvme_nvm
   xnvme_znd
   xnvme_lba
   xnvme_kvs
   xnvme_dev
   xnvme_geo
   xnvme_buf
   xnvme_mem
   xnvme_queue
   xnvme_spec
   xnvme_libconf
   xnvme_ver
   xnvme_cli
