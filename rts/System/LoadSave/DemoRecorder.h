/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEMO_RECORDER
#define DEMO_RECORDER

#include <vector>
#include <sstream>
#include <zlib.h>

#include "Demo.h"
#include "Game/Players/PlayerStatistics.h"
#include "Sim/Misc/TeamStatistics.h"


/**
 * @brief Used to record demos
 */
class CDemoRecorder : public CDemo
{
public:
	CDemoRecorder(const std::string& mapName, const std::string& modName, bool serverDemo);
	~CDemoRecorder();

	void WriteSetupText(const std::string& text);
	void SaveToDemo(const unsigned char* buf, const unsigned length, const float modGameTime);

	void SetStream();
	void SetName(const std::string& mapName, const std::string& modName);
	const std::string& GetName() const { return demoName; }

	void SetGameID(const unsigned char* buf);
	void SetTime(int gameTime, int wallclockTime);

	void AddNewPlayer(const std::string& name, int playerNum);
	void InitializeStats(int numPlayers, int numTeams);
	void SetPlayerStats(int playerNum, const PlayerStatistics& stats);
	void SetTeamStats(int teamNum, const std::vector<TeamStatistics>& stats);
	void SetWinningAllyTeams(const std::vector<unsigned char>& winningAllyTeams);

private:
	unsigned int WriteFileHeader(bool updateStreamLength);
	void SetFileHeader();
	void WritePlayerStats();
	void WriteTeamStats();
	void WriteWinnerList();
	void WriteDemoFile();

private:
	gzFile file;

	std::vector<PlayerStatistics> playerStats;
	std::vector< std::vector<TeamStatistics> > teamStats;
	std::vector<unsigned char> winningAllyTeams;

	bool isServerDemo;
};


#endif

