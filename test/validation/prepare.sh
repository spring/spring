#!/bin/sh

set -e #abort on error

if [ $# -lt 4 ]; then
	echo "Usage: $0 Game Map AI AIversion"
	exit 1
fi
GAME="$1"
MAP="$2"
AI="$3"
AIVERSION="$4"

cat <<EOD
// a validation script
// runs $GAME with $AI $AIVERSION vs $AI $AIVERSION on $MAP
[GAME]
{
	IsHost=1;
	MyPlayerName=TestMonkey;

	Mapname=$MAP;
	GameType=$GAME;

	StartPosType=0;
	[mapoptions]
	{
	}
	[modoptions]
	{
		disablemapdamage=0;
		fixedallies=0;
		ghostedbuildings=1;
		limitdgun=0;
		mo_allowfactionchange=1;
		mo_combomb_full_damage=1;
		mo_comgate=0;
		mo_coop=1;
		mo_enemywrecks=1;
		mo_greenfields=0;
		mo_noowner=0;
		mo_noshare=1;
		mo_nowrecks=0;
		mo_preventdraw=0;
		mo_progmines=0;
		maxspeed=300;
		maxunits=500;
		minspeed=0.3;
		mo_armageddontime=0;
		startenergy=1000;
		startmetal=1000;
		deathmode=com;
		mo_transportenemy=com;
		pathfinder=default;
	}
	NumRestrictions=0;
	[RESTRICT]
	{
	}
	[PLAYER1]
	{
		Name=ValidationClient;
		Spectator=1;
	}
	[PLAYER2]
	{
		Name=TestMonkey;
		CountryCode=;
		Spectator=1;
		Rank=0;
		IsFromDemo=0;
		Team=0;
	}
	[AI0]
	{
		Name=Bot1;
		ShortName=$AI;
		Version=$AIVERSION;
		Team=0;
		IsFromDemo=0;
		Host=2;
		[Options]
		{
		}
	}
	[AI1]
	{
		Name=Bot2;
		ShortName=$AI;
		Version=$AIVERSION;
		Team=1;
		IsFromDemo=0;
		Host=2;
		[Options]
		{
		}
	}

	[TEAM0]
	{
		TeamLeader=2;
		AllyTeam=0;
		RGBColor=0.976471 1 0;
		Side=ARM;
		Handicap=0;
	}
	[TEAM1]
	{
		TeamLeader=2;
		AllyTeam=1;
		RGBColor=0.509804 0.498039 1;
		Side=ARM;
		Handicap=0;
	}

	[ALLYTEAM0]
	{
		NumAllies=0;
	}
	[ALLYTEAM1]
	{
		NumAllies=0;
	}
}
EOD
