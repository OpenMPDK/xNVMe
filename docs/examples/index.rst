.. _sec-examples:

=================
 C API: Examples
=================

This section includes three examples of using the **xNVMe** library.
These are gradually increasing in complexity, and are meant to complement
the description of the :ref:`sec-c-api-core` API.

All of these examples can be run even without having an NVMe device.
If you give the argument ``1GB`` on the command-line, **xNVMe** will recognize that you
want to use the ``ramdisk`` backend.

In the source repository, you will find more usage examples in the ``examples`` folder.

Hello World
===========

In this example we open a device with the uri provided on the command line.
This is done by using the default ``xnvme_opts``, but this struct is where you would specify
the desired backend, sync/async/admin command interface, etc.  

.. literalinclude:: ../../examples/xnvme_hello.c
   :language: c

Synchronous IO
===============

In this example we open a device and send a single synchronous read command.
Note, when doing synchronous IO, there is no need for a command-queue.
Instead, you get the command-context directly from the device with ``xnvme_cmd_ctx_from_dev()``.
The example is intended to be easily extended by adding a for-loop and submitting multiple commands.

.. literalinclude:: ../../examples/xnvme_single_sync.c
   :language: c

Asynchronous IO
===============

In this example we open a device and send a single asynchronous read command.
This follows the flow described in the section on the :ref:`sec-c-api-core` API.
The example is intended to be easily extended by adding a for-loop and submitting multiple commands.

.. literalinclude:: ../../examples/xnvme_single_async.c
   :language: c
