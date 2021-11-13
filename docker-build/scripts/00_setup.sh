SPRINGRTS_GIT_URL="https://github.com/beyond-all-reason/spring"
SPRINGRTS_AUX_URL_PREFIX="https://github.com/beyond-all-reason"
BRANCH_NAME="BAR105"
PLATFORM="windows-64"
DUMMY=
ENABLE_CCACHE=1
STRIP_SYMBOLS=1

MYARCHTUNE=""
MYCFLAGS=""
MYRWDIFLAGS="-O3 -g -DNDEBUG"

SPRING_DIR="/spring"
BUILD_DIR="/spring/build"
PUBLISH_DIR="/publish"


while getopts :b:u:a:p:dc:h:r:f:s: flag
do
    case "${flag}" in
        b) BRANCH_NAME=${OPTARG};;
        u) SPRINGRTS_GIT_URL=${OPTARG};;
        a) SPRINGRTS_AUX_URL_PREFIX=${OPTARG};;
        p) PLATFORM=${OPTARG};;
        d) DUMMY=1;;
        h) ENABLE_CCACHE=${OPTARG};;
        c) MYARCHTUNE=${OPTARG};;
        r) MYRWDIFLAGS=${OPTARG};;
        f) MYCFLAGS=${OPTARG};;
        s) STRIP_SYMBOLS=${OPTARG};;
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
echo "Strip debug symbols: ${STRIP_SYMBOLS}"
echo "---------------------------------"
