# This file is included by the other shell scripts to set up some variables.
# Each of the scripts (including this) gets 2 arguments: <config> <branch>

# It sets the following variables:
# - CONFIG   : configuration (default, debug2, syncdebug, etc.)
# - BRANCH   : branch (master, etc.)
# - CONFIG_  : $CONFIG wrapped in [] or empty if CONFIG=default
# - BRANCH_  : $BRANCH wrapped in {} or empty if BRANCH=master
# - REV      : output of `git describe --tags'
# - BUILDDIR : absolute path to the build directory

# Quit on error.
set -e

CONFIG=${1:-default}
BRANCH=${2:-master}
REV=$(git describe --tags)
BUILDDIR=${PWD}/build-${CONFIG}

if [ ${CONFIG} == default ]; then
   CONFIG_=''
else
   CONFIG_="[${CONFIG}]"
fi

if [ ${BRANCH} == master ]; then
   BRANCH_=''
else
   BRANCH_="{${BRANCH}}"
fi

VERSION="${CONFIG_}${BRANCH_}${REV}"
