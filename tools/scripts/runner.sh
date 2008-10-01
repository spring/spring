#!/bin/sh
# Author: Tobi Vollebregt
#
# Usage: runner.sh [files]
#
# Can be passed a map (*.smf, *.sm3), mod (*.sdd, *.sdz, *.sd7) and AI (*.dll, *.so),
# in any order.  The file extension is used to guess what it is.
#
# Make sure to enable StdoutDebug=1 in ~/.springrc (or the wine registry if applicable).
#

# Extra flags passed to spring in GNU/Linux format.
# For wine they're autoconverted, e.g. '--quit=600' becomes '/quit 600'
SPRINGFLAGS="--minimise --quit=600"

# Binaries used for testing (don't use spaces in filenames!).
# These can be overridden from environment / other script.
[ -z "$SPRING_SERVER" ] && SPRING_SERVER="game/spring"
[ -z "$SPRING_CLIENT" ] && SPRING_CLIENT="game/spring"

# Where AIs are stored..
AIDIR="AI/Skirmish/impls"

# Some control of logging.
LOGDIR="log"
MASTERLOG="$LOGDIR/index.html"

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

# Usage: start_spring spring[.exe] [extra flags for spring]
# Starts spring. Through wine, gdb or natively.
# Returns the pid of the started process.
_start_spring()
{
	LOG="$1"
	EXEC="$2"
	shift 2
	[ ! -x $EXEC ] && die "cannot find $EXEC"
	case "$EXEC" in
	  *.exe)
		WINE=$(which wine)
		[ -x "$WINE" ] || die "Cannot run $1: wine not found"
		COMMAND="$WINE $EXEC $(echo "$@" | sed 's,--,/,g; s,=, ,g')"
		;;
	  *)
		GDB=$(which gdb)
		if [ -x "$GDB" ]; then
			SCRIPT="$LOGDIR/gdb_script.txt"
			/bin/echo -e "run\nthread apply all backtrace\nquit" >"$SCRIPT"
			COMMAND="$GDB -n -q -batch -x $SCRIPT --args $EXEC $@"
		else
			COMMAND="$EXEC $@"
		fi
		;;
	esac
	echo "starting $COMMAND..."
	echo "<a href=\"../$LOG\">starting</a> $COMMAND...<br>" >>"$MASTERLOG"
	$TIME $COMMAND >"$LOG" 2>&1 &
}

# Usage: _write_startscript [map] [mod] [AI] [script] [log]
# Can be passed a map (*.smf, *.sm3), mod (*.sdd, *.sdz, *.sd7), AI (*.dll, *.so),
# script (*.txt) and logfile (*.log) in any order.
# See Documentation/Spring start.txt for a more complete sample startscript.
_write_startscript()
{
	EXEC="$1"  #binary
	PNUM="$2"  #playernum
	shift 2
	MAP="Finns_Revenge.smf"
	MOD="AASS211.sdz"
	AI="AAI.so"
	# script and log should really be specified by the caller
	SCRIPT="_script_.txt"
	LOG="_write_startscript_.log"

	for ARG; do
		case "$ARG" in
		  *.sm[3f])
			MAP="$ARG" ;;
		  *.sd[7dz])
			MOD="$ARG" ;;
		  *.dll|*.so)
			AI="$ARG" ;;
		  *.txt)
			SCRIPT="$ARG" ;;
		  *.log)
			LOG="$ARG" ;;
		  *)
			die "Unrecognised argument passed to _write_startscript" ;;
		esac
	done

	# Transform AI:
	# The extension is fixed for the used platform (dll/so),
	# If a dll/so with identifier exists, it is used.
	# identifier is the XXX part in springXXX[.exe]
	case "$EXEC" in
	  *.exe)
		EXT=".dll" ;;
	  *)
		EXT=".so" ;;
	esac
	IDENT="$(basename "$EXEC" | sed 's/spring//g;s/.exe//g')"
	AI="$(echo "$AI" | sed 's/\.dll\|\.so//g')"  # strip extension
	if [ -x "$(dirname "$EXEC")/$AIDIR/$AI$IDENT$EXT" ]; then
		AI="$AI$IDENT$EXT"
	else
		AI="$AI$EXT"
	fi

	# Ready, now write the script.
	cat <<EOD | tee -a "$LOG" >"$SCRIPT"
[GAME]
{
	Mapname=$MAP;       //with .smf extension
	Gametype=$MOD;      //the primary mod archive
	StartMetal=1000;
	StartEnergy=1000;
	MaxUnits=500;       // per team
	StartPosType=1;     // 0 fixed, 1 random, 2 select in map
	GameMode=0;         // 0 cmdr dead->game continues, 1 cmdr dead->game ends
	LimitDgun=1;        // limit dgun to fixed radius around startpos?
	DiminishingMMs=0;   // diminish metal maker's metal production for every new one of them?
	DisableMapDamage=0; // disable map craters?
	GhostedBuildings=1; // ghost enemy buildings after losing los on them

	MaxSpeed=3;         // speed limits at game start
	MinSpeed=3;

	HostIP=localhost;
	HostPort=2113;      // usually 8452

	MyPlayerNum=$PNUM;  //only variable that should vary between different players

	NumPlayers=2;
	NumTeams=2;
	NumAllyTeams=2;

	[PLAYER0]
	{
		Name=Player0;
		Spectator=0;
		Team=0;
	}
	[PLAYER1]
	{
		Name=Player1;
		Spectator=0;
		Team=1;
	}
	[TEAM0]
	{
		TeamLeader=0;
		AllyTeam=0;
		RGBColor=0.35294 0.35294 1.00000;
		Side=ARM;
		Handicap=0;
		AIDLL=$AIDIR/$AI;
	}
	[TEAM1]
	{
		TeamLeader=0;
		AllyTeam=1;
		RGBColor=0.78431 0.00000 0.00000;
		Side=CORE;
		Handicap=0;
		AIDLL=$AIDIR/$AI;
	}
	[ALLYTEAM0]
	{
		NumAllies=0;
	}
	[ALLYTEAM1]
	{
		NumAllies=0;
	}
	NumRestrictions=0;
	[RESTRICT]
	{
	}
}
EOD
}

