.. _sec-ci-bench:

Bench
#####

.. figure:: ../../_static/xnvme-ci-overview.png
   :alt: xNVMe CI Resource Overview
   :align: center

   xNVMe **CI** environments and resources

This describes the setup / notes 

.. _sec-ci-bench-notes:

System Notes
============

Since the system utilized is a custom physical setup, then notes are provided
here describing the hardware utilized and the configuration performed to get the
hardware resources into their state of function.

bifrost
-------

The physical hardware resources utilized for benchmarking reside in an isolated
network, only traffic allowed in is via a WireGuard VPN. Taking care of these
firewall and VPN tasks is a `Netgate 1100`_ running **pfSense+**.

The following is setup:

* Firewall rules: default to deny
* DHCP Server with mac-address based IP assignment
* WireGuard Enabled

See the `Netgate 1100 Manual`_ for details on how to configures this.

bench-intel
-----------

.. list-table:: Hardware Specs. for ``bench-intel``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - Intel 12th Gen. Core i5-12600
     - 32 GB (2x Kingston 548C38-16)
     - MSI Z690-A
     - * 4x 980 PRO 2TB
       * 3x 980 PRO 1TB
       * 1x 980 PRO with Heatsink 1TB

bench-intel-pikvm
-----------------

This running PiKVM_ on a `Raspberry Pi 4b`_ with the `PiKVM V3 HAT`_, for setup
notes see `PiKVM V3 HAT Config Notes`_.

bench-amd
---------

.. list-table:: Hardware Specs. for ``bench-amd``
   :widths: 30 15 15 40
   :header-rows: 1

   * - CPU
     - Memory
     - Motherboard
     - NVMe Devices
   * - AMD Ryzen Threadripper 1900X 8-Core
     - 32 GB (4x CMK32GX4M4B3200C16)
     - ASRock X399 Phantom Gaming 6
     - - 8x MEMPEK1W016GA
       - 2x MEMPEK1J016GAL

bench-amd-pikvm
---------------

...


runner-rockpi-01
----------------

This is a `Rock PI 4b`_ with a 512GB NVMe SSD attached.
It is running `Armbian`_ / Bookworm.

Add a user for the github action runner, named ``ghr``:

.. code-block:: bash

	adduser ghr
	usermod -aG sudo ghr

Install some tools:

.. code-block:: bash

	apt-get -qy install \
		git \
		vim \
		time \
		tree

Configure the NVMe storage, by doing the following:

* Partition and ext4-format

  - fdisk /dev/nvme0n1
  - mkfs.ext4 /dev/nvme0n1p1

* Get the **UUID**

  - Run: ``blkid``

* Edit ``/etc/fstab`` using the **UUID** with mount-point at ``/gha``

Then reload:

.. code-block:: bash

	systemctl daemon-reload
	mount /ghr
	chown -R ghr:ghr /ghr

Install docker
~~~~~~~~~~~~~~

Do this:

.. code-block:: bash

	curl -fsSL https://get.docker.com -o get-docker.sh
	sh ./get-docker.sh

Change docker to store container-images and temporary data onto the NVMe device, to avoid wear on the emmc:

.. code-block:: bash

	# Setup a docker config
	mkdir /etc/systemd/system/docker.service.d
	echo "[Service]" >> /etc/systemd/system/docker.service.d/docker.conf
	echo "ExecStart=" >> "/etc/systemd/system/docker.service.d/docker.conf"
	echo "ExecStart=/usr/bin/dockerd --data-root /ghr/docker" >> "/etc/systemd/system/docker.service.d/docker.conf"

	# Reload it
	systemctl daemon-reload
	systemctl restart docker


GitHUB Runner
~~~~~~~~~~~~~

Switch to the ``ghr`` user, go into the ``/ghr`` mountpoint, download and
extract the github-action-runner:

.. code-block:: bash

	su ghr
	cd /ghr
	mkdir actions-runner && cd actions-runner
	curl -o actions-runner-linux-arm64-2.309.0.tar.gz -L https://github.com/actions/runner/releases/download/v2.309.0/actions-runner-linux-arm64-2.309.0.tar.gz
	echo "b172da68eef96d552534294e4fb0a3ff524e945fc5d955666bab24eccc6ed149  actions-runner-linux-arm64-2.309.0.tar.gz" | shasum -a 256 -c
	tar xzf ./actions-runner-linux-arm64-2.309.0.tar.gz

Then we create two runners, one for ``bench-amd``, and one for ``bench-intel``:

.. code-block:: bash

	export RUNNERS="bench-amd bench-intel"
	export RUNNER_USER=ghr
	export URL=https://github.com/OpenMPDK/xNVMe
	export TOKEN={SUPER_SECRET}

With the above defined, then you can execute these:

.. code-block:: bash

	cd /ghr

	# Setup runners
	for RUNNER_NAME in $RUNNERS; do cp -r actions-runner "runner-for-${RUNNER_NAME}"; done;

	# Register runners

	cd /ghr/runner-for-bench-intel
	./config.sh --unattended --url ${URL} --token ${TOKEN} --labels bench,intel --replace --name runner-for-bench-intel
  cd ..

	cd /ghr/runner-for-bench-amd
	./config.sh --unattended --url ${URL} --token ${TOKEN} --labels bench,amd --replace --name runner-for-bench-amd
  cd ..


Install and run them as a service:

.. code-block:: bash

	cd /ghr

	# Service(s): install
	for RUNNER_NAME in $RUNNERS; do pushd "runner-for-${RUNNER_NAME}"; sudo ./svc.sh install ${RUNNER_USER}; popd; done

	# Service(s): start
	for RUNNER_NAME in $RUNNERS; do pushd "runner-for-${RUNNER_NAME}"; sudo ./svc.sh start; popd; done

	# Service(s): status
	for RUNNER_NAME in $RUNNERS; do pushd "runner-for-${RUNNER_NAME}"; sudo ./svc.sh status; popd; done

And when needing to update:

.. code-block:: bash

	# Services: stop
	for RUNNER_NAME in $RUNNERS; do pushd "runner-for-${RUNNER_NAME}"; sudo ./svc.sh stop; popd; done

	# Services: uninstall
	for RUNNER_NAME in $RUNNERS; do pushd "runner-for-${RUNNER_NAME}"; sudo ./svc.sh uninstall; popd; done

	# Remove the runner
	for RUNNER_NAME in $RUNNERS; do pushd "runner-for-${RUNNER_NAME}"; ./config.sh remove --token ${TOKEN}; popd; done;


.. _Armbian: https://www.armbian.com/
.. _Netgate 1100: https://shop.netgate.com/products/1100-pfsense
.. _Netgate 1100 Manual: https://docs.netgate.com/pfsense/en/latest/solutions/sg-1100/
.. _Rock Pi 4b: https://rockpi.org/rockpi4

.. _PiKVM: https://pikvm.org/
.. _PiKVM V3 HAT: https://docs.pikvm.org/v3/
.. _PiKVM V3 HAT Config Notes: https://safl.dk/notebook/pikvm/
.. _Raspberry Pi 4b: https://www.raspberrypi.com/products/raspberry-pi-4-model-b/
