#!/bin/sh

DOCKER_BUILD_DIR="$(dirname -- "$( readlink -f -- "$0"; )";)"
ENGINE_GIT_DIR="$(dirname "${DOCKER_BUILD_DIR}")"
CACHE_DIR="${DOCKER_BUILD_DIR}/cache"
LINUX_LIBS_DIR="${CACHE_DIR}/spring-static-libs"
WINDOWS_LIBS_DIR="${CACHE_DIR}/mingwlibs64"
CCACHE_DIR="${CACHE_DIR}/ccache"
DOCKER_SCRIPTS_DIR="${DOCKER_BUILD_DIR}/scripts"

# Create directories on the host, otherwise docker would create them as root
mkdir -p "${LINUX_LIBS_DIR}"
mkdir -p "${WINDOWS_LIBS_DIR}"
mkdir -p "${CCACHE_DIR}"

docker run -it --rm                                      \
            -v /etc/passwd:/etc/passwd:ro                \
            -v /etc/group:/etc/group:ro                  \
            --user="$(id -u):$(id -g)"                   \
            -v "${ENGINE_GIT_DIR}":"/spring"             \
            -v "${LINUX_LIBS_DIR}":"/spring-static-libs" \
            -v "${WINDOWS_LIBS_DIR}":"/mingwlibs64"      \
            -v "${CCACHE_DIR}":"/ccache"                 \
            -v "${DOCKER_SCRIPTS_DIR}":"/scripts"        \
            bar-spring build -l "$@"
