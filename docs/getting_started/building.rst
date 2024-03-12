.. _sec-gs-building:

Building xNVMe
==============

**xNVMe** builds and runs on Linux, FreeBSD and Windows. First, retrieve the
**xNVMe** repository from  `GitHUB <https://github.com/OpenMPDK/xNVMe>`_:

.. include:: clone.rst

Before you invoke the compilation, then ensure that you have the compiler,
tools, and libraries needed. The :ref:`sec-building-toolchain` section
describes what to install, and how, on rich a selection of Linux distributions,
FreeBSD and Windows. For example on Debian do::

  sudo ./xnvme/toolbox/pkgs/debian-bullseye.sh

With the source available, and toolchain up and running, then go ahead:

.. include:: build_meson.rst

.. note:: Details on the build-errors can be seen by inspecting
   ``builddir/meson-logs/meson-log.txt``.

.. note:: In case you ran the meson-commands before installing, then you
   probably need to remove your ``builddir`` before re-running build commands.

In case you want to customize the build, e.g. install into a different location
etc. then this is all handled by `meson built-in options
<https://mesonbuild.com/Builtin-options.html>`_, in addition to those, then you
can inspect ``meson_options.txt`` which contains build-options specific to
**xNVMe**. For examples of customizing the build then have a look at the
following :ref:`sec-building-config`.

Otherwise, with a successfully built and installed **xNVMe**, then jump to
:ref:`sec-gs-system-config` and :ref:`sec-building-example`.
