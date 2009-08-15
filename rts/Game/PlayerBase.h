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
	
	void SetValue(const std::string& key, const std::string& value);
	const customOpts& GetAllValues() const {
		return customValues;
	}

private:
	customOpts customValues;
};

/**
 * @brief Contains statistical data about a player concerning a single game.
 */
class PlayerStatistics : public TeamControllerStatistics
{
public:
	/// how many pixels the mouse has traversed in total
	int mousePixels;
	int mouseClicks;
	int keyPresses;

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};

#endif // __PLAYER_BASE_H
