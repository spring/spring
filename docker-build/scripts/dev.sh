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
echo "Supported distros: Ubuntu 20.04"
echo "----------------------------------------------"


cd "${BUILD_DIR}"

# check if we are the container main process and start an interactive shell only then
if [ $$ -eq 1 ]; then
  /bin/bash
fi
