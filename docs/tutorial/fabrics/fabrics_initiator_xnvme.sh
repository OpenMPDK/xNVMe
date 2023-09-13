#!/usr/bin/env bash
set -eu
source fabrics_env.sh
source fabrics_initiator_modules.sh

echo "# Prepare the SPDK/DPDK environment"
xnvme-driver

echo "# Enumerate the device"
xnvme enum --uri "${NVMET_TRADDR}:${NVMET_TRSVCID}"

echo "# Inspect the device"
xnvme info "${NVMET_TRADDR}:${NVMET_TRSVCID}" --dev-nsid="${NVMET_SUBSYS_NSID}"

echo "# Run fio"
"${FIO_REPOS}/fio" \
  /usr/local/share/xnvme/xnvme-compare.fio \
  --section=default \
  --ioengine="xnvme" \
  --filename="${NVMET_TRADDR}\\:${NVMET_TRSVCID}" \
  --xnvme_dev_nsid=1
