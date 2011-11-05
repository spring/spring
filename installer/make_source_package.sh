#!/bin/sh

# Quit on error.
set -e

ORIG_DIR=$(pwd)

# Sanity check.
if [ ! -x /usr/bin/git ]; then
	echo "Error: Could not find /usr/bin/git" >&2
	exit 1
fi

# Find correct working directory.
# (Compatible with SConstruct, which is in trunk root)

while [ ! -d installer ]; do
	if [ "${PWD}" = "/" ]; then
		echo "Error: Could not find installer directory." >&2
		echo "Make sure to run this script from a directory below your checkout directory." >&2
		exit 2
	fi
	cd ..
done


branch=$(git rev-parse --abbrev-ref HEAD)
describe=$(git describe --tags --candidates 999 --match "*.*")

set +e # turn of quit on error
# Check if current HEAD has a version tag
git describe --tags --candidates 0 --match "*.*" &> /dev/null
onVersionTag=$(if [ $? -eq "0" ]; then echo "true"; else echo "false"; fi)
set -e # turn it on again

isRelease=false
if [ "${branch}" = "master" ]; then
	if ${onVersionTag}; then
		isRelease=true
	else
		echo "Error: On branch master but not on a version tag." >&2
		echo "This indicates a tagging, branching or push error." >&2
		exit 3
	fi
fi

if ${isRelease}; then
	echo "Making release-packages"
	versionString=${describe}
else
	echo "Making test-packages"
	# Insert the branch name as the patch-set part.
	# (double-quotation is required because of the sub-shell)
	versionString="${describe}_${branch}"
fi

echo "Using branch \"${versionString}\" as source"


dir="spring_${versionString}"

# Each one of these that is set, is built when running this script.
# Linux archives
# * linux (LF) line endings
# * GPL compatible
lzma="spring_${versionString}_src.tar.lzma"
#tbz="spring_${versionString}_src.tar.bz2"
tgz="spring_${versionString}_src.tar.gz"

# Windows archives
# * windows (CRLF) line endings (bugged, see TODO below)
# * contain everything from the GIT repository
#zip="spring_${versionString}_src.zip"
#seven_zip="spring_${versionString}_src.7z"

# This is the list of files/directories that go in the source package.
# (directories are included recursively)
include=" \
 ${dir}/AI/ \
 ${dir}/doc/ \
 ${dir}/cont/ \
 ${dir}/include/ \
 ${dir}/installer/ \
 ${dir}/rts/ \
 ${dir}/tools/unitsync/ \
 ${dir}/tools/ArchiveMover/ \
 ${dir}/tools/DemoTool/ \
 ${dir}/tools/CMakeLists.txt \
 ${dir}/CMakeLists.txt \
 ${dir}/Doxyfile \
 ${dir}/directories.txt \
 ${dir}/README.markdown \
 ${dir}/LICENSE \
 ${dir}/LICENSE.html \
 ${dir}/THANKS \
 ${dir}/AUTHORS \
 ${dir}/FAQ \
 ${dir}/COPYING"

# On linux, win32 executables are useless.
exclude_from_all=""
linux_exclude="${exclude_from_all}"
linux_include=""
windows_exclude="${exclude_from_all}"
windows_include=""

# Linux line endings, .tar.{bz2,gz} package.
echo 'Exporting checkout dir with LF line endings'
git clone -n . lf/${dir}
cd lf/${dir}
git checkout ${branch}
git submodule update --init
cd ..
[ -n "${linux_exclude}" ] && rm -rf ${linux_exclude}
[ -n "${lzma}" ] && echo "Creating .tar.lzma archive (${lzma})" && \
	tar -c -f --lzma "../${lzma}" ${include} ${linux_include} --exclude=.git
[ -n "${tbz}" ] && echo "Creating .tar.bz2 archive (${tbz})" && \
	tar -c -f -j     "../${tbz}"  ${include} ${linux_include} --exclude=.git
[ -n "${tgz}" ] && echo "Creating .tar.gz archive (${tgz})" && \
	tar -c -f -z     "../${tgz}"  ${include} ${linux_include} --exclude=.git
cd ..
echo 'Cleaning'
rm -rf lf

if [ -n "${zip}" ] || [ -n "${seven_zip}" ]; then
	### TODO: needs fixing (not really using CRLF)
	# Windows line endings, .zip/.7z package
	#echo 'Exporting checkout dir with CRLF line endings'
	git clone -n . crlf/${dir}
	cd crlf/${dir}
	git config core.autocrlf true
	git checkout ${branch}
	git submodule update --init
	cd ..

	[ -n "${windows_exclude}" ] && rm -rf ${windows_exclude}
	[ -n "${zip}" ] && [ -x /usr/bin/zip ] && echo "Creating .zip archive (${zip})" && \
		/usr/bin/zip -q -r -u -9 "../${zip}" ${include} ${windows_include}
	[ -n "${seven_zip}" ] && [ -x /usr/bin/7z ] && echo "Creating .7z archive (${seven_zip})" && \
		/usr/bin/7z a -t7z -m0=lzma -mx=9 -mfb=64 -md=32m -ms=on "../${seven_zip}" ${include} >/dev/null
	cd ..
	echo 'Cleaning'
	rm -rf crlf
fi

cd ${ORIG_DIR}
