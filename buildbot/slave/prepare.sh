# This file is included by the other shell scripts to set up some variables.
# Each of the scripts (including this) gets 2 arguments: <config> <branch>

# It sets the following variables:
# - CONFIG   : configuration (default, debug2, syncdebug, etc.)
# - BRANCH   : branch (master, etc.)
# - CONFIG_  : $CONFIG wrapped in [] or empty if CONFIG=default
# - BRANCH_  : $BRANCH wrapped in {} or empty if BRANCH=master
# - REV      : output of `git describe --tags'
# - SOURCEDIR: absolute path to the source directory
# - BUILDDIR : absolute path to the build directory
# - TMP_BASE : folder for temporary work items
# - TMP_PATH : $TMP_BASE/$CONFIG/$BRANCH/$REV

# Quit on error.
set -e

CONFIG=${1:-default}
BRANCH=${2:-master}
shift 2

REV=$(git describe --tags)
SOURCEDIR=${PWD}
BUILDDIR=${PWD}/build/${CONFIG}
TMP_BASE=/tmp/spring
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}/${REV}

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

VERSION="${CONFIG_}${BRANCH_}${REV}"
