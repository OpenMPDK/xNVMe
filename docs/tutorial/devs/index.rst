.. _sec-tutorials-devs-linux:

Linux Dev. Environment
======================

This shows the manual steps of setting up a physical machine for **xNVMe** test
and development. This includes using devices attached to the machine, as well
as using the physical machine to setup virtual machines with advanced
NVMe-device emulation.

Advanced NVMe device emulation covers command-set enchancements such as
Flexible Data-Placement (FDP), Zoned Namespaces (ZNS), and Key-Value SSDs (KV).

The documentation is provided as a means to setup development environment with
minimal fuss and maximum foss :)

* Custom Linux kernel

  * Built from source
  * Installed via ``.deb``

* Custom Qemu

  * Built from source
  * Latest NVMe emulation features

* xNVMe

  * Built from source
  * Testing package

.. _sec-tutorials-devs-linux-pm:

Physical Machine
----------------

It is assumes that you have a physisical machine with NVMe storage available,
and that you can utilize it destructively. By desctructively, what is meant, is
that any data can and will be lost.

Most machines will do for common development and testing, throughout an machine
with the following traits are used:

* An x86 64-bit CPU

* At least 8GB of memory

* A SATA SSD, 100GB will suffice, used for the

  * Operating system
  * User data (``$HOME``), repositories, workdirs, etc.

* One or more NVMe SSDs

  * Utilized for testing

This physical machine will be referred to as ``box01``.

.. tip::
   Since, the devices utilized for testing a primarily NVMe devices, then it is
   convenient to have the operating system and user-data installed on a
   separate non-NVMe device, quite simply since SATA pops up as ``/dev/sd*``
   and NVMe as ``/dev/nvme*`` / ``/dev/ng*``. It avoids one accidentally wiping
   the OS device when the devices are entirely separate.


.. _sec-tutorials-devs-linux-os:

Operating System
----------------

Here, the latest **stable** Debian Linux, currently **bookworm** is used
throughout.

* Install it

  * Un-select desktop environment

  * Select OpenSSH

* Add user ``odus`` with password as you see fit
* Partition using the entire device

* Setup networking

And make sure it is accessible from your "main" development machine::

  ssh-copy-id odus@box01

Log into the machine::

  ssh odus@box01

And then install a couple of things::

  # Switch to root
  su -
  apt-get -qy update
  apt-get -qy install \
    git \
    htop \
    screen \
    sudo \
    vim

  # Add odus to sudoers (required to do various things as non-root)
  usermod -aG sudo odus

  # Add odus to kvm (required to run qemu as non-root)
  usermod -aG kvm odus

  # switch back out of root
  exit

  # log out of odus
  exit

.. tip::
   Log out and back in again, to refresh credentials

Additionally, in order to prepare the system for user-space NVMe drivers, then
vfio/iommu should be enabled along with a couple of user-limit tweaks.

Have a look at the :ref:`sec-gs-system-config-userspace-config` section for the
details on this.

.. _sec-tutorials-devs-linux-homedir:

Homedir
-------

Create a directory structure in the ``$HOME`` directory::

  mkdir $HOME/{artifacts,git,workdirs,guests,images}

The directories are used for the following:

**git**
  A place to store source-repositories, usually these are git repositories for
  projects like: xnvme, fio, spdk, linux, and qemu.

**workdirs**
  A place for auxilary files, when executing **cijoe** workflows, or doing
  misc. experiments and exploration.

**artifacts**
  A place to store intermediate artifacts during development. Such as adhoc
  Linux kernel ``.deb`` packages, source-archives etc.

**guests**
  A place where boot-images, pid-files, cloud-seeds and other files related to
  qemu guests live.

**images**
  A place to store VM "boot-images", such as cloud-init enabled images.

.. _sec-tutorials-devs-linux-screen:

Screen + http.server
--------------------

Regardless of whether your **devbox** is physical/virtual/local/remote or some
combination thereof. Then having access to misc. files, and specifically, to
things like **cijoe** output / reports. Is very convenient.

With minimal fuss, then this is achievable with a combinaion of ``screen`` and
Python::

  cd ~/workdirs
  screen -d -m python3 -m http.server

The above starts a webserver, serving the content of the ``cwd`` where
``python3`` is executed and served up over ``tcp/http`` on port **8000**.

