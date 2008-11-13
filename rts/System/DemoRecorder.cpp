#include "StdAfx.h"
#include "DemoRecorder.h"

#include <assert.h>
#include <time.h>
#include <errno.h>

#include "mmgr.h"

#include "Sync/Syncify.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "Exceptions.h"
#include "Util.h"
#include "GlobalUnsynced.h"

#include "LogOutput.h"

#ifdef __GNUC__
#define __time64_t time_t
#define _time64(x) time(x)
#define _localtime64(x) localtime(x)
#endif

CDemoRecorder::CDemoRecorder()
{
	// We want this folder to exist
	if (!filesystem.CreateDirectory("demos"))
		return;

	SetName("unnamed");
	demoName = wantedName;

	std::string filename = filesystem.LocateFile(demoName, FileSystem::WRITE);
	recordDemo.open(filename.c_str(), std::ios::out | std::ios::binary);

	memset(&fileHeader, 0, sizeof(DemoFileHeader));
	strcpy(fileHeader.magic, DEMOFILE_MAGIC);
	fileHeader.version = DEMOFILE_VERSION;
	fileHeader.headerSize = sizeof(DemoFileHeader);
	strcpy(fileHeader.versionString, SpringVersion::Get().c_str());

	__time64_t currtime;
	_time64(&currtime);
	fileHeader.unixTime = currtime;

	recordDemo.write((char*)&fileHeader, sizeof(fileHeader));

	fileHeader.playerStatElemSize = sizeof(CPlayer::Statistics);
	fileHeader.teamStatElemSize = sizeof(CTeam::Statistics);
	fileHeader.teamStatPeriod = CTeam::statsPeriod;
	fileHeader.winningAllyTeam = -1;

	WriteFileHeader(false);
}

CDemoRecorder::~CDemoRecorder()
{
	WritePlayerStats();
	WriteTeamStats();
	WriteFileHeader();

	recordDemo.close();

	if (demoName != wantedName) {
		if (rename(demoName.c_str(), wantedName.c_str()) != 0) {
            logOutput << "Renaming demo " << demoName << " to " << wantedName << "\n";
            logOutput << "failed: " << strerror(errno) << "\n";
		}
	} else {
	    // pointless?
		//remove("demos/unnamed.sdf");
		//rename(demoName.c_str(), "demos/unnamed.sdf");
	}
}

void CDemoRecorder::WriteSetupText(const std::string& text)
{
	int length = gameSetup->gameSetupText.length();
	while (gameSetup->gameSetupText.c_str()[length - 1] == '\0')
		--length;

	fileHeader.scriptSize = length;
	recordDemo.write(gameSetup->gameSetupText.c_str(), length);
}

void CDemoRecorder::SaveToDemo(const unsigned char* buf, const unsigned length)
{
	DemoStreamChunkHeader chunkHeader;

	chunkHeader.modGameTime = gu->modGameTime;
	chunkHeader.length = length;
	chunkHeader.swab();
	recordDemo.write((char*)&chunkHeader, sizeof(chunkHeader));
	recordDemo.write((char*)buf, length);
	fileHeader.demoStreamSize += length + sizeof(chunkHeader);
	recordDemo.flush();
}

