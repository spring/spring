#!/bin/bash
set -e
. buildbot/slave/prepare.sh


DEST=${TMP_BASE}/inst
#FIXME: remove hardcoded /usr/local
INSTALLDIR=${DEST}/usr/local

echo "Installing into $DEST"

#Ultra settings, max number of threads taken from commandline.
SEVENZIP="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -mmt=${2:-on}"
SEVENZIP_NONSOLID="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=off -mmt=${2:-on}"
ZIP="zip -r9"

if [ -z $MINGWLIBS_PATH ]; then
	echo 'MINGWLIBS_PATH is not set'
	exit 1
fi

#use host from env if set
if [ -z "$MINGW_HOST" ]; then
	MINGW_HOST=i586-mingw32msvc-
fi

cd ${BUILDDIR}
DESTDIR=${DEST} ninja install

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
			${MINGW_HOST}objcopy --only-keep-debug ${tostripfile} ${debugfile}
			${MINGW_HOST}strip --strip-debug --strip-unneeded ${tostripfile}
			${MINGW_HOST}objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}
			DEBUGFILES="${DEBUGFILES} ${debugfile}"
		else
			echo "not stripping ${tostripfile}"
		fi
	fi
done

mkdir -p ${TMP_PATH}

#absolute path to the minimal portable (engine, unitsync + ais)
MIN_PORTABLE_ARCHIVE=${TMP_PATH}/spring_${VERSION}_minimal-portable.7z
MIN_PORTABLE_PLUS_DEDICATED_ARCHIVE=${TMP_PATH}/spring_${VERSION}_minimal-portable+dedicated.zip

#create portable spring excluding shard (ask AF why its excluded)
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${MIN_PORTABLE_ARCHIVE} ${INSTALLDIR}/* -x!spring-dedicated.exe -x!spring-headless.exe -xr!*.dbg
#for ZKL
(cd ${INSTALLDIR} && ${ZIP} ${MIN_PORTABLE_PLUS_DEDICATED_ARCHIVE} * -x spring-headless.exe \*.dbg)

# compress files excluded from portable archive
for file in spring-dedicated.exe spring-headless.exe; do
	name=${file%.*}
	${SEVENZIP} ${TMP_PATH}/${VERSION}_${name}.7z ${file}
done

#create archive for translate_stacktrace.py
${SEVENZIP_NONSOLID} ${TMP_PATH}/${VERSION}_spring_dbg.7z ${DEBUGFILES}

cd ${SOURCEDIR}

# create symlinks required for building installer
rm -f ${SOURCEDIR}/installer/downloads/spring_testing_minimal-portable.7z
mkdir -p ${SOURCEDIR}/installer/downloads/
ln -sv ${MIN_PORTABLE_ARCHIVE} ${SOURCEDIR}/installer/downloads/spring_testing_minimal-portable.7z

# create installer
./installer/make_installer.sh

# move installer to rsync-directory
mv ./installer/spring*.exe ${TMP_PATH}

./installer/make_portable_archive.sh ${TMP_PATH}/spring*.exe ${TMP_PATH}

# create relative symbolic links to current files for rsyncing
cd ${TMP_PATH}/../..
ln -sfv ${REV}/$OUTPUTDIR/spring_${REV}.exe spring_testing.exe
ln -sfv ${REV}/$OUTPUTDIR/spring_${REV}_portable.7z spring_testing-portable.7z
ln -sfv ${REV}/$OUTPUTDIR/spring_${VERSION}_minimal-portable.7z spring_testing_minimal-portable.7z
ln -sfv ${REV}/$OUTPUTDIR/spring_${VERSION}_minimal-portable+dedicated.zip spring_testing_minimal-portable+dedicated.zip

# create a file which contains the latest version of a branch
echo ${VERSION} > LATEST

