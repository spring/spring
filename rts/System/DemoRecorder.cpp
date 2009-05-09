#include "StdAfx.h"
#include "DemoRecorder.h"

#include <assert.h>
#include <errno.h>

#include "mmgr.h"

#include "FileSystem/FileSystem.h"
#include "FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "Util.h"
#include "TimeUtil.h"

#include "LogOutput.h"

CDemoRecorder::CDemoRecorder()
{
	// We want this folder to exist
	if (!filesystem.CreateDirectory("demos"))
		return;

	SetName("unnamed");
	demoName = GetName();

	std::string filename = filesystem.LocateFile(demoName, FileSystem::WRITE);
	recordDemo.open(filename.c_str(), std::ios::out | std::ios::binary);

	memset(&fileHeader, 0, sizeof(DemoFileHeader));
	strcpy(fileHeader.magic, DEMOFILE_MAGIC);
	fileHeader.version = DEMOFILE_VERSION;
	fileHeader.headerSize = sizeof(DemoFileHeader);
	strcpy(fileHeader.versionString, SpringVersion::Get().c_str());

	__time64_t currtime = CTimeUtil::GetCurrentTime();
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
#ifndef DEDICATED
			LogObject() << "Renaming demo " << demoName << " to " << wantedName << "\n";
			LogObject() << "failed: " << strerror(errno) << "\n";
#endif
		}
	} else {
		// pointless?
		//remove("demos/unnamed.sdf");
		//rename(demoName.c_str(), "demos/unnamed.sdf");
	}
}

void CDemoRecorder::WriteSetupText(const std::string& text)
{
	int length = text.length();
	while (text.c_str()[length - 1] == '\0') {
		--length;
	}

	fileHeader.scriptSize = length;
	recordDemo.write(text.c_str(), length);
}

void CDemoRecorder::SaveToDemo(const unsigned char* buf, const unsigned length, const float modGameTime)
{
	DemoStreamChunkHeader chunkHeader;

	chunkHeader.modGameTime = modGameTime;
	chunkHeader.length = length;
	chunkHeader.swab();
	recordDemo.write((char*)&chunkHeader, sizeof(chunkHeader));
	recordDemo.write((char*)buf, length);
	fileHeader.demoStreamSize += length + sizeof(chunkHeader);
	recordDemo.flush();
}

void CDemoRecorder::SetName(const std::string& mapname)
{
	// Returns the current local time as "JJJJMMDD_HHmmSS", eg: "20091231_115959"
	const std::string curTime = CTimeUtil::GetCurrentTimeStr();

	std::string name =
		curTime +
		"_" +
		mapname.substr(0, mapname.find_first_of(".")) +
		"_" +
		SpringVersion::Get();

	// games without gameSetup should have different names
	// note: the gameSetup pointer is non-0 when CPreGame
	// wants to rename us even in non-scripted games now
	if (!gameSetup || wantedName.find("local_") != std::string::npos) {
		name = "local_" + name;
	}

	char buf[1024];
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
