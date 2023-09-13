#!/bin/tcsh
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

# Install packages via the system package-manager (pkg)
pkg install -qy \
 autoconf \
 automake \
 bash \
 cunit \
 devel/py-pyelftools \
 e2fsprogs-libuuid \
 git \
 gmake \
 libtool \
 meson \
 nasm \
 ncurses \
 openssl \
 pkgconf \
 py39-pipx \
 python3 \
 wget

# Upgrade pip
python3 -m ensurepip --upgrade

# Installing meson via pip as the one available 'pkg install' currently that is 0.62.2, breaks with
# a "File name too long", 0.60 seems ok.
python3 -m pip install meson==0.60

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

meson setup builddir --prefix=/usr \
-Dc_args="-fPIC -Wno-error" \
-Dcpu_instruction_set=native \
-Ddisable_libs=bitratestats,cfgfile,flow_classify,gpudev,gro,gso,jobstats,kni,latencystats,metrics,node,pdump,pipeline,port,table \
-Ddisable_apps=pdump,test-cmdline,test,test-regex,test-gpudev,test-fib,test-pipeline,test-sad,test-acl,test-bbdev,proc-info,test-eventdev,test-flow-perf,test-pmd,dumpcap,test-compress-perf,test-crypto-perf,test-security-perf \
-Denable_docs=false \
-Denable_drivers=bus,bus/pci,bus/vdev,mempool/ring \
-Denable_kmods=true \
-Dtests=false


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

gmake
gmake install

# SPDK doesn't install to the correct pkg-config directory - thus they need to be moved manually
# Alternatively use the command 'export PKG_CONFIG_PATH=/usr/libdata/pkgconfig'

cp /usr/lib/pkgconfig/spdk* /usr/libdata/pkgconfig/

popd

