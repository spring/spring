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

#create portable spring excluding shard (ask AF why its excluded)
touch ${INSTALLDIR}/springsettings.cfg
${SEVENZIP} ${TMP_PATH}/spring_${VERSION}_portable.7z ${INSTALLDIR}/* -x!spring-dedicated.exe -x!spring-headless.exe -x!ArchiveMover.exe -xr!*.dbg -x!AI/Skirmish/Shard
# compress files excluded from portable archive
for file in spring-dedicated.exe spring-headless.exe ArchiveMover.exe; do
	name=${file%.*}
	${SEVENZIP} ${TMP_PATH}/${VERSION}_${name}.7z ${file}
done

#compress shard
${SEVENZIP} ${TMP_PATH}/${VERSION}_Shard.7z AI/Skirmish/Shard -xr!*.dbg

#create archives for translate_stacktrace.py
for tocompress in ${EXECUTABLES}; do
	#get parent-parent-directory name of file
	name=$(basename $(dirname $(dirname ${tocompress})))

	#set to filename without suffix if no parent dir
	if [ ${name} == "." ]; then
		name=${tocompress%.*}
	fi
	debugfile=${tocompress%.*}.dbg
	archive_debug="${TMP_PATH}/${VERSION}_${name}_dbg.7z"
	[ ! -f ${debugfile} ] || ${SEVENZIP} "${archive_debug}" ${debugfile}
done

cd ${SOURCEDIR}

echo "!define MINGWLIBS_DIR \"${MINGWLIBS_PATH}\"" > installer/custom_defines.nsi
echo "!define BUILD_DIR \"${BUILDDIR}\"" >> installer/custom_defines.nsi

# The 0 means: do not define DIST_DIR
./installer/make_installer.pl 0

mv ./installer/spring*.exe ${TMP_PATH}

