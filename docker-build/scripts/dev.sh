set -e

# make all setup variables available on the shell later
set -a 
. /scripts/00_setup.sh
set +a

. /scripts/01_clone.sh
. "/scripts/02_configure_${PLATFORM}.sh"

echo "----------------------------------------------"
echo "SpringRTS development environment has been set up"
echo "Source code directory: ${SPRING_DIR}"
echo "Build directory: ${BUILD_DIR}"
echo "----------------------------------------------"

cd "${BUILD_DIR}"

/bin/bash
