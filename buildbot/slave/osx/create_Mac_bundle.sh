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
cd ${BUNDLE_BASE}

# insert the version number in the Info.plist file
echo "-- installing header file"
cat ${SOURCEDIR}/installer/Mac/Info.plist | sed s/###VERSION###/${VERSION}/ > Info.plist

mkdir MacOS Resources

echo "-- installing executables"
mv ${INSTALLDIR}/bin/* MacOS/

echo "-- installing application icon"
cp ${SOURCEDIR}/installer/Mac/spring.icns Resources/

echo "-- installing spring libs"
mv ${INSTALLDIR}/lib .

echo "-- installing game content"
mv ${INSTALLDIR}/share .

# move spring libraries to the executable folder (avoids spring relative path issues)
mv lib/libunitsync.dylib MacOS/

# not needed (yet) for the server lib, uncomment if necessary
#mv lib/libspringserver.dylib MacOS/

echo "-- installing 3rd-party libraries (provided by MacPorts)"
for executable in `ls MacOS`
do
	# for each MacPorts's dylib required by the executable
	for dylib in `otool -L MacOS/${executable} | grep ${MACPORTS_BASE} | egrep -o lib.*dylib`
	do
		# dylib =~ "lib/lib*.dylib"

		echo "---- installing ${dylib}"
		cp ${MACPORTS_BASE}/${dylib} lib

		# take write permissions on the bundled lib
		chmod u+w ${dylib}

		# change the bundled lib's id to relative pathing mode (not necessary but cleaner)
		install_name_tool -id @loader_path/../${dylib} ${dylib}

		# point the executable to the bundled lib in relative pathing mode
		install_name_tool -change ${MACPORTS_BASE}/${dylib} @loader_path/../${dylib} MacOS/${executable}

		# the bundled lib is autonomous => it can be stripped
		${STRIP} ${dylib}
	done

	# finally, strip the executable
	${STRIP} MacOS/${executable}
done

# continue with recursive dependencies
end=0
while [ $end -eq 0 ]
do
	# don't loop unless necessary
	end=1

	# for each dylib already bundled
	for dylib in `ls lib`
	do
		# check if all dependent MacPorts libs are also bundled
		for requiredlib in `otool -L lib/${dylib} | grep ${MACPORTS_BASE} | egrep -o lib.*dylib`
		do
			#echo "MacPorts lib required: ${requiredlib}"
			if [ ! -f ${requiredlib} ]
			then
				# loop
				end=0

				echo "---- installing ${requiredlib}"
				cp ${MACPORTS_BASE}/${requiredlib} lib

				# take write permissions on the bundled lib
				chmod u+w ${requiredlib}

				# change the bundled lib's id to relative pathing mode (not necessary but cleaner)
				install_name_tool -id @loader_path/../${requiredlib} ${requiredlib}

				# the bundled lib is autonomous => it can be stripped
				${STRIP} ${requiredlib}
			fi

			# point the parent lib to the bundled lib in relative pathing mode
			install_name_tool -change ${MACPORTS_BASE}/${requiredlib} @loader_path/../${requiredlib} lib/${dylib}
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

