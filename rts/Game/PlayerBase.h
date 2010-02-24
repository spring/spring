/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __PLAYER_BASE_H
#define __PLAYER_BASE_H

#include "Game/TeamController.h"

#include <string>
#include <map>

/**
 * @brief Acts as a base class for the various player-representing classes
 */
class PlayerBase : public TeamController
{
public:
	typedef std::map<std::string, std::string> customOpts;

	/**
	 * @brief Constructor assigning standard values
	 */
	PlayerBase();

	int rank;
	std::string countryCode;
	bool spectator;
	bool isFromDemo;
	bool readyToStart;
	bool desynced;
	float cpuUsage;
	
	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const {
		return customValues;
	}

	const char *GetType(const bool capital = true) const {
		if(capital)
			return spectator ? "Spectator" : "Player";
		return spectator ? "spectator" : "player";
	}

private:
	customOpts customValues;
};

#endif // __PLAYER_BASE_H
