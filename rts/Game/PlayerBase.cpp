#include "StdAfx.h"
#include "PlayerBase.h"

#include <cstdlib>

#include "Platform/byteorder.h"

PlayerBase::PlayerBase() :
team(0),
rank(-1),
name("no name"),
spectator(false),
isFromDemo(false)
{
}

void PlayerBase::SetValue(const std::string& key, const std::string& value)
{
	if (key == "team")
		team = std::atoi(value.c_str());
	else if (key == "rank")
		rank = std::atoi(value.c_str());
	else if (key == "name")
		name = value;
	else if (key == "countryCode")
		countryCode = value;
	else if (key == "spectator")
		spectator = static_cast<bool>(std::atoi(value.c_str()));
	else if (key == "isfromdemo")
		isFromDemo = static_cast<bool>(std::atoi(value.c_str()));
	else
		customValues[key] = value;
};

void PlayerStatistics::swab()
{
	mousePixels = swabdword(mousePixels);
	mouseClicks = swabdword(mouseClicks);
	keyPresses = swabdword(keyPresses);
	numCommands = swabdword(numCommands);
	unitCommands = swabdword(unitCommands);
}