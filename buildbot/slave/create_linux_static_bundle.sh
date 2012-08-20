#!/bin/bash
set -e
. buildbot/slave/prepare.sh

FILEPREFIX="linux-static"

DEST=${TMP_BASE}/inst
INSTALLDIR=${DEST}

echo "Installing into $DEST"

#Ultra settings, max number of threads taken from commandline.
SEVENZIP="nice -19 ionice -c3 7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on -mmt=${2:-on}"
ZIP="zip -r9"

cd ${BUILDDIR}
make install DESTDIR=${DEST}

#strip symbols and archive them
cd ${INSTALLDIR}
BINARIES="spring* lib*.so pr-downloader"
for tostripfile in ${BINARIES}; do
	if [ -f ${tostripfile} ]; then
		if readelf -h ${tostripfile} &> /dev/null; then
			# dont strip binaries that we processed earlier
			if ! objdump -h ${tostripfile} | grep -q .gnu_debuglink; then
				echo "stripping ${tostripfile}"
				debugfile=${tostripfile%.*}.dbg
				objcopy --only-keep-debug ${tostripfile} ${debugfile}
				strip --strip-debug --strip-unneeded ${tostripfile}
				objcopy --add-gnu-debuglink=${debugfile} ${tostripfile}
			else
				echo "not stripping ${tostripfile}"
			fi
		fi
	fi
done

mkdir -p ${TMP_PATH}

#create archives for translate_stacktrace.py
for tocompress in ${BINARIES}; do
	if [ -f ${tocompress} ]; then
		if readelf -h ${tocompress} &> /dev/null; then
			#get parent-parent-directory name of file
			name=$(basename $(dirname $(dirname ${tocompress})))

			#set to filename without suffix if no parent dir
			if [ ${name} == "." ]; then
				name=${tocompress%.*}
			fi
			debugfile=${tocompress%.*}.dbg
			archive_debug="${TMP_PATH}/${VERSION}_${name}_dbg.7z"
			[ -f ${debugfile} ] && ${SEVENZIP} "${archive_debug}" ${debugfile} && rm ${debugfile}
		fi
	fi
done

#absolute path to the minimal portable (engine, unitsync + ais)
MIN_PORTABLE_ARCHIVE=${TMP_PATH}/spring_${VERSION}_minimal-portable-${FILEPREFIX}.7z
MIN_PORTABLE_ARCHIVE_ZIP=${TMP_PATH}/spring_${VERSION}_minimal-portable-${FILEPREFIX}.zip

#create portable spring excluding shard (ask AF why its excluded)
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${MIN_PORTABLE_ARCHIVE} ${INSTALLDIR}/* -xr!spring-dedicated* -xr!spring-headless* -xr!*.dbg -xr!*.dbg.7z
#for ZKL
#(cd ${INSTALLDIR} && ${ZIP} ${MIN_PORTABLE_ARCHIVE_ZIP} * -x spring-headless\* spring-dedicated\* \*.dbg \*.dbg.7z)

# compress files excluded from portable archive
for file in spring-dedicated spring-headless; do
	if [ -f ${file} ]; then
		name=${file%.*}
		${SEVENZIP} ${TMP_PATH}/${VERSION}_${name}-${FILEPREFIX}.7z ${file}
	fi
done

# create relative symbolic links to current files for rsyncing
cd ${TMP_PATH}/..
ln -sfv ${REV}/spring_${VERSION}_minimal-portable-${FILEPREFIX}.7z  spring_testing_minimal-portable-${FILEPREFIX}.7z
#ln -sfv ${REV}/spring_${VERSION}_minimal-portable-${FILEPREFIX}.zip spring_testing_minimal-portable-${FILEPREFIX}.zip

# create a file which contains the latest version of a branch
#echo ${VERSION} > LATEST

#cd ${BUILDDIR}
#make uninstall DESTDIR=${DEST}
