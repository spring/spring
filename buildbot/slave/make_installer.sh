#!/bin/bash
set -e
. buildbot/slave/prepare.sh


DEST=${TMP_BASE}/inst
#FIXME: remove hardcoded /usr/local
INSTALLDIR=${DEST}/usr/local

echo "Installing into $DEST"

#Ultra settings
SEVENZIP="7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on"

MINGWLIBS_PATH=${1}
MINGW_HOST=i586-mingw32msvc-

cd ${BUILDDIR}
make install DESTDIR=${DEST}

function zip() {
	file=$1
	name=$2
	if [ -f ${file} ]; then
		debugfile=${file%.*}.dbg
		archive="${TMP_PATH}/${VERSION}_${name}.7z"
		archive_debug="${TMP_PATH}/${VERSION}_${name}_dbg.7z"
		${SEVENZIP} "${archive}" ${file}
		[ ! -f ${debugfile} ] || ${SEVENZIP} "${archive_debug}" ${debugfile}
	fi
}

#strip symbols and archive them
cd ${INSTALLDIR}
EXECUTABLES="spring.exe spring-dedicated.exe spring-multithreaded.exe spring-headless.exe unitsync.dll ArchiveMover.exe $(find AI/Skirmish -name SkirmishAI.dll) $(find AI/Interfaces -name AIInterface.dll)"
for tostripfile in ${EXECUTABLES}; do
	if [ -f ${tostripfile} ]; then
		# dont strip binaries that we processed earlier
		if ! ${MINGW_HOST}objdump -h ${tostripfile} | grep -q .gnu_debuglink; then
			echo "stripping ${tostripfile}"
			debugfile=${tostripfile%.*}.dbg
			${MINGW_HOST}objcopy --only-keep-debug ${tostripfile} ${debugfile}
			${MINGW_HOST}strip --strip-debug --strip-unneeded ${tostripfile}
			${MINGW_HOST}objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}
		else
			echo "not stripping ${tostripfile}"
		fi
	fi
done

mkdir -p ${TMP_PATH}

#create portable spring
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${TMP_PATH}/${VERSION}_portable.7z ${INSTALLDIR}/* -x!AI -x!spring-dedicated.exe -x!spring-headless.exe -x!ArchiveMover.exe -x!*.dbg

#create archives
for tocompress in ${EXECUTABLES}; do
	zip ${tocompress} $(basename ${tocompress})
done

#create 7z's for AI's and Interface
for TYPE in $(find ${INSTALLDIR}/AI -maxdepth 1 -mindepth 1 -type d) ; do
	echo ${TYPE}
	type=$(basename ${TYPE})
	echo "Packing ${type}"
	for DIR in $(find ${TYPE} -maxdepth 1 -mindepth 1 -type d); do
		NAME=$(basename ${DIR})
		${SEVENZIP} ${TMP_PATH}/$(basename ${VERSION}_${type}-${NAME}.7z) ${DIR} -xr!*.dbg
	done
done

cd ${SOURCEDIR}

echo "!define MINGWLIBS_DIR \"${MINGWLIBS_PATH}\"" > installer/custom_defines.nsi
echo "!define BUILD_DIR \"${BUILDDIR}\"" >> installer/custom_defines.nsi

# The 0 means: do not define DIST_DIR
./installer/make_installer.pl 0

mv ./installer/spring*.exe ${TMP_PATH}

