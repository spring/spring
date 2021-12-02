SPRINGRTS_GIT_URL="https://github.com/beyond-all-reason/spring"
SPRINGRTS_AUX_URL_PREFIX="https://github.com/beyond-all-reason"
BRANCH_NAME="BAR105"
PLATFORM="windows-64"
DUMMY=
ENABLE_CCACHE=1
DEBUG_CCACHE=
STRIP_SYMBOLS=1

MYARCHTUNE=""
MYCFLAGS=""
MYRWDIFLAGS="-O3 -g -DNDEBUG"

SPRING_DIR="/spring"
BUILD_DIR="/spring/build"
PUBLISH_DIR="/publish"
INSTALL_DIR="${BUILD_DIR}/install"


function print_usage() {
    echo "usage:"
    echo "  -b      git brach name"
    echo "  -u      URL of SpringRTS git repository"
    echo "  -a      URL prefix for auxiliary repositories"
    echo "  -p      platform to build"
    echo "  -d      dummy mode"
    echo "  -e      enable ccache"
    echo "  -c      archtune flags"
    echo "  -r      release with debug flags"
    echo "  -f      c/c++ flags"
    echo "  -s      strip debug symbols"
    echo "  -z      enable ccache debug"
    echo "  -h      print this help"
    exit 1
}

while getopts :b:u:a:p:dc:hr:f:s:z:e: flag
do
    case "${flag}" in
        b) BRANCH_NAME=${OPTARG};;
        u) SPRINGRTS_GIT_URL=${OPTARG};;
        a) SPRINGRTS_AUX_URL_PREFIX=${OPTARG};;
        p) PLATFORM=${OPTARG};;
        d) DUMMY=1;;
        h) print_usage;;
        e) ENABLE_CCACHE=${OPTARG};;
        c) MYARCHTUNE=${OPTARG};;
        r) MYRWDIFLAGS=${OPTARG};;
        f) MYCFLAGS=${OPTARG};;
        s) STRIP_SYMBOLS=${OPTARG};;
        z) DEBUG_CCACHE=${OPTARG};;
        \:) printf "argument missing from -%s option\n" $OPTARG >&2
            exit 2
            ;;
        \?) printf "unknown option: -%s\n" $OPTARG >&2
            exit 2
            ;;
    esac
done

if [ "${PLATFORM}" != "windows-64" ] && [ "${PLATFORM}" != "linux-64" ]; then
    echo "Unsupported platform: '${PLATFORM}'" >&2
    exit 1
fi

echo "---------------------------------"
echo "Starting buildchain with following configuration:"
echo "Platform: ${PLATFORM}"
echo "Using branch: ${BRANCH_NAME}"
echo "Use Spring Git URL: ${SPRINGRTS_GIT_URL}"
echo "Use Git URL prefix for auxiliary projects: ${SPRINGRTS_AUX_URL_PREFIX}"
echo "Local artifact publish dir: ${PUBLISH_DIR}"
echo "Local SpringRTS source dir: ${SPRING_DIR}"
echo "RELWITHDEBINFO compilation flags: ${MYRWDIFLAGS}"
echo "Archtune flags: ${MYARCHTUNE}"
echo "Extra compilation flags: ${MYCFLAGS}"
echo "Dummy mode: ${DUMMY}"
echo "Enable ccache: ${ENABLE_CCACHE}"
echo "Debug ccache: ${DEBUG_CCACHE}"
echo "Strip debug symbols: ${STRIP_SYMBOLS}"
echo "---------------------------------"

# configuring ccache
if [ "${ENABLE_CCACHE}" != "1" ]; then
    echo "Disabling ccache"
    export CCACHE_DISABLE=1
else
    echo "Using ccache in directory: ${CCACHE_DIR}"

    echo "Content of ccache links: $(ls -la /usr/lib/ccache)"

    if [ "${DEBUG_CCACHE}" == "1" ]; then
        echo "ccache debugging enabled"
        export CCACHE_DEBUG=1
        mkdir -p /ccache_dbg
    fi
fi



echo "---------------------------------"
echo "MXE compiler stats:"
stat /mxe/usr/bin/x86_64-w64-mingw32.static-g++
stat /mxe/usr/bin/x86_64-w64-mingw32.static-c++
stat /mxe/usr/bin/x86_64-w64-mingw32.static-gcc
stat /usr/bin/ccache
stat /usr/lib/ccache/x86_64-w64-mingw32.static-g++
stat /usr/lib/ccache/x86_64-w64-mingw32.static-c++
stat /usr/lib/ccache/x86_64-w64-mingw32.static-gcc
echo "---------------------------------"
