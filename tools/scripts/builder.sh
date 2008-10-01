#!/bin/sh
# Author: Tobi Vollebregt
#
# Usage: builder.sh

# Compilers to test.
GCCS="gcc-3.4 gcc-4.0 gcc-4.1 i586-mingw32msvc-gcc"

# optimize= options to test.
OPTS="0 2"

# AI used for testing.. (used as compile target and to generate DLL/SO name)
AI="AAI"
AIDIR="AI/Skirmish/impls"  # relative to '$WINEDIR' or 'game'.

# For crosscompiling MinGW builds.
MINGDIR="/usr/i586-mingw32msvc"

# For testing MinGW builds.
WINEDIR="$HOME/.wine/drive_c/TASpring"

# Some control of logging.
LOGDIR="log"
MASTERLOG="$LOGDIR/index.html"

# SVN revision (used in exe names etc.)
REVISION="$(svn info --non-interactive | grep Revision | awk '{print $2}')"

##### USUALLY THERE'S NO NEED TO EDIT BELOW THIS LINE #####

# Be paranoid: quit shell on a single error.
set -e

# timing
TIME="$(which time) -v"

# Usage: die [reason]
# Error function.
die()
{
	echo "$@"
	exit 1
}

# Usage: configure [flags]
# Runs scons configure with the appropriate environment variables set.
# Arguments are passed on to scons configure.
configure()
{
	LOG="configure-$CC-$(date "+%y%m%d%H%M%S").log"
	echo "configuring..."
	echo "<a href=\"$LOG\">configuring</a>...<br>" >>"$MASTERLOG"
	LOG="$LOGDIR/$LOG"
	CXX=$(echo "$CC" | sed 's/gcc/g++/g')
	case "$CC" in
	  i586-mingw32msvc-gcc*)
		CC=$CC CXX=$CXX MINGDIR=$MINGDIR $TIME scons configure --config=force "cachedir=_cache-${CC}_" "platform=windows" "$@" >"$LOG" 2>&1
		;;
	  *)
		CC=$CC CXX=$CXX $TIME scons configure --config=force "cachedir=_cache-${CC}_" "debug=yes" "$@" >"$LOG" 2>&1
		;;
	esac
}

# Usage: compile [targets]
# Runs scons. Arguments are passed on to scons.
compile()
{
	LOG="compile-$CC-$(date "+%y%m%d%H%M%S").log"
	echo "compiling..."
	echo "<a href=\"$LOG\">compiling</a>...<br>" >>"$MASTERLOG"
	LOG="$LOGDIR/$LOG"
	$TIME scons "$@" >"$LOG" 2>&1
}

# Usage: clean
# Cleans the build.
clean()
{
	LOG="clean-$CC-$(date "+%y%m%d%H%M%S").log"
	echo "cleaning..."
	echo "<a href=\"$LOG\">cleaning</a>...<br>" >>"$MASTERLOG"
	LOG="$LOGDIR/$LOG"
	[ ! -d build ] || $TIME scons -c "$@" >"$LOG" 2>&1
}

# main program
[ ! -d "$LOGDIR" ] && (mkdir -p "$LOGDIR" || die "Cannot mkdir $LOGDIR")
unset SPRING_SERVER

# Compile
for CC in $GCCS; do
	for OPT in $OPTS; do
		EXEC="spring-r$REVISION-$CC-O$OPT"
		AIEXEC="$AI-r$REVISION-$CC-O$OPT"
		# only build etc. if one of the things we are gonna build doesn't exist yet
		if ( [ ! -x "game/$EXEC" ] && [ ! -x "$WINEDIR/$EXEC.exe" ] ) || \
			( [ ! -x "game/$AIDIR/$AIEXEC.so" ] && [ ! -x "$WINEDIR/$AIDIR/$AIEXEC.dll" ] )
		then
			echo
			echo "$EXEC: building..."
			echo "<br>" >>"$MASTERLOG"
			echo "$EXEC: building...<br>" >>"$MASTERLOG"
			clean
			if CC=$CC configure "optimize=$OPT" "$@" && CC=$CC compile spring "$AI"; then
				if [ -x "game/spring" ]; then
					mv "game/spring" "game/$EXEC"
					mv "game/$AIDIR/$AI.so" "game/$AIDIR/$AIEXEC.so"
					SPRING_SERVER="$SPRING_SERVER game/$EXEC"
				elif [ -x "game/spring.exe" ]; then
					mv "game/spring.exe" "$WINEDIR/$EXEC.exe"
					mv "game/$AIDIR/$AI.dll" "$WINEDIR/$AIDIR/$AIEXEC.dll"
					SPRING_SERVER="$SPRING_SERVER $WINEDIR/$EXEC.exe"
				fi
				echo "$EXEC : build succeeded"
				echo "$EXEC : build succeeded<br>" >>"$MASTERLOG"
			else
				echo "$EXEC : build failed"
				echo "$EXEC : <font color=\"red\">build failed</font><br>" >>"$MASTERLOG"
			fi
		else
			if [ -x "game/$EXEC" ]; then
				SPRING_SERVER="$SPRING_SERVER game/$EXEC"
			elif [ -x "$WINEDIR/$EXEC.exe" ]; then
				SPRING_SERVER="$SPRING_SERVER $WINEDIR/$EXEC.exe"
			fi
			echo "(cached) $EXEC : build succeeded"
		fi
	done
done

# Test
SPRING_SERVER="$SPRING_SERVER" SPRING_CLIENT="$SPRING_SERVER" \
	$(dirname "$0")/runner.sh "$AI.so"
