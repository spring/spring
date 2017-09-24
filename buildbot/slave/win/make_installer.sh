#!/bin/bash
set -e
. buildbot/slave/prepare.sh


DEST=${TMP_BASE}/inst
INSTALLDIR=${DEST}
PLATFORM=$OUTPUTDIR

echo "Installing into $DEST"

#Ultra settings, max number of threads taken from commandline.
SEVENZIP="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -mmt=${2:-on}"
SEVENZIP_NONSOLID="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=off -mmt=${2:-on}"

if [ -z $MINGWLIBS_PATH ]; then
	echo 'MINGWLIBS_PATH is not set'
	exit 1
fi

#use host from env if set
if [ -z "$MINGW_HOST" ]; then
	MINGW_HOST=i586-mingw32msvc-
fi

cd ${BUILDDIR}
DESTDIR=${DEST} ${MAKE} install

#strip symbols and archive them
cd ${INSTALLDIR}
EXECUTABLES="$(find -name '*.exe' -printf ' %f') unitsync.dll springserver.dll $(find AI/Skirmish -name SkirmishAI.dll) $(find AI/Interfaces -name AIInterface.dll) $(find -name pr-downloader_shared.dll -printf ' %f')"
DEBUGFILES=""
for tostripfile in ${EXECUTABLES}; do
	if [ -f ${tostripfile} ]; then
		# dont strip binaries that we processed earlier
		if ! ${MINGW_HOST}objdump -h ${tostripfile} | grep -q .gnu_debuglink; then
			echo "stripping ${tostripfile}"
			debugfile=${tostripfile%.*}.dbg
			(${MINGW_HOST}objcopy --only-keep-debug ${tostripfile} ${debugfile} && \
			${MINGW_HOST}strip --strip-debug --strip-unneeded ${tostripfile} && \
			${MINGW_HOST}objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}) &
			DEBUGFILES="${DEBUGFILES} ${debugfile}"
		else
			echo "not stripping ${tostripfile}"
		fi
	fi
done

# strip UnitTests (don't store their debugsymbols in spring_dbg.7z)
for tostripfile in "${BUILDDIR}"/test/*.exe; do
	if [ -f ${tostripfile} ]; then
		${MINGW_HOST}strip --strip-debug --strip-unneeded ${tostripfile} &
	fi
done

# wait for finished stripping
wait

mkdir -p ${TMP_PATH}

#absolute path to the minimal portable (engine, unitsync + ais)
MIN_PORTABLE_ARCHIVE=${TMP_PATH}/spring_${VERSION}_${PLATFORM}-minimal-portable.7z
INSTALLER_FILENAME=${TMP_PATH}/spring_${VERSION}_${PLATFORM}.exe
DEBUG_ARCHIVE=${TMP_PATH}/${VERSION}_${PLATFORM}_spring_dbg.7z
UNITTEST_ARCHIVE=${TMP_PATH}/${VERSION}_${PLATFORM}_UnitTests.7z
MIN_PORTABLE_ZIP=${TMP_PATH}/spring_${VERSION}_${PLATFORM}_minimal-portable.zip

#create portable spring
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${MIN_PORTABLE_ARCHIVE} ${INSTALLDIR}/* -xr!*.dbg &

# compress UnitTests
${SEVENZIP} ${UNITTEST_ARCHIVE} "${BUILDDIR}"/test/*.exe &

# create archive for translate_stacktrace.py
${SEVENZIP_NONSOLID} ${DEBUG_ARCHIVE} ${DEBUGFILES} &

# wait for 7zip
wait

cd ${SOURCEDIR}

# create symlinks required for building installer
rm -f ${SOURCEDIR}/installer/downloads/spring_testing_minimal-portable.7z
mkdir -p ${SOURCEDIR}/installer/downloads/
ln -sv ${MIN_PORTABLE_ARCHIVE} ${SOURCEDIR}/installer/downloads/spring_testing_minimal-portable.7z

# create installer
./installer/make_installer.sh

# move installer to rsync-directory
mv ./installer/spring*.exe ${INSTALLER_FILENAME}

./installer/make_portable_archive.sh ${INSTALLER_FILENAME} ${TMP_PATH}

# create a file which contains the latest version of a branch
echo ${VERSION} > ${TMP_BASE}/${CONFIG}/${BRANCH}/LATEST_${PLATFORM}

