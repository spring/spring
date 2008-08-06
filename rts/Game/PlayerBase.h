#ifndef PLAYERBASE_H
#define PLAYERBASE_H

#include <string>

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

#endif