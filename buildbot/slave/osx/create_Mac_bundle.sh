#!/bin/bash
set -e
. buildbot/slave/prepare.sh


DEST=${TMP_BASE}/inst
#FIXME: remove hardcoded /usr/local
INSTALLDIR=${DEST}/usr/local

echo "Installing into $DEST"

cd ${BUILDDIR}
make install DESTDIR=${DEST}

# The Spring.app bundle will have the following folder hierarchy:
# -> Contents
# ---> Info.plist
# ---> MacOS
# -----> executables
# ---> Resources
# -----> app icon
# ---> lib
# -----> Required libraries to run (from MacPorts)
# ---> share
# -----> game content, doc, etc.

BUNDLE_NAME=Spring_${VERSION}.app
BUNDLE_BASE=${TMP_PATH}/${BUNDLE_NAME}/Contents

echo "Creating ${BUNDLE_NAME}..."

MACPORTS_BASE=`which port | sed s/[/]bin[/]port//`
STRIP="strip -r -u"

mkdir -p ${BUNDLE_BASE}
echo "-- changing directory to ${BUNDLE_BASE}"
cd ${BUNDLE_BASE}

# insert the version number in the Info.plist file
echo "-- creating executable header file (Info.plist)"
cat ${SOURCEDIR}/installer/Mac/Info.plist | sed s/###VERSION###/${VERSION}/ > Info.plist

echo "-- creating folders MacOS and Resources"
mkdir MacOS Resources

echo "-- installing executables into folder: MacOS"
mv ${INSTALLDIR}/bin/* MacOS/

echo "-- installing application icon into folder: Resources"
cp ${SOURCEDIR}/installer/Mac/spring.icns Resources/

echo "-- moving library folder into folder: lib"
mv ${INSTALLDIR}/lib .

echo "-- moving unitsync library to folder: MacOS (due to spring's relative pathing)"
mv lib/libunitsync.dylib MacOS/

# not needed (yet) for the server lib, uncomment if necessary
#mv lib/libspringserver.dylib MacOS/

echo "-- moving game content into folder: share"
mv ${INSTALLDIR}/share .

echo "-- copying 3rd-party libraries into folder: lib (provided by MacPorts)"
for executable in `ls MacOS`
do
	echo "--- copying libraries for ${executable}"
	
	# for each MacPorts's dylib required by the executable
	for dylib in `otool -L MacOS/${executable} | grep ${MACPORTS_BASE} | egrep -o lib.*dylib`
	do
		# dylib =~ "lib/lib*.dylib"

		echo "---- copying ${MACPORTS_BASE}/${dylib} to folder: lib"
		cp ${MACPORTS_BASE}/${dylib} lib

		# strip down lib's subdirectories (if any)
		dylib=lib/`echo ${dylib} | egrep -o [^/]*$`

		echo "---- taking write permissions on ${dylib}"
		chmod u+w ${dylib}

		# change the bundled lib's id to relative pathing mode (not necessary but cleaner)
		install_name_tool -id @loader_path/../${dylib} ${dylib}

		echo "---- telling ${executable} how to find ${dylib}"
		# point the executable to the bundled lib in relative pathing mode
		install_name_tool -change ${MACPORTS_BASE}/${dylib} @loader_path/../${dylib} MacOS/${executable}

		# the bundled lib is autonomous => it can be stripped
		echo "---- stripping down ${dylib}"
		${STRIP} ${dylib}
	done

	echo "--- stripping down ${executable}"
	${STRIP} MacOS/${executable}
done

# continue with recursive dependencies
echo "-- copying other required 3rd-party libraries into folder: lib"
end=0
while [ $end -eq 0 ]
do
	# don't loop unless necessary
	end=1

	# for each dylib already bundled
	for dylib in `ls lib`
	do
		# check if all dependent MacPorts libs are also bundled
		echo "--- looking for missing dependencies of lib/${dylib}"
		for requiredlib in `otool -L lib/${dylib} | grep ${MACPORTS_BASE} | egrep -o lib.*dylib`
		do
			# strip down lib's subdirectories (if any)
			locallib=lib/`echo ${requiredlib} | egrep -o [^/]*$`

			echo "---- scanning dependency: ${locallib}"

			if [ ! -f ${locallib} ]
			then
				# loop
				end=0

				echo "---- copying ${MACPORTS_BASE}/${requiredlib} to folder: lib"
				cp ${MACPORTS_BASE}/${requiredlib} lib

				echo "---- taking write permissions on ${locallib}"
				chmod u+w ${locallib}

				# change the bundled lib's id to relative pathing mode (not necessary but cleaner)
				install_name_tool -id @loader_path/../${locallib} ${locallib}

				# the bundled lib is autonomous => it can be stripped
				echo "---- stripping down ${locallib}"
				${STRIP} ${locallib}
			fi

			# point the parent lib to the bundled lib in relative pathing mode
			echo "---- telling lib/${dylib} how to find ${locallib}"
			install_name_tool -change ${MACPORTS_BASE}/${requiredlib} @loader_path/../${locallib} lib/${dylib}
		done
	done
done

# here, the bundle should be ready, a little compression and voila

ARCHIVE_NAME=spring_${VERSION}_MacOSX-10.6-SnowLeopard.zip
echo "-- creating ${TMP_PATH}/${ARCHIVE_NAME}"
cd ${TMP_PATH}
zip -r9 ${ARCHIVE_NAME} ${BUNDLE_NAME}

#remove temp files
rm -rf ${TMP_PATH}/${BUNDLE_NAME}

#create symbolic link
cd ${TMP_PATH}/..
ln -sfv ${REV}/${ARCHIVE_NAME} Spring_testing-MacOSX-10.6-SnowLeopard.zip
echo "-- done"

