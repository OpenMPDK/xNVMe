.. _sec-c-api:

########
 C API
########

For didactic purposes the **xNVMe** API can be broken into two parts: the :ref:`sec-c-api-core` API and the :ref:`sec-c-api-extended` API.

The :ref:`sec-c-api-core` API should be the starting point for anyone new to **xNVMe** and storage programming in general.
It describes the minimal set of functions, and their respective header-files, you need to get started with **xNVMe**, which essentially boils down to opening a device, creating a queue, and sending a command. 

The :ref:`sec-c-api-extended` API contains everything else you might need once you are ready to dive deeper into **xNVMe**, including, but not limited to, more command-sets, helpers for lba-ranges, and a representation of the NVMe specification. 


In reality, there is no division of the API. Instead, you can mix and match header-files as needed, or include everything through ``libxnvme.h``.
The ``libxnvme.h`` header is available for inspection below:

.. literalinclude:: ../../include/libxnvme.h
   :language: c

.. toctree::
   :maxdepth: 2
   :hidden:

   core/index
   extended/index
