#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Unattended update, upgrade, and install
export DEBIAN_FRONTEND=noninteractive
export DEBIAN_PRIORITY=critical
apt-get -qy update
apt-get -qy \
  -o "Dpkg::Options::=--force-confdef" \
  -o "Dpkg::Options::=--force-confold" upgrade
apt-get -qy --no-install-recommends install apt-utils
apt-get -qy autoclean
apt-get -qy install \
 autoconf \
 bash \
 build-essential \
 clang-format \
 git \
 libaio-dev \
 libcunit1-dev \
 libncurses5-dev \
 libnuma-dev \
 libssl-dev \
 libtool \
 liburing-dev \
 make \
 meson \
 nasm \
 openssl \
 pipx \
 pkg-config \
 python3 \
 python3-pip \
 python3-pyelftools \
 python3-venv \
 uuid-dev

#
# Clone, build and install dpdk
#
# Assumptions:
#
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/dpdk/dpdk.git toolbox/third-party/dpdk/repository
pushd toolbox/third-party/dpdk/repository 

git checkout v22.11

# 'cpu_instruction_set=generic' is used for cross compilation reasons for optimal performance use 'cpu_instruction_set=native'
meson setup builddir --prefix=/usr \
-Dc_args="-fPIC -Wno-error  -Wno-stringop-overflow -fcommon -Wno-stringop-overread -Wno-array-bounds" \
-Dcpu_instruction_set=generic \
-Ddisable_libs=gro,node,cfgfile,pdump,bitratestats,flow_classify,table,port,latencystats,kni,metrics,pipeline,jobstats,gpudev,gso,regexdev \
-Ddisable_apps=pdump,test-cmdline,test,test-regex,test-gpudev,test-fib,test-pipeline,test-sad,test-acl,test-bbdev,proc-info,test-eventdev,test-flow-perf,test-pmd,dumpcap,test-compress-perf,test-crypto-perf,test-security-perf \
-Denable_docs=false \
-Denable_drivers=bus,bus/pci,bus/vdev,mempool/ring \
-Denable_kmods=false \
-Dtests=false


meson compile -C builddir
meson install -C builddir
popd

#
# Clone, build and install libvfn
#
# Assumptions:
#
# - Dependencies for building libvfn are met (system packages etc.)
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git toolbox/third-party/libvfn/repository

pushd toolbox/third-party/libvfn/repository
git checkout v2.0.2
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

#
# Clone, build and install spdk
#
# Assumptions:
#
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/spdk/spdk.git toolbox/third-party/spdk/repository

pushd toolbox/third-party/spdk/repository 
git checkout v23.05
git submodule update --init --recursive

./configure \
  --disable-tests \
  --disable-unit-tests \
  --disable-examples \
  --disable-apps \
  --without-crypto \
  --without-fuse \
  --without-idxd \
  --without-iscsi-initiator \
  --without-nvme-cuse \
  --without-ocf \
  --without-rbd \
  --without-uring \
  --without-usdt \
  --without-vhost \
  --without-vtune \
  --without-virtio \
  --without-xnvme \
  --with-shared \
  --with-dpdk=/usr \
  --prefix=/usr

make
make install
popd