The ``screen -d -m`` part, creates a screen-session and detaches from it. Thus,
it continues executing even if you disconnect.

You can see the running screen-sessions with::

  screen -list

And attach to them using their ``<name>``::

  screen -r <name>

.. _sec-tutorials-devs-linux-cijoe:

CIJOE
-----

Setup ``python3`` and ``pipx``::

  sudo apt-get -qy install \
    pipx \
    python3-pip \
    python3-venv
  sudo pipx ensurepath

Then install **cijoe** in a ``pipx`` virtual environment::

  pipx install cijoe --include-deps
  pipx inject cijoe cijoe-pkg-linux
  pipx inject cijoe cijoe-pkg-qemu
  pipx inject cijoe cijoe-pkg-fio

Then logout and back in to reload the environment, the addition of ``pipx`` and
the ``cijoe`` into ``$PATH``.

Do a trial-run::

  # Create a workdir
  mkdir -p ~/workdirs/cijoe
  cd ~/workdirs/cijoe

  # Create a default configuration and workflow
  cijoe --example core

In case everything is fine, then it will execute silently.

You can increase the information-level with ``-l``
argument, the more times you provide the higher the level.
Try running it with two, that is debug-level::

  cijoe -ll

In the ``cwd`` then a ``cijoe-output`` is produced, this
directory holds all information about what was executed.
Have a look at the generated report at
``cijoe-output/report.html``.

.. _sec-tutorials-devs-linux-customkernel:

Linux Kernel
------------

Install prerequisites::

  sudo apt-get -qy install \
    bc \
    bison \
    build-essential \
    debhelper \
    flex \
    git \
    libelf-dev \
    libssl-dev \
    pahole \
    rsync

Then run the **cijoe** workflow, compiling a custom kernel as a ``.deb``
package::

  # Create a workdir for the workflow
  mkdir -p ~/workdirs/linux
  cd ~/workdirs/linux

  # Grab the cijoe-example for linux
  cijoe --example linux

  # Run it with logging (-l)
  cijoe -l

Then re-run the command above. It should now succeed, after which you can
collect the artifacts of interest::

  cp -r cijoe-output/artifacts/linux ~/artifacts/

