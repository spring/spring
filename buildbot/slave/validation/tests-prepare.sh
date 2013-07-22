#!/bin/bash

# installs spring-headless, downloads some content files and
# runs a simple high level tests of the engine (AI vs AI)

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
DOWNLOADDIR=${TMP_BASE}/download
CONTENT_DIR=${TESTDIR}/.config/spring

if [ ! -d test/validation/ ];
then
	echo "$0 has to be run from the source root dir"
	exit 1
fi

GAME=$1
MAP=$2
AI=$3
AIVER=$4

echo "Running test: GAME=$GAME MAP=$MAP AI=$AI AIVER=$AIVER"

#install
cd ${BUILDDIR}
DESTDIR=${TESTDIR} ${MAKE} install-spring-headless install-pr-downloader demotool lua2php

cd ${SOURCEDIR}

mkdir -p "${DOWNLOADDIR}"
mkdir -p "${CONTENT_DIR}"

PRDL="${TESTDIR}/usr/local/bin/pr-downloader --filesystem-writepath=$DOWNLOADDIR"
# get the name of the latest versions
GAME1=$($PRDL $GAME |egrep -o '\[Download\] (.*)' |cut -b 12-)
$PRDL "$MAP"

#install required files into spring dir
cd ${SOURCEDIR}
mkdir -p ${CONTENT_DIR}/LuaUI/Widgets ${CONTENT_DIR}/LuaUI/Config

#symlink files into into destination dir
for i in games maps pool packages;
do
	# delete existing destination dir
	rm -rf ${CONTENT_DIR}/$i
	ln -sfv ${DOWNLOADDIR}/$i ${CONTENT_DIR}/$i
done

#copy widget + config
cp -suv ${SOURCEDIR}/test/validation/LuaUI/Widgets/test.lua ${CONTENT_DIR}/LuaUI/Widgets/test.lua
cp -v ${SOURCEDIR}/test/validation/LuaUI/Config/ZK_data.lua ${CONTENT_DIR}/LuaUI/Config/ZK_data.lua

#copy default config for spring-headless
cp -v ${SOURCEDIR}/cont/springrc-template-headless.txt ${CONTENT_DIR}/springsettings.cfg

# adjust springsettings.cfg
(
	# set datadir
	echo "SpringData = ${TESTDIR}/usr/local/share/games/spring"
	# disable bandwith limits (for syncdebug)
	echo "LinkIncomingMaxPacketRate = 0"
	echo "LinkIncomingMaxWaitingPackets = 0"
	echo "LinkIncomingPeakBandwidth = 0"
	echo "LinkIncomingSustainedBandwidth = 0"
	echo "LinkOutgoingBandwidth = 0"
) >> ${CONTENT_DIR}/springsettings.cfg

echo "Creating script: test/validation/prepare.sh \"$GAME1\" \"$MAP\" \"$AI\" \"$AIVER\""
${SOURCEDIR}/test/validation/prepare.sh "$GAME1" "$MAP" "$AI" "$AIVER" > "$OUTPUT"

${SOURCEDIR}/test/validation/prepare-client.sh ValidationClient 127.0.0.1 8452 >${CONTENT_DIR}/connect.txt

