#!/bin/sh

# tarball generation script

. buildbot/slave/prepare.sh

# Quit on error.
set -e

OUTPUTDIR=${TMP_PATH}

if [ -z "${TMP_PATH}" ]  || [ "${TMP_PATH}" == "/" ]; then
	echo "Invalid path: ${TMP_PATH}"
	exit 1;
fi

echo Using OUTPUTDIR=${OUTPUTDIR}

# create output dir
mkdir -p "$OUTPUTDIR/source"

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

SOURCEROOT=$(pwd)

describe=$(git describe --tags --candidates 999 --match "*.*")

set +e # turn of quit on error
# Check if current HEAD has a version tag
git describe --tags --candidates 0 --match "*.*" &> /dev/null
onVersionTag=$(if [ $? -eq "0" ]; then echo "true"; else echo "false"; fi)
set -e # turn it on again

isRelease=${onVersionTag}

if [ "${BRANCH}" = "master" -a ! ${onVersionTag} ]; then
	echo "Error: On branch master but not on a version tag." >&2
	echo "This indicates a tagging, branching or push error." >&2
	exit 3
fi

if ${isRelease}; then
	echo "Making release-packages"
	versionString=${describe}
	versionInfo=${describe}
else
	echo "Making test-packages"
	# Insert the branch name as the patch-set part.
	# (double-quotation is required because of the sub-shell)
	versionString="${describe}_${BRANCH}"
	versionInfo="${describe} ${BRANCH}"
fi

echo "Using version \"${versionInfo}\" as source"
dir="spring_${versionString}"

# Each one of these that is set, is built when running this script.
# Linux archives
# * linux (LF) line endings
# * GPL compatible
lzma="spring_${versionString}_src.tar.lzma"
tgz="spring_${versionString}_src.tar.gz"

echo 'Exporting checkout dir with LF line endings'
git clone -s --recursive ${SOURCEROOT} ${OUTPUTDIR}/${dir}

cd ${OUTPUTDIR}/${dir}

# Checkout the release-version
git checkout ${BRANCH}

# Add the engine version info, as we can not fetch it through git
# when using a source archive
echo "${versionInfo}" > ./VERSION
rm -rf	${dir}/.git \
	${dir}/.gitignore \
	${dir}/.gitmodules \
	${dir}/.mailmap \
	${dir}/tools/pr-downloader/src/lsl \
	${dir}/tools/pr-downloader/src/lib/cimg

cd ..
# XXX use git-archive instead? (submodules may cause a bit trouble with it)
# https://github.com/meitar/git-archive-all.sh/wiki

echo "Creating .tar.lzma archive (${lzma})"
tar -c --lzma -v -f "${OUTPUTDIR}/source/${lzma}" ${dir} --exclude=.git

echo "Creating .tar.gz archive (${tgz})"
tar -c --gzip -f  "${OUTPUTDIR}/source/${tgz}" ${dir} --exclude=.git

echo "Cleaning up ${OUTPUTDIR}/${dir}"
rm -rf "${OUTPUTDIR}/${dir}"

