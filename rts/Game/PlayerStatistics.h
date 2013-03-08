/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_STATISTICS_H
#define PLAYER_STATISTICS_H

#include "TeamController.h"
#include "System/creg/creg_cond.h"


/**
 * @brief Contains statistical data about a player concerning a single game.
 * In the future, this should be inheriting TeamControllerStatistics.
 */
struct PlayerStatistics : public TeamControllerStatistics
{
public:
	CR_DECLARE(PlayerStatistics);

	PlayerStatistics();

	/// how many pixels the mouse has traversed in total
	int mousePixels;
	int mouseClicks;
	int keyPresses;

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};


#endif // PLAYER_STATISTICS_H
