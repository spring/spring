#!/bin/sh

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
CONTENT_DIR=${TESTDIR}/.config/spring


#install
cd ${BUILDDIR}
DESTDIR=${TESTDIR} ${MAKE} install-spring-headless install-pr-downloader demotool lua2php

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

