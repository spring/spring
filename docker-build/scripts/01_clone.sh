set -e

cd /
UBUNTU_VER=18.04

# Do not use git depth parameter cause git describe later will not work as expected

git config --global user.name  "Docker SpringRTS Build"
git config --global user.email "dockerbuild@beyondallreason.info"

rm -rf "${SPRING_DIR}"

echo "---------------------------------"
echo "Cloning SpringRTS from: ${SPRINGRTS_GIT_URL}"
echo "Using branch: ${BRANCH_NAME}"
echo "---------------------------------"

CMD="git clone "${SPRINGRTS_GIT_URL}" "${SPRING_DIR}""

echo "Command: ${CMD}"

${CMD}

cd "${SPRING_DIR}"
git checkout "${BRANCH_NAME}"
git submodule update --init --recursive

if [ "${PLATFORM}" == "windows-64" ]; then
    git clone "${SPRINGRTS_AUX_URL_PREFIX}/mingwlibs64.git" mingwlibs64
elif [ "${PLATFORM}" == "linux-64" ]; then
    git clone "${SPRINGRTS_AUX_URL_PREFIX}/spring-static-libs.git" -b $UBUNTU_VER spring-static-libs
else
    echo "Unsupported platform: '${PLATFORM}'"
    exit 1
fi

cd "${SPRING_DIR}"
