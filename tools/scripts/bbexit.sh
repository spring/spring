#!/bin/sh
# Author: Tobi Vollebregt
#
# Hook called as last step in buildbot crosscompile build.
# Gets passed one argument: a revision number.
#

if [ "$#" != "1" ]; then
	echo "Usage: $0 <revision>"
	exit 1
fi

exitstatus=0

# Zip & put exe online
echo "Creating spring-r$1.zip containing spring.exe"
cd game
if zip "spring-r$1.zip" "spring.exe" "spring.def"; then
	chmod 644 "spring-r$1.zip" || exit 1
	mv "spring-r$1.zip" "/home/buildbot/public_html/spring/executable/" || exit 1
else
	echo "failed to zip spring.exe into spring-r$1.zip"
	exitstatus=1
fi
cd ..
echo Done
echo

# Build installer & put online
#if installer/make_test_installer.sh "$1"; then
#	chmod 644 "installer/spring_r$1_nightly_gpl.exe" || exit 1
#	mv "installer/spring_r$1_nightly_gpl.exe" "/home/buildbot/public_html/spring/installer/" || exit 1
#else
#	echo "failed to build installer"
#	exitstatus=1
#fi

# Revert version so we don't get conflicts on next update when someone changes actual version.
svn revert rts/Game/GameVersion.h || exit 1

exit $exitstatus