You can install them by running::

  sudo dpkg -i ~/artifacts/linux/*.deb

.. _sec-tutorials-devs-linux-qemu:

Qemu
----

Install prerequisites::

  # Packages for building qemu
  sudo apt-get -qy install \
    meson \
    libattr1-dev \
    libcap-ng-dev \
    libglib2.0-dev \
    libpixman-1-dev \
    libslirp-dev \
    pkg-config

  # Packages for cloud-init
  sudo apt-get -qy install \
    cloud-image-utils

Checkout qemu::

  cd ~/git
  git clone https://github.com/OpenMPDK/qemu --recursive
  cd qemu
  git checkout for-xnvme
  git submodule update --init --recursive

Create a work-directory::

  mkdir -p ~/workdirs/qemu
  cd ~/workdirs/qemu

Run the **cijoe** qemu workflow::

  # Grab the config and workflow example for qemu
  cijoe --example qemu

  # Run it with log-level debug (-l)
  cijoe -l

With the packages installed, go back and run the **cijoe** workflow. Have a
look at the report, it describes what it does, that is, build and install qemu,
spin up a vm using a cloud-init-enabled Debian image, ssh into it.

.. tip::
   In case you get errors such as::

     Could not access KVM kernel module: No such file or directory
     qemu-system-x86_64: failed to initialize kvm: No such file or directory

   Then this is usually a symptom of virtualization being
   disabled in the BIOS of the physical machine. Have a look
   at ``dmesg`` it might proide messages supporting this.

xNVMe
-----

clone, build, and install
~~~~~~~~~~~~~~~~~~~~~~~~~

Clone **xNVMe** and checkout the ``next`` branch::

  cd ~/git
  git clone https://github.com/OpenMPDK/xNVMe.git xnvme
  cd xnvme
  git checkout next

Install prerequisites::

  sudo ./toolbox/pkgs/debian-bookworm.sh

Build and install **xNVMe**::

  cd ~/git/xnvme
  make
  sudo make install

Check that it is functional::

  sudo xnvme enum

This should yield output similar to::

  xnvme_cli_enumeration:
  - {uri: '/dev/nvme0n1', dtype: 0x2, nsid: 0x1, csi: 0x0, subnqn: ''}

Artifacts
~~~~~~~~~

Produce a set of **artifacts**::

  cd ~/git/xnvme
  make clobber gen-artifacts

  # Keep them handy if need be
  cp -r /tmp/artifacts ~/artifacts/xnvme

.. warning::
   The ``make clobber`` removes any unstaged changes.
   This is done to ensure an entirely "clean" repository. Thus, make sure that
   you have commit your changes.
   The ``make clobber`` is required for ``make gen-artifacts``, as it will
   otherwise include side-effects from previous builds.

.. note::
   The artifacts produces by ``make gen-artifacts`` are output to
   ``/tmp/artifacts``. There are **cijoe** workflows, expecting to be available
   at that location, specifically the **provision** workflow.

Reproduce GitHUB Actions locally
--------------------------------

The **cijoe** workflows and configurations in this directory are used in the
xNVMe GitHUB actions. You can reproduce what is running on GitHUB by adjusting
the config-files, and provide the artifacts from the GitHUB action:

* xnvme-py-sdist.tar.gz
* xnvme-src.tar.gz

To do so, then:

* Place the artifacts in ``/tmp/artifacts``
* Change ``qemu.system_bin`` to point to your qemu-system-binary (qemu 7+)
* Add the SSH-key(``keys/guest_key``) to your SSH-agent.

Then you should be able to run the following::

  # Provision and test on Debian Bullseye
  cijoe -c configs/debian-bullseye.toml -w workflows/provision.yaml
  cijoe -c configs/debian-bullseye.toml -w workflows/test-debian-bullseye.yaml

  # Provision and test on FreeBSD 13
  cijoe -c configs/freebsd-13.toml -w workflows/provision.yaml
  cijoe -c configs/freebsd-13.toml -w workflows/test-freebsd-13.yaml

  # Generate documentation (provisions qemu-guest and generates the docs)
  cijoe -c configs/debian-bullseye.toml -w workflows/docgen.yaml

In case you are setting up the test-target using other tools, or just want to
run pytest directly, then the following two sections describe how to do that.

Running pytest from the repository
----------------------------------

Invoke pytest providing a configuration file and an output directory for
artifacts and captured output::

  pytest \
    --config configs/debian-bullseye.toml \
    --output /tmp/somewhere \
   tests

The ``--config`` is needed to inform pytest about the environment you are
running in such as which devices it can use for testing. The information is
utilized by pytest to, among other things, do parametrization for xNVMe backend
configurations etc.

Provision a qemu-guest
~~~~~~~~~~~~~~~~~~~~~~

Setup a virtual machine with **xNVMe** installed, and a bunch of NVMe devices configured::

  cijoe -c configs/debian-bullseye.toml -w provision.yaml

.. tip::
   It will likely fail with the error::

     /bin/sh: 1: /opt/qemu/bin/qemu-system-x86_64: not found

   This is because the default configuration is for running on Github. Thus,
   adjust the file ``configs/debian-bullseye.toml`` such that qemu is
   pointing to ``$HOME``.

Create boot-images
~~~~~~~~~~~~~~~~~~

The ``debian-bullseye-amd64.qcow2`` is created by::

  cijoe -c configs/debian-bullseye.toml -w workflows/bootimg-debian-bullseye-amd64.yaml

The ``freebsd-13.1-ksrc-amd64.qcow2`` is created by::

  cijoe -c configs/freebsd-13.toml -w workflows/bootimg-freebsd-13-amd64.yaml

Remote dev
----------

Assuming your primary device for development is something like a
Chromebook/Macbook, something light-weight and great for reading mail... but
now you want to fire up your editor and do some development.

Or, your primary system is simply separate from the dev-box for a myriad of
reasons. Then do something like::

  cp configs/debian-bullseye.toml configs/dev-metal.toml

.. note::
   all configs prefix on the file-name pattern ``dev-*.toml`` are ignored by git.

Open up ``configs/dev-metal.toml`` and adjust it to your physical machine. That
is, change the ssh-login information, change the list of devices, paths to
binaries etc. Once you have done that, then go ahead and run::

  cijoe -c configs/dev-metal.toml -w dev-sync-and-build.yaml
