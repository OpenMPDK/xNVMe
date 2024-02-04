.. _sec-api-python:

========
 Python
========

Bindings to the :ref:`sec-c-api` from Python is provided via an interface
implemented using ctypes_ and distributed via pypi.

The Python interface is a work-in-progress, provided as a preview of what can
be done when integrating with Python. See, section :ref:`sec-python-todo` for a
list of known shortcomings and areas to improve. Your input on these are most
welcome along with pull-requests.

.. _sec-python-ctypes:

ctypes "sugar"
==============

The ctypes_ bindings provides access to the xNVMe C API via ctypes_. If you
have used ctypes_ before, then you can go right ahead and use these. The only
thing added as an augmentation of what ctypes_ provides is an improvement of
the Dynamic loader logic:

* library-loading is extended with use of ``pkg-config``
* library-loading is done implicitly upon ``import``
* library-loading can be checked via:
  - ``xnvme.ctypes_bindings.is_loaded``, check the bool
  - ``xnvme.ctypes_bindings.guard_unloadable()``, exit when not loaded

A convenient aspect of the bindings are that Python ctypes_ comes with
CPython_, that is, with the reference implementation of the Python intepreter.
When using the ctypes_ bindings, the only other runtime dependency is pytest_.

FAQ
===

Attribute Error
---------------

This::

        xnvme.xnvme_dev_get_geo(dev).lba_nbytes

Gives the error::

        AttributeError: 'LP_xnvme_geo' object has no attribute 'lba_nbytes'

One might expect that accessing the above would be possible. However, what the
error-message conveys is that the above accesses a pointer, not the object, and
thus, the pointer does not have the attribute ``lba_nbytes``.

Thus, instead do::

        xnvme.xnvme_dev_get_geo(dev).contents.lba_nbytes

This is the Ctypes_ approach to dereferencing pointers.

Install
-------

Before installing, then ensure that the following system dependencies are met:

* Python 3.6+
* **xNVMe** installed on your system
* ``pkg-config`` installed and able to locate **xNVMe** on your system

See, the :ref:`sec-building` on how to build and install xNVMe. With the above
then you can go ahead and install the bindings using ``pip``::

  # install: for the current user
  python3 -m pip install xnvme --user

For a description of building and installing the package from source, then see
the :ref:`sec-python-development` section.

.. note:: The **xNVMe** Python package comes with pytests, thus, you can verify
   your installation by running: ``python3 -m pytest --pyargs
   xnvme.ctypes_mapping``.

Usage
-----

See the files in ``bin/*``. At the time of writing this, there were three
examples, two of which require no arguments, these are run like so::

  # Enumerate devices on the system
  ./bin/xpy_enumerate

  # Print library-information
  ./bin/xpy_libconf

The third opens and queries a device for information, and thus requires a
device as well as permissions, on a Linux system, running it could look
something like this::

  sudo -E ./bin/xpy_dev_open --uri /dev/nvme0n1

.. note:: In the above example ``sudo -E`` is used. The ``sudo`` is there to
   ensure permission to ``/dev/nvme0n1``, then ``-E`` is there such that the
   **xNVMe** package installed as ``--user`` is available.

.. _sec-python-development:

Development
===========

A couple of notes on the idiosyncrasies of the implementations.

ctypes
------

The bindings are generated using the ctypeslib2_ tool, patched (injecting an
extension to the library-loader), and then formatted with black_.

The build-requirements are installable via ``requirements.txt``::

  python3 -m pip install -r requirements.txt --user

Furthermore, ``clang`` is needed on the system::

  # Debian
  sudo apt-get install libclang-dev

  # Alpine
  sudo apk add clang-libs

A Makefile is available for common tasks, run::

  make help

To see what it provides / common-tasks during development. For example::

  make build uninstall install test

The above ``make`` invocation will generate the ctypes-mapping via
``clang2py``, then patch the mappings using the auxilary scripts
``aux/patch_ctypes_mapping.py``, adjust the style according to the conventions
of ``black``, create a Python sdist package, install the package, and finally
run the pytests.

**CAVEAT:** the mappings produced by ``clang2py`` aren't stable. That is, the
ordering in which classes are emitted can change from each invocation of the
tool.

.. _sec-python-todo:

TODO
====

As mentioned earlier, then the Python language bindings are a work in progress,
the following are mixture of notes for improvment along with things to be aware
of when using the Python language bindings.

* Explore how to distribute the **xNVMe** source on pypi_

  - Should provide the source-archive of **xNVMe**
  - Should provide means of building the library along with the Python package
  - Should provide a means of making the library available to the Python language bindings
  - See one approach to explore in the mention on ``mesonpep517``

* Explore using ``mesonpep517`` for the bindings

  - https://pypi.org/project/mesonpep517/
  - https://github.com/mesonbuild/meson/issues/7863
  - https://thiblahute.gitlab.io/mesonpep517/

* Re-consider the API-guard ``capi.guard_unloadable()``.

* The package-readme ``python/README.rst`` is lacking in proper description and
  pointers to information. This should be improved.

* **testing** The bindings have only been tested on Linux and macOS

  - Add testing on Windows
  - Add testing on FreeBSD

* **RECONSIDER:** The auto-generated ctypes-mapping has prefixes for e.g.
  ``union_`` and ``struct_``, the patcher removes these. This works for the
  xNVMe C API since there are no collisions, however, in the general case it
  would break. So, reconsider which is the preferable form for a "raw C API
  mapping".

* **ctypes_mapping:** The bit-fields and nested structs have cumbersome
  accessors in Python, this could be improved by modifying the ``clang2py`` /
  ``ctypeslib2``

* **ctypes_mapping:** The generated bindings are **not** stable, that is, the
  output emitted from ``clang2py`` changes order of the generated items. This
  would be nice to fix by submitting a PR to the ctypeslib2_.

.. _CPython: https://en.wikipedia.org/wiki/CPython
.. _black: https://github.com/psf/black
.. _ctypes: https://docs.python.org/3/library/ctypes.html
.. _ctypeslib2: https://github.com/trolldbois/ctypeslib/
.. _pypi: https://pypi.org/
.. _pytest: https://pytest.org/
