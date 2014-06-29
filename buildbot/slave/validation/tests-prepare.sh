#!/bin/bash

# downloads game/map and creates script.txt

set -e
. buildbot/slave/validation/tests-env.sh

mkdir -p "${DOWNLOADDIR}" "${CONTENT_DIR}/LuaUI/Widgets" "${CONTENT_DIR}/LuaUI/Config"

PRDL="time ${TESTDIR}/usr/local/bin/pr-downloader --filesystem-writepath=$DOWNLOADDIR"
# get the name of the latest versions
GAME1=$($PRDL --download-game "$GAME" |egrep -o '\[Download\] (.*)' |cut -b 12-)
$PRDL --download-map "$MAP"

echo "Creating script: test/validation/prepare.sh \"$GAME1\" \"$MAP\" \"$AI\" \"$AIVER\""
${SOURCEDIR}/test/validation/prepare.sh "$GAME1" "$MAP" "$AI" "$AIVER" > ${CONTENT_DIR}/script.txt
${SOURCEDIR}/test/validation/prepare-client.sh ValidationClient localhost 8452 >${CONTENT_DIR}/connect.txt

#install required files into spring dir
cd ${SOURCEDIR}

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

