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

cd ${SOURCEDIR}
#fetch required files
mkdir -p $DOWNLOADDIR
if [ ! -s $DOWNLOADDIR/ba760.sdz ]; then
	wget -N -P $DOWNLOADDIR http://springfiles.com/sites/default/files/downloads/spring/games/ba760.sdz || ( rm -rf ${DOWNLOADDIR} && exit 1 )
fi

if [ ! -s $DOWNLOADDIR/Altair_Crossing.sd7 ]; then
	wget -N -P $DOWNLOADDIR http://springfiles.com/sites/default/files/downloads/spring/spring-maps/Altair_Crossing.sd7 || ( rm -rf ${DOWNLOADDIR} && exit 1 )
fi

#install required files into spring dir
cd ${SOURCEDIR}
mkdir -p ${CONTENT_DIR}/games ${CONTENT_DIR}/maps ${CONTENT_DIR}/LuaUI/Widgets
cp -suv ${DOWNLOADDIR}/ba760.sdz ${CONTENT_DIR}/games/ba760.sdz
cp -suv ${DOWNLOADDIR}/Altair_Crossing.sd7 ${CONTENT_DIR}/maps/Altair_Crossing.sd7
cp -suv ${SOURCEDIR}/test/validation/LuaUI/Widgets/test.lua ${CONTENT_DIR}/LuaUI/Widgets/test.lua
cp -v ${SOURCEDIR}/cont/springrc-template-headless.txt ${TESTDIR}/.springrc
echo "SpringData = ${TESTDIR}/usr/local/share/games/spring" >> ${TESTDIR}/.springrc

function makescript {
	${SOURCEDIR}/test/validation/prepare.sh $1 $2 > ${CONTENT_DIR}/$1.script.txt
}

makescript AAI 0.9
makescript E323AI 3.25.0
makescript KAIK 0.13
makescript RAI 0.601
makescript Shard dev
