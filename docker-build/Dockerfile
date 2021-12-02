FROM ubuntu:18.04 AS mxe

ARG mxe_version=02852a7b690aa411ce2a2089deea25a7292a33d6
ARG mxe_gcc=gcc11

# Install MXE dependencies
RUN apt-get update;apt-get install -y \
        autoconf \
        automake \
        autopoint \
        bash \
        bison \
        bzip2 \
        flex \
        g++ \
        g++-multilib \
        gettext \
        git \
        gperf \
        intltool \
        libc6-dev-i386 \
        libgdk-pixbuf2.0-dev \
        libltdl-dev \
        libssl-dev \
        libtool-bin \
        libxml-parser-perl \
        lzip \
        make \
        openssl \
        p7zip-full \
        patch \
        perl \
        python \
        ruby \
        sed \
        unzip \
        wget \
        xz-utils

# Install MXE and build cross-compile windows gcc
RUN cd /; \
    git clone https://github.com/mxe/mxe.git; \
    cd mxe; \
    git checkout $mxe_version; \
    make cc -j$(nproc) MXE_USE_CCACHE= MXE_PLUGIN_DIRS=plugins/$mxe_gcc MXE_TARGETS=x86_64-w64-mingw32.static; \
    rm -rf /mxe/pkg /mxe/src /mxe/log /mxe/.git

# Main Image
FROM ubuntu:18.04

ARG cmake_version=3.16.*
ARG ccache_version="v4.5.1"

COPY --from=mxe /mxe /mxe

ENV PATH="/mxe/usr/bin:${PATH}"

# suppress questions from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install Build Dependencies (Common)
RUN apt-get update -y; \
    apt-get install --fix-missing -y \
        clang-format \
        coreutils \
        git \
        ninja-build \
        p7zip-full \
        pigz \
        software-properties-common \
        python3-pip; \
        pip3 install cmake==$cmake_version; \
        apt purge python3-pip -y; apt auto-remove -y

# GCC-10 PPA (linux-64)
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y; \
    apt update -y; \
    apt install -y gcc-10 g++-10

# Install Build Dependencies (linux-64)
RUN apt-get install --fix-missing -y \
        libsdl2-dev \
        libopenal-dev \
        libfreetype6-dev \
        libfontconfig1-dev

# Compile and install ccache
RUN git clone https://github.com/ccache/ccache.git /tmp/ccache/src; \
    cd /tmp/ccache/src; \
    git checkout $ccache_version; \
    mkdir -p build; \
    cd build; \
    cmake .. -DHIREDIS_FROM_INTERNET=ON -DZSTD_FROM_INTERNET=ON -DCMAKE_INSTALL_PREFIX=/tmp/ccache/bin -DCMAKE_INSTALL_SYSCONFDIR=/etc -DENABLE_DOCUMENTATION= -DENABLE_TESTING=; \
    make -j$(nproc); \
    make install; \
    cp /tmp/ccache/bin/bin/ccache /usr/bin; \
    rm -rf /tmp/ccache

# Setup ccache
ENV CCACHE_DIR=/ccache
ENV CCACHE_CONFIGPATH=/etc/ccache.conf
ENV PATH="/usr/lib/ccache:${PATH}"

COPY scripts /scripts
ENV PATH="/scripts:${PATH}"

# set primary config so no config from cache directory will be used
COPY config/ccache.conf /etc/ccache.conf

RUN mkdir -p /usr/lib/ccache; \
        /scripts/update-ccache-symlinks.sh; \
        ln -s ../../bin/ccache /usr/lib/ccache/x86_64-w64-mingw32.static-g++; \
        ln -s ../../bin/ccache /usr/lib/ccache/x86_64-w64-mingw32.static-c++; \
        ln -s ../../bin/ccache /usr/lib/ccache/x86_64-w64-mingw32.static-gcc

VOLUME /publish

ENTRYPOINT ["/bin/bash", "/scripts/entrypoint.sh"]
