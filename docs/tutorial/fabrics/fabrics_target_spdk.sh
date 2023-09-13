#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_target_modules.sh

echo "### NVMe-oF Fabrics target setup using SPDK"
xnvme-driver

echo "## Build the SPDK NVMe-oF target-app (nvmf_tgt)"
pushd "${SPDK_REPOS}/app/nvmf_tgt"
make
popd

echo "# Start 'nvmf_tgt' and give it two seconds to settle"
pkill -f nvmf_tgt || echo "Failed killing 'nvmf_tgt'; OK, continuing."
pushd "${SPDK_REPOS}/build/bin"
./nvmf_tgt &
popd
sleep 2
if ! pidof nvmf_tgt; then
  echo "## Failed starting 'nvmf_tgt'"
  exit
fi

echo "## Attach local PCIe controllers (${NVMET_TRTYPE})"
"${SPDK_REPOS}/scripts/rpc.py" bdev_nvme_attach_controller \
 -b Nvme0 \
 -t PCIe \
 -a "${EXPORT_DEV_PCIE}"
# The above command will output e.g. 'Nvme0n1'

echo "## Create NVMe-oF transport (${NVMET_TRTYPE})"
"${SPDK_REPOS}/scripts/rpc.py" nvmf_create_transport \
	-t "${NVMET_TRTYPE}" \
	-u 16384 \
	-m 8 \
	-c 8192

echo "## Create a NVMe-oF subsystem/controller"
"${SPDK_REPOS}/scripts/rpc.py" nvmf_create_subsystem \
	"${NVMET_SUBSYS_NQN}" \
	-a \
	-s SPDK00000000000001 \
	-d Controller1

echo "# Export (${EXPORT_DEV_PCIE}) -- add device to SPDK subsystem/controller"
"${SPDK_REPOS}/scripts/rpc.py" nvmf_subsystem_add_ns \
	"${NVMET_SUBSYS_NQN}" \
	Nvme0n1

echo "## Setup NVMe-oF connection-listener"
"${SPDK_REPOS}/scripts/rpc.py" nvmf_subsystem_add_listener \
	"${NVMET_SUBSYS_NQN}" \
	-t "${NVMET_TRTYPE}" \
	-a "${NVMET_TRADDR}" \
	-s "${NVMET_PORT}" \
	-f "${NVMET_ADRFAM}"
