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
make install-spring-headless DESTDIR=${TESTDIR}

# HACK/FIXME force spring to detect install dir as read-only
find ${TESTDIR} -type d |xargs chmod 555

cd ${SOURCEDIR}

function download_file {
	OUTPUT=$1
	URL=$2
	OUTPUTDIR=$(dirname $OUTPUT)
	if [ ! -d $OUTPUTDIR ]; then
		mkdir -p $OUTPUTDIR
	fi
	if [ ! -s $OUTPUT ]; then
		wget -N -P $OUTPUTDIR $URL || ( rm -rf ${OUTPUTDIR} && exit 1 )
	fi
}

function makescript {
	GAME=$1
	MAP=$2
	AI=$3
	AIVERSION=$4
	OUTPUT=${CONTENT_DIR}/$AI.script.txt
	echo "Creating script: test/validation/prepare.sh \"$GAME\" \"$MAP\" \"$AI\" \"$AIVERSION\""
	${SOURCEDIR}/test/validation/prepare.sh "$GAME" "$MAP" "$AI" "$AIVERSION" > "$OUTPUT"
}

download_file $DOWNLOADDIR/ba760.sdz http://springfiles.com/sites/default/files/downloads/spring/games/ba760.sdz
download_file $DOWNLOADDIR/Altair_Crossing.sd7 http://springfiles.com/sites/default/files/downloads/spring/spring-maps/Altair_Crossing.sd7
download_file $DOWNLOADDIR/zk-v0.8.14.sdz http://packages.springrts.com/builds/zk-v0.8.14.sdz

#install required files into spring dir
cd ${SOURCEDIR}
mkdir -p ${CONTENT_DIR}/games ${CONTENT_DIR}/maps ${CONTENT_DIR}/LuaUI/Widgets ${CONTENT_DIR}/LuaUI/Config

#symlink files into into destination dir
cp -suv ${DOWNLOADDIR}/ba760.sdz ${CONTENT_DIR}/games/ba760.sdz
cp -suv ${DOWNLOADDIR}/zk-v0.8.14.sdz ${CONTENT_DIR}/games/zk-v0.8.14.sdz
cp -suv ${DOWNLOADDIR}/Altair_Crossing.sd7 ${CONTENT_DIR}/maps/Altair_Crossing.sd7

#copy widget + config
cp -suv ${SOURCEDIR}/test/validation/LuaUI/Widgets/test.lua ${CONTENT_DIR}/LuaUI/Widgets/test.lua
cp -v ${SOURCEDIR}/test/validation/LuaUI/Config/ZK_data.lua ${CONTENT_DIR}/LuaUI/Config/ZK_data.lua

#copy default config for spring-headless
cp -v ${SOURCEDIR}/cont/springrc-template-headless.txt ${TESTDIR}/.springrc

#set data directory to test directory
echo "SpringData = ${TESTDIR}/usr/local/share/games/spring" >> ${TESTDIR}/.springrc


GAME1="Balanced Annihilation V7.60"
GAME2="Zero-K v0.8.14"
MAP="Altair_Crossing-V1"

makescript "$GAME1" "$MAP" AAI 0.9
makescript "$GAME1" "$MAP" E323AI 3.25.0
makescript "$GAME1" "$MAP" KAIK 0.13
makescript "$GAME1" "$MAP" RAI 0.601
makescript "$GAME1" "$MAP" Shard dev
makescript "$GAME2" "$MAP" CAI ""

