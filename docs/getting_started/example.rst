.. _sec-gs-building-example:

Building an xNVMe Program
=========================

At this point you should have **xNVMe** built and installed on your system and
have the system correctly configured and you should by now also be familiar
with how to instrument **xNVMe** to utilize different backends and backend
options.

With all that in place, go ahead and compile your own **xNVMe** program.

Example code
------------

This `"hello-world"` example prints out device information of the NVMe
device at ``/dev/nvme0n1``.

To use **xNVMe** include the ``libxnvme.h`` header in your C/C++ source:

.. literalinclude:: hello.c
   :language: c

Compile and link
----------------

A pkg-config is provided with xNVMe, you can use ``pkg-config`` to get the
required linker flags:

.. literalinclude:: pkg_config_00.cmd
   :language: bash

This will output something like the output below, it will vary depending on the
features enabled/disabled.

.. literalinclude:: pkg_config_00.out
   :language: bash

You can pass the arguments above to your compiler, or using pkg-config like so:

.. literalinclude:: hello_00.cmd
   :language: bash

.. note:: You do not need to link with SPDK/DPDK, as these are bundled
  with **xNVMe**. However, do take note of the linker flags surrounding
  ``-lxnvme``, these are required as SPDK makes use of
  ``__attribute__((constructor))``. Without the linker flags, none of the SPDK
  transports will work, as **ctors** will be "linked-out", and **xNVMe** will
  give you errors such as **device not found**.

Also, **xNVMe** provides two different libraries, a static, and a shared
library as well. Here is what the different libraries are intended for:

* ``libxnvme.a``, this is static library version of **xNVMe** except it does
  **not** come with **batteries included**, so you have to manually link with
  SPDK, liburing, etc.
* ``libxnvme.so``, this is the shared library version of **xNVMe** and it comes
  with **batteries included**, that is, all the third-party libraries are bundled
  within the shared library ``libxnvme.so``. Thus you only need to link with
  **xNVMe**, as described above, and need not worry about linking with SPDK,
  liburing etc.

Using ``libxnvme.so`` is the preferred way to consume **xNVMe** as it comes with
the correct version of the various third-party libraries and provides for
a simpler link-target.

Run!
----

.. literalinclude:: hello_01.cmd
   :language: bash

.. literalinclude:: hello_01.out
   :language: bash

This should conclude the getting started guide of **xNVMe**, go ahead and
explore the :ref:`sec-tools`, :ref:`sec-c-api`, and :ref:`sec-examples`.

Should **xNVMe** or your system still be misbehaving, then take a look in the
:ref:`sec-troubleshooting` section or reach out by raising an issue_, start an
asynchronous discussion_, or go to Discord_ for synchronous interaction.
