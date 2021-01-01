/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_BASE_H
#define PLAYER_BASE_H

#include <string>

#include "Game/Players/TeamController.h"
#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"

/**
 * @brief Acts as a base class for the various player-representing classes
 */
class PlayerBase : public TeamController
{
	CR_DECLARE(PlayerBase)

public:
	typedef spring::unordered_map<std::string, std::string> customOpts;

	PlayerBase();
	virtual ~PlayerBase() {}

	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const { return customValues; }

	const char* GetType(const bool capital = true) const {
		if (capital)
			return spectator ? "Spectator" : "Player";

		return spectator ? "spectator" : "player";
	}

	void SetReadyToStart(bool b) { readyToStart = b; }

	int GetRank() const { return rank; }
	float GetCPUUsage() const { return cpuUsage; }

	bool IsSpectator() const { return spectator; }
	bool IsFromDemo() const { return isFromDemo; }
	bool IsReadyToStart() const { return readyToStart; }
	bool IsDesynced() const { return desynced; }

	const std::string& GetCountryCode() const { return countryCode; }

// protected:
	int rank = -1;
	float cpuUsage = 0.0f;

	bool spectator = false;
	bool isFromDemo = false;
	bool readyToStart = false;
	bool desynced = false;

	std::string countryCode;

private:
	customOpts customValues;
};

#endif // PLAYER_BASE_H
