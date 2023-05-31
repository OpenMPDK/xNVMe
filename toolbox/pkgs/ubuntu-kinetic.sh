#!/bin/sh
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
 findutils \
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
 patch \
 pipx \
 pkg-config \
 python3 \
 python3-pip \
 python3-pyelftools \
 python3-venv \
 uuid-dev

#
# Clone, build and install libvfn
#
# Assumptions:
#
# - These commands are executed with sufficient privileges (sudo/root)
#
git clone https://github.com/OpenMPDK/libvfn.git
pushd libvfn
git checkout v1.0.0
meson setup builddir -Dlibnvme="disabled" -Ddocs="disabled" --prefix=/usr
meson compile -C builddir
meson install -C builddir
popd

