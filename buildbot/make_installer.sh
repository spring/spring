#!/bin/bash
. prepare.sh

MINGWLIBS_PATH=${3}
MINGW_HOST=i586-mingw32msvc-
TMP_BASE=/tmp/spring
TMP_PATH=${TMP_BASE}/${CONFIG}/${BRANCH}

echo "!define MINGWLIBS_DIR \"${MINGWLIBS_PATH}\"" > installer/custom_defines.nsi
echo "!define BUILD_DIR \"${BUILDDIR}\"" >> installer/custom_defines.nsi

#strip symbols and archive them
cd ${BUILDDIR}
for tostripfile in spring.exe spring-dedicated.exe spring-gml.exe spring-hl.exe unitsync.dll $(find AI/Skirmish -name SkirmishAI.dll); do
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
cd $OLDPWD

# The 0 means: do not define DIST_DIR
./installer/make_installer.pl 0

mkdir -p ${TMP_PATH}
mv ./installer/spring*.exe ${TMP_PATH}
