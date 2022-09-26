set -e

if [ ! ${LOCAL_BUILD} ]; then
    git config --global user.name  "Docker SpringRTS Build"
    git config --global user.email "dockerbuild@beyondallreason.info"

    rm -rf "${SPRING_DIR}"

    echo "---------------------------------"
    echo "Cloning SpringRTS from: ${SPRINGRTS_GIT_URL}"
    echo "Using branch: ${BRANCH_NAME}"
    echo "---------------------------------"

    # Do not use git depth parameter cause git describe later will not work as expected
    CMD="git clone "${SPRINGRTS_GIT_URL}" "${SPRING_DIR}""

    echo "Command: ${CMD}"

    ${CMD}

    cd "${SPRING_DIR}"
    git checkout "${BRANCH_NAME}"
    git submodule update --init --recursive
fi

if [ "${PLATFORM}" == "windows-64" ]; then
    LIBS_DIR="/mingwlibs64"
    LIBS_BRANCH=master
elif [ "${PLATFORM}" == "linux-64" ]; then
    LIBS_DIR="/spring-static-libs"
    LIBS_BRANCH=18.04
fi

if [ ! -d "${LIBS_DIR}/.git" ]; then
    git clone "${SPRINGRTS_AUX_URL_PREFIX}${LIBS_DIR}.git" -b "${LIBS_BRANCH}" --depth 1 "$LIBS_DIR"
else
    cd "${LIBS_DIR}"
    git pull
    git checkout ${LIBS_BRANCH}
fi
