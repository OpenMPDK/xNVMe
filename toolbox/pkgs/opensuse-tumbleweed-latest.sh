#!/usr/bin/env bash
# Query the linker version
ld -v || true

# Query the (g)libc version
ldd --version || true

zypper --non-interactive refresh

# Install packages via the system package-manager (zypper)
zypper --non-interactive install -y --allow-downgrade \
 autoconf \
 awk \
 bash \
 clang-tools \
 cunit-devel \
 findutils \
 gcc \
 gcc-c++ \
 git \
 gzip \
 libaio-devel \
 libarchive-devel \
 libnuma-devel \
 libopenssl-devel \
 libtool \
 liburing-devel \
 libuuid-devel \
 make \
 meson \
 nasm \
 ncurses \
 patch \
 pkg-config \
 python3 \
 python3-devel \
 python3-pip \
 python3-pipx \
 python3-pyelftools \
 python3-setuptools \
 tar

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
git checkout v3.0.1
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --buildtype=release --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

#
# Clone, build and install libisal
#
# Assumptions:
#
# - Commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/intel/isa-l.git toolbox/third-party/libisal/repository

pushd toolbox/third-party/libisal/repository
git checkout v2.30.0
./autogen.sh
./configure --prefix=/usr --libdir=/usr/lib
make
make install
popd

