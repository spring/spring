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
cd ${INSTALLDIR}


BINARIES="spring* lib*.so pr-downloader $(find AI/Skirmish -name libSkirmishAI.so) $(find AI/Interfaces -name libAIInterface.so)"

#strip symbols into a separate file
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

# not all distros have a libSDL-1.2.so.0 link, so better link to the always existing libSDL.so
# note, cmake says it links to /usr/lib/libSDL.so. But the final binary links then libSDL-1.2.so.0, so either cmake or gcc/ld fails.
for binary in ${BINARIES}; do
	if [ -f ${binary} ]; then
		if readelf -h ${binary} &> /dev/null; then
			if ! objdump -h ${binary} | grep -q .gnu_debuglink; then
				echo "fix libSDL.so linking in ${binary}"
				SDL_LIB_NAME_VER="libSDL-1.2.so.0"
				SDL_LIB_NAME_FIX="libSDL.so\x0\x0\x0\x0\x0\x0"
				sed -i "s/${SDL_LIB_NAME_VER}/${SDL_LIB_NAME_FIX}/g" ${binary}
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
			archive_debug="${TMP_PATH}/${VERSION}_${name}_${FILEPREFIX}_dbg.7z"
			[ -f ${debugfile} ] && ${SEVENZIP} "${archive_debug}" ${debugfile} && rm ${debugfile}
		fi
	fi
done


#absolute path to the minimal portable (engine, unitsync + ais)
MIN_PORTABLE_ARCHIVE=${TMP_PATH}/spring_${VERSION}_minimal-portable-${FILEPREFIX}.7z

#create portable spring excluding shard (ask AF why its excluded)
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${MIN_PORTABLE_ARCHIVE} ${INSTALLDIR}/* -xr!spring-dedicated* -xr!spring-headless* -xr!*.dbg -xr!*.dbg.7z

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
