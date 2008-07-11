#include "DemoRecorder.h"

#include <assert.h>
#include <time.h>

#include "Sync/Syncify.h"
#include "Platform/FileSystem.h"
#include "FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Game/GameSetup.h"

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

	wantedName = demoName = "demos/unnamed.sdf";

	std::string filename = filesystem.LocateFile(demoName, FileSystem::WRITE);
	recordDemo.open(filename.c_str(), std::ios::out | std::ios::binary);

	memset(&fileHeader, 0, sizeof(DemoFileHeader));
	strcpy(fileHeader.magic, DEMOFILE_MAGIC);
	fileHeader.version = DEMOFILE_VERSION;
	fileHeader.headerSize = sizeof(DemoFileHeader);
	strcpy(fileHeader.versionString, VERSION_STRING);

	__time64_t currtime;
	_time64(&currtime);
	fileHeader.unixTime = currtime;

	recordDemo.write((char*)&fileHeader, sizeof(fileHeader));

	if (gameSetup) {
		// strip trailing null termination characters
		int length = gameSetup->gameSetupTextLength;
		while (gameSetup->gameSetupText[length - 1] == '\0')
			--length;

		fileHeader.scriptSize = length;
		recordDemo.write(gameSetup->gameSetupText, length);
	}

	fileHeader.playerStatElemSize = sizeof(CPlayer::Statistics);
	fileHeader.teamStatElemSize = sizeof(CTeam::Statistics);
	fileHeader.teamStatPeriod = CTeam::statsPeriod;
	fileHeader.winningAllyTeam = -1;

	WriteFileHeader();
}

CDemoRecorder::~CDemoRecorder()
{
	WritePlayerStats();
	WriteTeamStats();
	WriteFileHeader();

	if (demoName != wantedName) {
		rename(demoName.c_str(), wantedName.c_str());
	} else {
		remove("demos/unnamed.sdf");
		rename(demoName.c_str(), "demos/unnamed.sdf");
	}
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

	char buf[500];
	sprintf(buf, "%02i%02i%02i", newtime->tm_year % 100, newtime->tm_mon + 1, newtime->tm_mday);
	std::string name = std::string(buf) + "-" + mapname.substr(0, mapname.find_first_of("."));
	name += std::string("-") + VERSION_STRING;

	sprintf(buf,"demos/%s.sdf", name.c_str());
	CFileHandler ifs(buf);
	if (ifs.FileExists()) {
		for (int a = 0; a < 9999; ++a) {
			sprintf(buf,"demos/%s-%i.sdf", name.c_str(), a);
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
	WriteFileHeader();
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
void CDemoRecorder::WriteFileHeader()
{
	int pos = recordDemo.tellp();

	recordDemo.seekp(0);

	fileHeader.swab(); // to little endian
	recordDemo.write((char*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab(); // back to host endian

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
