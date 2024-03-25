
#!/usr/bin/env bash
#
# These are the tools needed for generating the docs
#

export DEBIAN_FRONTEND=noninteractive
export DEBIAN_PRIORITY=critical
apt-get -qy update
apt-get -qy \
        -o "Dpkg::Options::=--force-confdef" \
        -o "Dpkg::Options::=--force-confold" upgrade
apt-get -qy autoclean
apt-get -qy install \
        bash \
        doxygen \
        universal-ctags \
        git \
        graphviz \
        pkg-config \
        python3 \
