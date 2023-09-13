#!/usr/bin/env bash
#
# Variables defining the fabrics setup, adjust these to match your setup
#
export NVMET_SUBSYS_NQN="nqn.2022-05.io.xnvme:ctrlnode1"
export NVMET_SUBSYS_NSID="1"

export NVMET_TRADDR="1.2.3.4"
export NVMET_TRTYPE="tcp"
export NVMET_TRSVCID="4420"
export NVMET_ADRFAM="ipv4"

export EXPORT_DEV_PATH="/dev/nvme0n1"
export EXPORT_DEV_PCIE="0000:03:00.0"

# Absolute path to the xNVMe repository
export XNVME_REPOS="/root/git/xnvme"

# Absolute path to the SPDK repository
export SPDK_REPOS="/root/git/spdk"

# Absolute path to the Fio repository
export FIO_REPOS="/root/git/fio"
