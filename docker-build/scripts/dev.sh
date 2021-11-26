if [ $$ -gt 1 ] && [ $PPID -gt 1 ]; then
    echo "Are you already in a dev shell? Please start this from the initial shell. Exiting..."
    exit 1
fi

# export variables to make them vavailable to the interactive shell later
set -a
. /scripts/00_setup.sh
set +a

/scripts/01_clone.sh

ERROR=$?
if [ $? -eq 0 ]; then
    /scripts/02_configure.sh
    ERROR=$?
fi

echo "----------------------------------------------"
if [ $ERROR -eq 0 ]; then
    echo "SpringRTS development environment has been set up successfully"
else
    printf "\n!!!!!! ERROR: SpringRTS development environment could not be setup successfully !!!!!!\n\n"
fi
echo "Source code directory: ${SPRING_DIR}"
echo "Build directory: ${BUILD_DIR}"
echo "----------------------------------------------"


cd "${BUILD_DIR}"

/bin/bash --rcfile <(cat ~/.bashrc; echo 'PS1="<springdev> \[\e]0;\u@\h: \w\a\]${debian_chroot:+($debian_chroot)}\u@\h:\w\$ "')