void CDemoRecorder::SetName(const std::string& mapname)
{
	struct tm *newtime;
	__time64_t long_time;
	_time64(&long_time);                /* Get time as long integer. */
	newtime = _localtime64(&long_time); /* Convert to local time. */

	// Don't see how this can happen (according to docs _localtime64 only returns
	// NULL if long_time is before 1/1/1970...) but a user's stacktrace indicated
	// NULL newtime in the snprintf line...
	if (!newtime)
		throw content_error("error: _localtime64 returned NULL");

	char buf[1000];
	SNPRINTF(buf, sizeof(buf), "%04i%02i%02i_%02i%02i%02i", newtime->tm_year+1900, newtime->tm_mon + 1, newtime->tm_mday,
        newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	std::string name = std::string(buf) + "_" + mapname.substr(0, mapname.find_first_of("."));
	name += std::string("_") + SpringVersion::Get();
	// games without gameSetup should have different names
	if (!gameSetup) {
	    name = "local_" + name;
	}

	SNPRINTF(buf, sizeof(buf), "demos/%s.sdf", name.c_str());
	CFileHandler ifs(buf);
	if (ifs.FileExists()) {
		for (int a = 0; a < 9999; ++a) {
			SNPRINTF(buf, sizeof(buf), "demos/%s_(%i).sdf", name.c_str(), a);
			CFileHandler ifs(buf);
			if (!ifs.FileExists())
				break;
		}
	}
	wantedName = buf;
}

void CDemoRecorder::SetGameID(const unsigned char* buf)
{
	memcpy(&fileHeader.gameID, buf, sizeof(fileHeader.gameID));
	WriteFileHeader(false);
}

void CDemoRecorder::SetTime(int gameTime, int wallclockTime)
{
	fileHeader.gameTime = gameTime;
	fileHeader.wallclockTime = wallclockTime;
}

void CDemoRecorder::SetMaxPlayerNum(int maxPlayerNum)
{
	if (fileHeader.maxPlayerNum < maxPlayerNum)
		fileHeader.maxPlayerNum = maxPlayerNum;
}

void CDemoRecorder::InitializeStats(int numPlayers, int numTeams, int winningAllyTeam)
{
	fileHeader.numPlayers = numPlayers;
	fileHeader.numTeams = numTeams;
	fileHeader.winningAllyTeam = winningAllyTeam;

	playerStats.resize(numPlayers);
	teamStats.resize(numTeams);
}

/** @brief Set (overwrite) the CPlayer::Statistics for player playerNum */
void CDemoRecorder::SetPlayerStats(int playerNum, const CPlayer::Statistics& stats)
{
	assert((unsigned)playerNum < playerStats.size());

	playerStats[playerNum] = stats;
}

/** @brief Set (overwrite) the CTeam::Statistics history for team teamNum */
void CDemoRecorder::SetTeamStats(int teamNum, const std::list< CTeam::Statistics >& stats)
{
	assert((unsigned)teamNum < teamStats.size());

	teamStats[teamNum].clear();
	teamStats[teamNum].reserve(stats.size());

	for (std::list< CTeam::Statistics >::const_iterator it = stats.begin(); it != stats.end(); ++it)
		teamStats[teamNum].push_back(*it);
}

/** @brief Write DemoFileHeader
Write the DemoFileHeader at the start of the file and restores the original
position in the file afterwards. */
void CDemoRecorder::WriteFileHeader(bool updateStreamLength)
{
	int pos = recordDemo.tellp();

	recordDemo.seekp(0);

	DemoFileHeader tmpHeader;
	memcpy(&tmpHeader, &fileHeader, sizeof(fileHeader));
	if (!updateStreamLength)
		tmpHeader.demoStreamSize = 0;
	tmpHeader.swab(); // to little endian
	recordDemo.write((char*)&tmpHeader, sizeof(tmpHeader));
	recordDemo.seekp(pos);
}

/** @brief Write the CPlayer::Statistics at the current position in the file. */
void CDemoRecorder::WritePlayerStats()
{
	if (fileHeader.numPlayers == 0)
		return;

	int pos = recordDemo.tellp();

	for (std::vector< CPlayer::Statistics >::iterator it = playerStats.begin(); it != playerStats.end(); ++it) {
		CPlayer::Statistics& stats = *it;
		stats.swab();
		recordDemo.write((char*)&stats, sizeof(CPlayer::Statistics));
	}
	playerStats.clear();

	fileHeader.playerStatSize = (int)recordDemo.tellp() - pos;
}

/** @brief Write the CTeam::Statistics at the current position in the file. */
void CDemoRecorder::WriteTeamStats()
{
	if (fileHeader.numTeams == 0)
		return;

	int pos = recordDemo.tellp();

	// Write array of dwords indicating number of CTeam::Statistics per team.
	for (std::vector< std::vector< CTeam::Statistics > >::iterator it = teamStats.begin(); it != teamStats.end(); ++it) {
		unsigned int c = swabdword(it->size());
		recordDemo.write((char*)&c, sizeof(unsigned int));
	}

	// Write big array of CTeam::Statistics.
	for (std::vector< std::vector< CTeam::Statistics > >::iterator it = teamStats.begin(); it != teamStats.end(); ++it) {
		for (std::vector< CTeam::Statistics >::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
			CTeam::Statistics& stats = *it2;
			stats.swab();
			recordDemo.write((char*)&stats, sizeof(CTeam::Statistics));
		}
	}
	teamStats.clear();

	fileHeader.teamStatSize = (int)recordDemo.tellp() - pos;
}
