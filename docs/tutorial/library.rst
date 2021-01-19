.. _sec-tutorials-library:

Library Usage Notes
===================

The essence of xNVMe is an interface that enables you to command a device to do
something. This is a achieved via a command-interface with asynchronous as well
as synchronous semantics. In pseudo-code, you get a device-handle by opening
it::

  dev = xnvme_dev_open("/dev/nvme0n1");

To send a command, you need a command context, for synchronous execution, then
you retrieve a command-context from the device::

  # Get a synchronous command context
  ctx = xnvme_cmd_ctx_from_dev(dev);

  # Submit and wait for completion
  res = xnvme_cmd_pass(ctx, cmd, ...);

For asynchronous execution, then you get the command context from a queue::

  # Get an asynchronous command context
  ctx = xnvme_cmd_ctx_from_queue(queue);

  # Submit the command
  res = xnvme_cmd_pass(ctx, cmd, ...);

  # Process at least 'n' commands, n can be 0
  res = xnvme_queue_poke(queue, n);

  # Wait for all
  res = xnvme_queue_wait(queue);

There are more details to the use of the library. E.g. what is a command, how
do you construct and how do you allocate and provide data for the command to
work with? And how do you setup queues, provide callback-functions, and
callback-function arguments?

However, that is just boiler-plate code and designed to make it look and feel
familiar.

When synchronous, then there is less things to do since ``xnvme_cmd_pass`` is
blocking until the command has completed or if it failed submitting.

When using asynchronous semantics, then there is more things to do, you have to
setup a queue, and poll/wait and implement a call-back function if you want to
do something specific to the command that completed. This is because the
submission and completion has been split up.

However, the pseuod-code above is the gist of the library, open a device, get a
command context and send a command.

Backends
--------

...

Command Interfaces
------------------

...

Return Values and Status Codes
------------------------------

The ``xnvme_cmd`` functions signal errors using the standard ``errno``
thread-local variable and a return value of ``-1``. To gain more insights into
what went wrong in a command, use the optional ``xnvme_cmd_ctx`` parameter that all
``xnvme_cmd`` functions take.  When supplied, the ``xnvme_cmd_ctx`` will contain
the NVMe status code in the ``status`` field.

**Note** that while the :ref:`spdk <sec-backends-spdk>` backend has a
one-to-one correspondence between what is in the ``xnvme_cmd_ctx`` and what is
reported by the device, the :ref:`Linux <sec-backends-linux>`, backend will
transform certain errors from the kernel into their NVMe equivalents.

This is because certain ``ioctl`` calls may return ``-1`` and ``Invalid
Argument`` without actually sending a command to the device. To align the
backends with each other, such an error will be transformed into the NVMe
equivalent, if possible. For example, ``Invalid Argument`` will be transformed
into the NVMe status code ``Invalid Field in Command`` and set that in the
given ``xnvme_cmd_ctx``.

TODO: add references to the C API

Memory Management
-----------------

When using the ``xnvme_cmd`` option ``XNVME_CMD_SGL``, then use these functions
to manage the Scather / Gather Lists.

Asynchronous Interface
----------------------

When using the ``xnvme_cmd`` option ``XNVME_CMD_ASYNC``, then use these
functions to steer the asynchronous mechanics.

Pretty-Printing
---------------

Every structure has a pretty-printer function. Even the opaque ones, e.g.
:cpp:class:`xnvme_dev` has the printer function :c:func:`xnvme_dev_pr()`.  By
convention they are named the same as the structure plus a `_pr` postfix.

The printers all take an `opts` parameter which controls how the structure
should be printed, the current default is `YAML`_ representation.

.. _YAML: https://yaml.org/
