#!/bin/bash
set -e
. buildbot/slave/prepare.sh


DEST=${TMP_BASE}/inst
#FIXME: remove hardcoded /usr/local
INSTALLDIR=${DEST}/usr/local

echo "Installing into $DEST"

cd ${BUILDDIR}
${MAKE} install DESTDIR=${DEST}

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
	# for each MacPorts's dylib required by the executable
	for dylib in `otool -L MacOS/${executable} | grep ${MACPORTS_BASE} | egrep -o lib.*dylib`
	do
		# dylib =~ "lib{/optional-sub-dirs}/lib{name-version}.dylib"

		#echo "---- ${executable} requires ${MACPORTS_BASE}/${dylib}"

		# strip down the optional subdirectories (if any)
		locallib=lib/`echo ${dylib} | egrep -o [^/]*$`

		if [ ! -f ${locallib} ]
		then
			echo "---- copying ${MACPORTS_BASE}/${dylib} to ${locallib}"
			cp ${MACPORTS_BASE}/${dylib} ${locallib}
	
			#echo "---- taking write permissions on ${locallib}"
			chmod u+w ${locallib}
	
			# change the bundled lib's id to relative pathing mode (not necessary but cleaner)
			install_name_tool -id @loader_path/../${locallib} ${locallib}
		fi
		
		#echo "---- telling ${executable} to use ${locallib} instead of ${MACPORTS_BASE}/${dylib}"
		# point the executable to the bundled lib in relative pathing mode
		install_name_tool -change ${MACPORTS_BASE}/${dylib} @loader_path/../${locallib} MacOS/${executable}

		# the bundled lib is autonomous => it can be stripped
		#echo "---- stripping down ${locallib}"
		${STRIP} ${locallib}
	done

	#echo "--- stripping down ${executable}"
	${STRIP} MacOS/${executable}
done

# continue with recursive dependencies
echo "-- copying other required 3rd-party libraries into folder: lib (provided by MacPorts)"
end=0
while [ $end -eq 0 ]
do
	# assume we're already done
	end=1

	# for each dylib already bundled
	for dylib in `ls lib`
	do
		# check if all dependent MacPorts libs are also bundled

		for requiredlib in `otool -L lib/${dylib} | grep ${MACPORTS_BASE} | egrep -o lib.*dylib`
		do
			#echo "---- lib/${dylib} requires ${MACPORTS_BASE}/${requiredlib}"
			
			# strip down lib's subdirectories (if any)
			locallib=lib/`echo ${requiredlib} | egrep -o [^/]*$`

			if [ ! -f ${locallib} ]
			then
				# a new library will be copied, we'll have to loop to scan it
				end=0

				echo "---- copying ${MACPORTS_BASE}/${requiredlib} to ${locallib}"
				cp ${MACPORTS_BASE}/${requiredlib} ${locallib}

				#echo "---- taking write permissions on ${locallib}"
				chmod u+w ${locallib}

				# change the bundled lib's id to relative pathing mode (not necessary but cleaner)
				install_name_tool -id @loader_path/../${locallib} ${locallib}

				# the bundled lib is autonomous => it can be stripped
				#echo "---- stripping down ${locallib}"
				${STRIP} ${locallib}
			fi

			# point the parent lib to the bundled lib in relative pathing mode
			#echo "---- telling lib/${dylib} to use ${locallib} instead of ${MACPORTS_BASE}/${requiredlib}"
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