# Usage: start_spring binary playernum [files]
start_spring()
{
	EXEC="$1"  #binary
	PNUM="$2"  #playernum
	shift 2
	# this assumes the dir where the executable lives == the data dir
	SCRIPTFILE="_script${PNUM}_.txt"         # just filename
	SCRIPT="$(dirname "$EXEC")/$SCRIPTFILE"  # path/to/filename
	# log should really be specified by the caller
	LOG="_start_spring_${PNUM}_.log"

	for ARG; do
		case "$ARG" in
		  *.txt)
			SCRIPT="$ARG" ;;
		  *.log)
			LOG="$ARG" ;;
		esac
	done

	_write_startscript "$EXEC" "$PNUM" "$SCRIPT" "$@"
	_start_spring "$LOG" "$EXEC" "$SPRINGFLAGS" "$SCRIPTFILE"
}

# Usage: kill_spring exec_name_1 [exec_name_2 [exec_name_3 [etc]]]
kill_spring()
{
	for EXEC; do
		PID="$(ps --no-headers -f | grep "$(basename "$EXEC")" | grep -v 'grep' | awk '{print $2}')"
		if [ ! -z "$PID" ]; then
			echo "killing $EXEC ($PID)..."
			/bin/kill -9 $PID
			sleep 5
		fi
	done
}

# Usage: run_spring server_spring client_spring [files]
# Starts 2 instances of spring, waits till both are finished.
# Logfiles are put in log/datetime.log, where datetime is the current date&time.
run_spring()
{
	EXEC1="$1"
	EXEC2="$2"
	shift 2

	[ ! -d "$LOGDIR" ] && (mkdir -p "$LOGDIR" || die "Cannot mkdir $LOGDIR")
	LOG1="$LOGDIR/$(basename "$EXEC1")-server-$(date "+%y%m%d%H%M%S").log"
	LOG2="$LOGDIR/$(basename "$EXEC2")-client-$(date "+%y%m%d%H%M%S").log"

	echo
	echo "<br>" >>"$MASTERLOG"

	# run
	start_spring "$EXEC1" "0" "$LOG1" "$@"; sleep 5
	start_spring "$EXEC2" "1" "$LOG2" "$@"; sleep 5

	# sleep till they're finished
	while [ ! -z "$(ps --no-headers -f | grep "$(basename "$EXEC1")" | grep -v 'grep')" ] && \
	      [ ! -z "$(ps --no-headers -f | grep "$(basename "$EXEC2")" | grep -v 'grep')" ]; do
		sleep 1
	done

	# give some time so both have the chance to quit correctly.
	# kill them if they're still alive after this timeout.
	# (after regetting the PIDs to prevent chance for PID collision)
	sleep 5
	kill_spring "$EXEC1" "$EXEC2"

	# show summary
	if [ ! -z "$(grep "Sync error" "$LOG1" "$LOG2")" ]; then
		echo "$EXEC1 vs $EXEC2 : sync error"
		echo "$EXEC1 vs $EXEC2 : <font color=\"red\">sync error</font><br>" >>"$MASTERLOG"
	elif [ -z "$(grep "joined as 0" "$LOG1" "$LOG2")" ] || \
	     [ -z "$(grep "joined as 1" "$LOG1" "$LOG2")" ] || \
	     [ -z "$(grep "Automatical quit enforced from commandline" "$LOG1")" ]; then
		# ^^ these messages must occur if everything went reasonably fine (engine wise)
		echo "$EXEC1 vs $EXEC2 : error"
		echo "$EXEC1 vs $EXEC2 : <font color=\"red\">error</font><br>" >>"$MASTERLOG"
	elif [ ! -z "$(grep -i "error" "$LOG1" "$LOG2" | grep "GlobalAI")" ]; then
		# ^^ this catches GlobalAI errors.
		echo "$EXEC1 vs $EXEC2 : error"
		echo "$EXEC1 vs $EXEC2 : <font color=\"red\">error</font><br>" >>"$MASTERLOG"
	else
		# assuming everything went fine
		echo "$EXEC1 vs $EXEC2 : ok"
		echo "$EXEC1 vs $EXEC2 : ok<br>" >>"$MASTERLOG"
	fi

	# cleanup
	rm -f "$(dirname "$EXEC1")/_script0_.txt" "$(dirname "$EXEC2")/_script1_.txt"
}


# main program
for S in $SPRING_SERVER; do
	for C in $SPRING_CLIENT; do
		# only run if we didn't already test this combination (or the reverse: server/client swapped)
		if [ ! -f "$MASTERLOG" ] || ( [ -z "$(grep "$S vs $C" "$MASTERLOG")" ] && [ -z "$(grep "$C vs $S" "$MASTERLOG")" ] ); then
			run_spring "$S" "$C"  "$@"
		else
			echo -n "(cached) "
			grep "$S vs $C" "$MASTERLOG" | sed 's/<[^>]\+>//g'
			[ "$C" != "$S" ] && grep "$C vs $S" "$MASTERLOG" | sed 's/<[^>]\+>//g'
		fi
	done
done
