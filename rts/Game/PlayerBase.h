/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_BASE_H
#define PLAYER_BASE_H

#include "Game/TeamController.h"
#include "System/creg/creg_cond.h"

#include <string>
#include <map>

/**
 * @brief Acts as a base class for the various player-representing classes
 */
class PlayerBase : public TeamController
{
	CR_DECLARE(PlayerBase);

public:
	typedef std::map<std::string, std::string> customOpts;

	/**
	 * @brief Constructor assigning standard values
	 */
	PlayerBase();

	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const {
		return customValues;
	}

	const char* GetType(const bool capital = true) const {

		if (capital) {
			return spectator ? "Spectator" : "Player";
		}
		return spectator ? "spectator" : "player";
	}


	int rank;
	std::string countryCode;
	bool spectator;
	bool isFromDemo;
	bool readyToStart;
	bool desynced;
	float cpuUsage;

private:
	customOpts customValues;
};

#endif // PLAYER_BASE_H
