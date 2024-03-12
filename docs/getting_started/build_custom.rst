.. _sec-building-custom:

Customizing the Build
=====================

.. _sec-building-custom-toolchain:

Non-GCC Toolchain
-----------------

To use a compiler other than **gcc**, then:

1) Set the ``CC`` and ``CXX`` environment variable for the ``./configure`` script
2) Pass ``CC``, and ``CXX`` as arguments to ``make``

For example, compiling **xNVMe** on a system where the default compiler is not
**gcc**:

.. code-block:: bash

  CC=gcc CXX=g++ ./configure <YOUR_OPTIONS_HERE>
  make CC=gcc CXX=g++
  make install CC=gcc CXX=g++

Recent versions of **icc**, **clang**, and **pgi** should be able to satisfy
the **C11** and **pthreads** requirements. However, it will most likely require
a bit of fiddling.

.. note:: The **icc** works well after you bought a license and installed it
   correctly. There is a free option with Intel System Suite 2019.


.. note:: The **pgi** compiler has some issues linking with **SPDK/DPDK** due
   to unstable **ABI** for **RTE** it seems.

The path of least resistance is to just install the toolchain and libraries as
described in the :ref:`sec-building-toolchain` section.

.. _sec-building-config:

Custom Configuration
--------------------

See the list of options in ``meson_options.txt``, this file defines the
different non-generic options that you can toggle. For traditional
build-configuration such as ``--prefix`` then these are managed like all other
meson-based builds::

  meson setup builddir -Dprefix=/foo/bar

See: https://mesonbuild.com/Builtin-options.html

For details

.. _sec-building-crosscompiling:

Cross-compiling for ARM on x86
------------------------------

This is managed like any other meson-based build, see:

https://mesonbuild.com/Cross-compilation.html

for details

.. _issue: https://github.com/OpenMPDK/xNVMe/issues

.. _discussion: https://github.com/OpenMPDK/xNVMe/discussions

.. _Discord: https://discord.com/invite/XCbBX9DmKf

.. _vfio: https://www.kernel.org/doc/Documentation/vfio.txt

.. _uio: https://www.kernel.org/doc/html/v4.14/driver-api/uio-howto.html

.. _Chocolatey: https://chocolatey.org/

.. _MinGW: https://www.mingw-w64.org/

.. _Homebrew: https://brew.sh/