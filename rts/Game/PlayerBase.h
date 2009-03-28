#ifndef PLAYERBASE_H
#define PLAYERBASE_H

#include <string>
#include "Platform/byteorder.h"

/**
@brief Acts as a base class for the various player-representing classes
*/
class PlayerBase
{
public:
	/**
	@brief Constructor assigning standard values
	*/
	PlayerBase();
	int team;
	int rank;
	std::string name;
	std::string countryCode;
	bool spectator;
	bool isFromDemo;
};

struct PlayerStatistics
{
	/// how many pixels the mouse has traversed in total
	int mousePixels;
	int mouseClicks;
	int keyPresses;

	int numCommands;
	/// total amount of units affected by commands
	/// (divide by numCommands for average units/command)
	int unitCommands;

	/// Change structure from host endian to little endian or vice versa.
	void swab() {
		mousePixels = swabdword(mousePixels);
		mouseClicks = swabdword(mouseClicks);
		keyPresses = swabdword(keyPresses);
		numCommands = swabdword(numCommands);
		unitCommands = swabdword(unitCommands);
	}
};

#endif
