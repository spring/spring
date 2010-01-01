#include "PlayerStatistics.h"

#include "Platform/byteorder.h"

void PlayerStatistics::swab()
{
	mousePixels = swabdword(mousePixels);
	mouseClicks = swabdword(mouseClicks);
	keyPresses = swabdword(keyPresses);
	
	numCommands = swabdword(numCommands);
	unitCommands = swabdword(unitCommands);
}
