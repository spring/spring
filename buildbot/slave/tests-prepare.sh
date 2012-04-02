#!/bin/bash

# installs spring-headless, downloads some content files and
# runs a simple high level tests of the engine (AI vs AI)

set -e
. buildbot/slave/prepare.sh

TESTDIR=${TMP_BASE}/tests
DOWNLOADDIR=${TMP_BASE}/download
CONTENT_DIR=${TESTDIR}/.spring

if [ ! -d test/validation/ ];
then
	echo "$0 has to be run from the source root dir"
	exit 1
fi

#install
cd ${BUILDDIR}
make install-spring-headless demotool install-pr-downloader DESTDIR=${TESTDIR}

# HACK/FIXME force spring to detect install dir as read-only
chmod 555 ${TESTDIR}/usr/local/share/games/spring

cd ${SOURCEDIR}

function makescript {
	GAME=$1
	MAP=$2
	AI=$3
	AIVERSION=$4
	OUTPUT=${CONTENT_DIR}/$AI.script.txt
	echo "Creating script: test/validation/prepare.sh \"$GAME\" \"$MAP\" \"$AI\" \"$AIVERSION\""
	${SOURCEDIR}/test/validation/prepare.sh "$GAME" "$MAP" "$AI" "$AIVERSION" > "$OUTPUT"
}

GAME1="Balanced Annihilation V7.68"
GAME2="Zero-K v1.0.4.0"
MAP="Altair_Crossing-V1"

PRDL="${TESTDIR}/usr/local/bin/pr-downloader --filesystem-writepath=$DOWNLOADDIR"

$PRDL --download-game "$GAME1"
$PRDL --download-game "$GAME2"
$PRDL --download-map "$MAP"

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
cp -v ${SOURCEDIR}/cont/springrc-template-headless.txt ${TESTDIR}/.springrc

#set data directory to test directory
echo "SpringData = ${TESTDIR}/usr/local/share/games/spring" >> ${TESTDIR}/.springrc

makescript "$GAME1" "$MAP" AAI 0.9
makescript "$GAME1" "$MAP" E323AI 3.25.0
makescript "$GAME1" "$MAP" KAIK 0.13
makescript "$GAME1" "$MAP" RAI 0.601
makescript "$GAME1" "$MAP" Shard dev
makescript "$GAME2" "$MAP" CAI ""
${SOURCEDIR}/test/validation/prepare-client.sh ValidationClient 127.0.0.1 8452 >${CONTENT_DIR}/connect.txt

