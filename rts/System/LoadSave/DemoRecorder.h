/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEMO_RECORDER
#define DEMO_RECORDER

#include <vector>
#include <fstream>
#include <list>

#include "Demo.h"
#include "Game/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"

/**
 * @brief Used to record demos
 */
class CDemoRecorder : public CDemo
{
public:
	CDemoRecorder();
	~CDemoRecorder();

	void WriteSetupText(const std::string& text);
	void SaveToDemo(const unsigned char* buf,const unsigned length, const float modGameTime);
	
	/**
	@brief assign a map name for the demo file
	When this function is called, we can rename our demo file so that
	map name / game time are visible. The demo file will be renamed by the
	destructor. Otherwise the name "DATE_TIME_unnamed_VERSION.sdf" will be used.
	*/
	void SetName(const std::string& mapname, const std::string& modname);
	const std::string& GetName() { return wantedName; }

	void SetGameID(const unsigned char* buf);
	void SetTime(int gameTime, int wallclockTime);

	void InitializeStats(int numPlayers, int numTeams, int winningAllyTeam);
	void SetPlayerStats(int playerNum, const PlayerStatistics& stats);
	void SetTeamStats(int teamNum, const std::list< TeamStatistics >& stats);

private:
	void WriteFileHeader(bool updateStreamLength = true);
	void WritePlayerStats();
	void WriteTeamStats();

	std::ofstream recordDemo;
	std::string wantedName;
	std::vector<PlayerStatistics> playerStats;
	std::vector< std::vector<TeamStatistics> > teamStats;
};


#endif

