.. _sec-ci:

####
 CI
####

An overview of the environments and the virtual, and physical resources
utilized for the **xNVMe** **CI** is illustrated below.

.. figure:: ../../_static/xnvme-ci-overview.png
   :alt: xNVMe CI Resource Overview
   :align: center

   xNVMe **CI** environments and resources

.. _sec-ci-infrastructure:

Infrastructure
##############

The main logical infrastructure component for the **xNVMe** **CI** is `GitHUB
Actions`_ (**GHA**). **GHA** handles **events** occuring on the following
repositories:

* https://github.com/OpenMPDK/xNVMe/
* https://github.com/xnvme/xnvme.github.io
* https://github.com/xnvme/xnvme-docker
* https://github.com/xnvme/dpdk-windows-drivers

And decides what to execute and where. In other words **GHA** is utilized as a
**resource-scheduler** and **pipeline-engine**. The **executor** role is
delegated to `CIJOE`_ for details, then have a look at `CIJOE in xNVMe`_.

The motivation for this separation is to make it simpler to reproduce build,
test, and verfication issues occuring during a **CI** run, using locally
available resources, by executing the `CIJOE in xNVMe`_ workflows and scripts.

.. _sec-ci-jobs:

Jobs
####

The jobs performed by the **xNVMe** **CI** catch the following issues during
integration of changes / contributions:

* Code format issues

  - Linting and code-formating
  - clang-format for C
  - clippy for Rust
  - black / ruff for Python

* Build issues

  - On a rich collection of Linux Distributions
  - macOS 12, 13
  - Windows 2022

* Functional regressions

  - Running logical tests exercising all code-paths
  - Using a naive "ramdisk" backend
  - Using emulated NVMe devices via qemu

In addition to cathing issues, then the CI is also utilized for:

* Benchmarking of xNVMe

  - Using physical machines
  - Measure peak IOPS for a single physical CPU core
  - Specifically for the integration of xNVMe in SPDK (``bdev_xnvme``)

* Statically Analyze the C code-base

  - CodeQL via GitHUB
  - Coverity

* Produce and deploy documentation

  - Run all example commands (``.cmd`` files) and collect their output in
    ``.out`` files
  - Render the Sphinx-doc documentation as **HTML**
  - Upload rendered documentation to `xnvme.io via GitHUB-pages`_

These following sections provide system-setup notes and other details for the
various **CI** jobs.

.. toctree::
   :maxdepth: 2
   :hidden:
   :includehidden:

   build/index.rst
   bench/index.rst
   verify/index.rst

.. _CIJOE in xNVMe: https://github.com/OpenMPDK/xNVMe/tree/main/cijoe
.. _CIJOE: https://cijoe.readthedocs.io
.. _GitHUB Actions: https://github.com/features/actions
.. _xnvme.io via GitHUB-pages: https://github.com/xnvme/xnvme.github.io
