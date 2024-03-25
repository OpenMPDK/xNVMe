.. _sec-c-api-extended:

##########
 Extended
##########

This section describes some options for where to go next after you have gained
familiarity with **xNVMe** and the :ref:`sec-c-api-core` API.

**Different command sets**

If your device has support for more than just the NVM command set, then 
:ref:`sec-c-api-xnvme_kvs` and :ref:`sec-c-api-xnvme_znd` will be of interest. 
These include functions for constructing and submitting commands for their respective command set.

Alternatively, if you want to interact with a file, you can use the API in :ref:`sec-c-api-xnvme_file`.

**Quality of life**

**xNVMe** has a number of header files for making programming NVMe easier to manage,
some of these are described below.

:ref:`sec-c-api-xnvme_geo` and :ref:`sec-c-api-xnvme_ident` include structs
to make accessing useful information about the namespace (or controller) easier. 
``xnvme_geo`` includes the maximum data transfer size (``mdts_nbytes``), the size of a logical block
address (``lba_nbytes``), and more.
``xnvme_ident`` includes the namespace identifier (``nsid``), command set identifier, and more.   

:ref:`sec-c-api-xnvme_lba` includes functions for constructing a `xnvme_lba_range`, which is a struct
representing a range of logical block addresses (LBAs).

**NVMe specification**

:ref:`sec-c-api-xnvme_spec` is a C representation of the NVMe specification.
This includes most of the structures described across the base, NVM, ZNS, and KV specifications. 

In line with this, :ref:`sec-c-api-xnvme_pi` and :ref:`sec-c-api-xnvme_topology` provides functions
for NVMe operations which aren't commands.
:ref:`sec-c-api-xnvme_pi` has functions for generating and verifying protection info.
:ref:`sec-c-api-xnvme_topology` has functions for subsystem (or controller) reset, namespace rescan,
and getting controller registers.

**Command-line interface**

:ref:`sec-c-api-xnvme_cli` provides functionality to create
command-line interfaces for **xNVMe** related applications. Most of the **xNVMe**
examples, tools, and tests use this header to provide a coherent command-line
interface with bash-completion-scripts and man-pages. 

.. toctree::
   :hidden:
   :glob:

   xnvme_*
