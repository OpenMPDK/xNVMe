.. _sec-c-api-core:

######
 Core
######

This section describes the core **xNVMe** API, which includes functions (and headers) 
that will most likely be part of every program using the **xNVMe** library.

For the most part, **xNVMe** follows the NVMe specification (represented in C 
in :ref:`sec-c-api-xnvme_spec`). However, there are a couple abstractions added on
top to make programming easier. The most essential of these are the structs:
``xnvme_cmd_ctx`` and ``xnvme_queue``.

The ``xnvme_cmd_ctx``, referred to as command-context, encapsulates both the NVMe
command to be processed and the completion result once it is done.

The ``xnvme_queue``, referred to as command-queue, is initialized with a number of
command-contexts equal to the capacity. When you want to submit a command, you 
get a command-context from the command-queue. When you want to process completed
commands you poke the command-queue, and a callback function is called for every
command-context. At the end of the callback function, you should put the 
command-context back into the command-queue.

The typical flow for doing asynchronous IO can be reduced to the following:

- Open device with ``xnvme_dev_open()``
- Initialize command-queue with ``xnvme_queue_init()``
- Set queue callback with ``xnvme_queue_set_cb()``
- Get command-context from queue with ``xnvme_queue_get_cmd_ctx``
- Submit an IO command, e.g., with ``xnvme_nvm_read()`` (see :ref:`sec-c-api-xnvme_nvm` for more options)
- Get completion with ``xnvme_queue_poke()`` 
- Terminate command-queue with ``xnvme_queue_term()``

Examples of both asynchronous and synchronous IO in C can be seen here: :ref:`sec-examples`.

The following gives a brief description of the header files in the core API:

- :ref:`sec-c-api-xnvme_adm` includes functions for construction and submission of NVMe admin commands
- :ref:`sec-c-api-xnvme_buf` includes functions for allocating and freeing memory with respect to the backend
- :ref:`sec-c-api-xnvme_cmd` includes the command-context and functions related to it
- :ref:`sec-c-api-xnvme_dev` includes functions for enumerating and opening devices
- :ref:`sec-c-api-xnvme_nvm` includes functions for construction and submission of NVMe IO commands
- :ref:`sec-c-api-xnvme_queue` includes the command-queue and functions related to it


.. toctree::
   :hidden:
   :glob:

   xnvme_*
