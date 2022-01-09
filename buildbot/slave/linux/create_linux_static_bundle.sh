#!/bin/bash
set -e
. buildbot/slave/prepare.sh

FILEPREFIX="${OUTPUTDIR}-static"
PLATFORM=${OUTPUTDIR}

DEST=${TMP_BASE}/inst
INSTALLDIR=${DEST}

echo "Installing into $DEST"

#Ultra settings, max number of threads taken from commandline.
SEVENZIP="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -mmt=${2:-on}"
SEVENZIP_NONSOLID="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=off -mmt=${2:-on}"
ZIP="zip -r9"

cd ${BUILDDIR}
DESTDIR=${DEST} ${MAKE} install
cd ${INSTALLDIR}


EXECUTABLES="spring* lib*.so pr-downloader mapcompile mapdecompile $(find AI/Skirmish -name libSkirmishAI.so) $(find AI/Interfaces -name libAIInterface.so)"

DEBUGFILES=""

#strip symbols into a separate file
for tostripfile in ${EXECUTABLES}; do
	if [ -f ${tostripfile} ]; then
		if readelf -h ${tostripfile} &> /dev/null; then
			# dont strip binaries that we processed earlier
			if ! objdump -h ${tostripfile} | grep -q .gnu_debuglink; then
				echo "stripping ${tostripfile}"
				debugfile=${tostripfile%.*}.dbg
				objcopy --only-keep-debug ${tostripfile} ${debugfile}
				strip --strip-debug --strip-unneeded ${tostripfile}
				objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}
				DEBUGFILES="${DEBUGFILES} ${debugfile}"
			else
				echo "not stripping ${tostripfile}"
			fi
			# remove RPATH/RUNPATH
			chrpath --delete ${tostripfile}
		fi
	fi
done

mkdir -p ${TMP_PATH}

#create archive for translate_stacktrace.py
${SEVENZIP_NONSOLID} ${TMP_PATH}/${VERSION}_spring_dbg.7z ${DEBUGFILES}

#absolute path to the minimal portable (engine, unitsync + ais)
MIN_PORTABLE_ARCHIVE=${TMP_PATH}/spring_${VERSION}_minimal-portable-${FILEPREFIX}.7z

#create portable spring excluding shard (ask AF why its excluded)
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${MIN_PORTABLE_ARCHIVE} ${INSTALLDIR}/* -xr!*.dbg -xr!*.dbg.7z

# create relative symbolic links to current files for rsyncing
cd ${TMP_PATH}/../..
ln -sfv ${REV}/$OUTPUTDIR/spring_${VERSION}_minimal-portable-${FILEPREFIX}.7z  spring_testing_minimal-portable-${FILEPREFIX}.7z
echo ${VERSION} > LATEST_${PLATFORM}
