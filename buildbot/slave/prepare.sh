# This file is included by the other shell scripts to set up some variables.
# Each of the scripts (including this) gets 2 arguments: <config> <branch>

# It sets the following variables:
# - CONFIG   : configuration (default, debug, syncdebug, etc.)
# - BRANCH   : branch (master, etc.)
# - CONFIG_  : $CONFIG wrapped in [] or empty if CONFIG=default
# - BRANCH_  : $BRANCH wrapped in {} or empty if BRANCH=master
# - REV      : output of `git describe --tags'
# - SOURCEDIR: absolute path to the source directory
# - BUILDDIR : absolute path to the build directory
# - TMP_BASE : folder for temporary work items
# - TMP_PATH : $TMP_BASE/$CONFIG/$BRANCH/$REV
# - VERSION_ : version string, unsafe, may include special characters
# - VERSION  : sanitized version string, suitable for filenames

# Quit on error.
set -e

if [ ! -n "$1" ] && [ ! -n "$2" ]; then
	echo "Please run with $0 <config> <branch>"
	exit 1
fi


CONFIG=${1:-default}
BRANCH=${2:-develop}
shift 2

REV=$(git describe --tags)
SOURCEDIR=${PWD}
BUILDDIR=${PWD}/build/${CONFIG}
TMP_BASE=/tmp/spring
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}/${REV}/$OUTPUTDIR

if [ x${CONFIG} = xdefault ]; then
   CONFIG_=''
else
   CONFIG_="[${CONFIG}]"
fi

if [ x${BRANCH} = xmaster ]; then
   BRANCH_=''
else
   BRANCH_="{${BRANCH}}"
fi

VERSION_="${CONFIG_}${BRANCH_}${REV}"
VERSION=`echo "${VERSION_}" | tr '<>:\"/\\|?*' -`

if [ -z "${MAKE}" ]; then
	echo "MAKE isn't set, using 'make' as default"
	MAKE=make
fi

