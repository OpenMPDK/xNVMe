.. _sec-examples:

=================
 C API: Examples
=================

In the source repos you will find usage examples in the ``examples`` folder.
Additionally, having a look at cli-tools in ``tools`` is also useful as they
touch most of the C APIs including.

The usage-tests in ``tests`` could be consulted as well, however, they are by
no means written with the intent of being read.

For a quick look at what code using **xNVMe** then have a look at the examples
in the following sections.

Hello World
===========

.. literalinclude:: ../../examples/xnvme_hello.c
   :language: c

Asynchronous IO
===============

.. literalinclude:: ../../examples/xnvme_io_async.c
   :language: c
