#include "StdAfx.h"
#include "PlayerBase.h"

#include "Platform/byteorder.h"

PlayerBase::PlayerBase() :
team(0),
rank(-1),
name("no name"),
spectator(false),
isFromDemo(false)
{
}

void PlayerStatistics::swab()
{
	mousePixels = swabdword(mousePixels);
	mouseClicks = swabdword(mouseClicks);
	keyPresses = swabdword(keyPresses);
	numCommands = swabdword(numCommands);
	unitCommands = swabdword(unitCommands);
}