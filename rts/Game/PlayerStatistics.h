/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLAYER_STATISTICS_H
#define PLAYER_STATISTICS_H

#include "System/creg/creg_cond.h"

#define	PLAYER_STATISTICS_DATA\
	int mousePixels;\
	int mouseClicks;\
	int keyPresses;\
	int numCommands;\
	int unitCommands;

#define PLAYER_STATISTICS_DATA_START_ADDRESS(playerStats) (playerStats->get_raw_data())

// keep a raw data struct to prevent platform dependent size/layout mismatch caused by creg
struct PlayerStatisticsData {
	PLAYER_STATISTICS_DATA
};

/**
 * @brief Contains statistical data about a player concerning a single game.
 * In the future, this should be inheriting TeamControllerStatistics.
 */
struct PlayerStatistics
{
public:
	CR_DECLARE_STRUCT(PlayerStatistics);

	PlayerStatistics();

	/// how many pixels the mouse has traversed in total
	PLAYER_STATISTICS_DATA

	const PlayerStatisticsData* get_raw_data() const { return reinterpret_cast<const PlayerStatisticsData*>(&mousePixels); }
	      PlayerStatisticsData* get_raw_data()       { return reinterpret_cast<      PlayerStatisticsData*>(&mousePixels); }
	void set_raw_data(const PlayerStatisticsData* data) { *reinterpret_cast<PlayerStatisticsData*>(&mousePixels) = *data; }

	/// Change structure from host endian to little endian or vice versa.
	void swab();
};


#endif // PLAYER_STATISTICS_H
