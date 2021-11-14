FROM ubuntu:20.04

# suppress questions from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install Build Dependencies (Common)
RUN  apt-get update -y; \
	apt-get install --fix-missing -y \
		  clang-format \
		  coreutils \
		  git \
		  ninja-build \
		  p7zip-full \
		  pigz \
		  software-properties-common \
		  ccache \
		  cmake

# Install Build Dependencies (windows-64)
RUN apt-get install --fix-missing -y \
    binutils-mingw-w64-x86-64 g++-mingw-w64-x86-64 gcc-mingw-w64-x86-64
		  
# GCC-10 PPA (linux-64)
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y; \
        apt update -y; \
        apt install -y gcc-10 g++-10

# Install Build Dependencies (linux-64)
RUN  apt-get install --fix-missing -y \
        libsdl2-dev \
        libopenal-dev \
        libfreetype6-dev \
        libfontconfig1-dev

# Setup ccache
ENV CCACHE_DIR=/ccache
ENV PATH="/usr/lib/ccache:${PATH}"

COPY config/ccache.conf /etc/ccache.conf
RUN /usr/sbin/update-ccache-symlinks; \
        ln -s ../../bin/ccache /usr/lib/ccache/x86_64-w64-mingw32-g++-posix; \
        ln -s ../../bin/ccache /usr/lib/ccache/x86_64-w64-mingw32-gcc-posix

COPY scripts /scripts
ENV PATH="/scripts:${PATH}"

VOLUME /publish

ENTRYPOINT ["/bin/bash", "/scripts/entrypoint.sh"]
