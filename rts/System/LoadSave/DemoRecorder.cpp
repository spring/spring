/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DemoRecorder.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Sim/Misc/TeamStatistics.h"
#include "System/Util.h"
#include "System/TimeUtil.h"

#include "System/Log/ILog.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fstream>

CDemoRecorder::CDemoRecorder(const std::string& mapName, const std::string& modName): demoStream(std::ios::binary | std::ios::out)
{
	SetName(mapName, modName);
	SetFileHeader();
}

CDemoRecorder::~CDemoRecorder()
{
	WriteWinnerList();
	WritePlayerStats();
	WriteTeamStats();
	WriteFileHeader(true);
	WriteDemoFile(dataDirsAccess.LocateFile(demoName, FileQueryFlags::WRITE), demoStream.str());
}

void CDemoRecorder::SetFileHeader()
{
	memset(&fileHeader, 0, sizeof(DemoFileHeader));
	strcpy(fileHeader.magic, DEMOFILE_MAGIC);
	fileHeader.version = DEMOFILE_VERSION;
	fileHeader.headerSize = sizeof(DemoFileHeader);
	STRNCPY(fileHeader.versionString, (SpringVersion::GetSync()).c_str(), sizeof(fileHeader.versionString) - 1);
	fileHeader.unixTime = CTimeUtil::GetCurrentTime();
	fileHeader.playerStatElemSize = sizeof(PlayerStatistics);
	fileHeader.teamStatElemSize = sizeof(TeamStatistics);
	fileHeader.teamStatPeriod = TeamStatistics::statsPeriod;
	fileHeader.winningAllyTeamsSize = 0;

	demoStream.seekp(WriteFileHeader(false) + sizeof(DemoFileHeader));
}

void CDemoRecorder::WriteDemoFile(const std::string& name, const std::string& data)
{
	std::ofstream file;

	// using operator<<(basic_stringbuf*) requires the stream to be opened with std::ios::in
	// stringbuf::{eback(), egptr(), gptr()} are protected so we cannot access them directly
	// (plus data is not guaranteed to be stored contiguously) ==> the only clean OO solution
	// that avoids str()'s copy would be to supply our own stringbuffer backend to demoStream
	// which is slightly overdoing it
	file.open(name.c_str(), std::ios::binary | std::ios::out);
	file.write(data.c_str(), data.size());
	file.flush();
	file.close();
}

void CDemoRecorder::WriteSetupText(const std::string& text)
{
	int length = text.length();
	while (text.c_str()[length - 1] == '\0') {
		--length;
	}

	fileHeader.scriptSize = length;
	demoStream.write(text.c_str(), length);
}

void CDemoRecorder::SaveToDemo(const unsigned char* buf, const unsigned length, const float modGameTime)
{
	DemoStreamChunkHeader chunkHeader;

	chunkHeader.modGameTime = modGameTime;
	chunkHeader.length = length;
	chunkHeader.swab();
	demoStream.write((char*) &chunkHeader, sizeof(chunkHeader));
	demoStream.write((char*) buf, length);
	fileHeader.demoStreamSize += length + sizeof(chunkHeader);
}

void CDemoRecorder::SetName(const std::string& mapname, const std::string& modname)
{
	// We want this folder to exist
	if (!FileSystem::CreateDirectory("demos"))
		return;

	// Returns the current local time as "JJJJMMDD_HHmmSS", eg: "20091231_115959"
	const std::string curTime = CTimeUtil::GetCurrentTimeStr();

	std::ostringstream oss;
	std::ostringstream buf;

	oss << "demos/" << curTime << "_";
	oss << FileSystem::GetBasename(mapname);
	oss << "_";
	// FIXME: why is this not included?
	// oss << FileSystem::GetBasename(modname);
	// oss << "_";
	oss << SpringVersion::GetSync();
	buf << oss.str() << ".sdf";

	unsigned int n = 0;

	while (FileSystem::FileExists(buf.str()) && (n < 99)) {
		buf.str(""); // clears content
		buf << oss.str() << "_" << n++ << ".sdf";
	}

	demoName = buf.str();
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

void CDemoRecorder::InitializeStats(int numPlayers, int numTeams )
{
	fileHeader.numPlayers = numPlayers;
	fileHeader.numTeams = numTeams;

	playerStats.resize(numPlayers);
	teamStats.resize(numTeams);
}

/** @brief Set (overwrite) the CPlayer::Statistics for player playerNum */
void CDemoRecorder::SetPlayerStats(int playerNum, const PlayerStatistics& stats)
{
	assert((unsigned)playerNum < playerStats.size());

	playerStats[playerNum] = stats;
}

/** @brief Set (overwrite) the TeamStatistics history for team teamNum */
void CDemoRecorder::SetTeamStats(int teamNum, const std::list< TeamStatistics >& stats)
{
	assert((unsigned)teamNum < teamStats.size());

	teamStats[teamNum].clear();
	teamStats[teamNum].reserve(stats.size());

	for (std::list< TeamStatistics >::const_iterator it = stats.begin(); it != stats.end(); ++it)
		teamStats[teamNum].push_back(*it);
}


/** @brief Set (overwrite) the list of winning allyTeams */
void CDemoRecorder::SetWinningAllyTeams(const std::vector<unsigned char>& winningAllyTeamIDs)
{
	fileHeader.winningAllyTeamsSize = (winningAllyTeamIDs.size() * sizeof(unsigned char));
	winningAllyTeams = winningAllyTeamIDs;
}

/** @brief Write DemoFileHeader
Write the DemoFileHeader at the start of the file and restores the original
position in the file afterwards. */
unsigned int CDemoRecorder::WriteFileHeader(bool updateStreamLength)
{
#ifdef _MSC_VER // MSVC8 behaves strange if tell/seek is called before anything has been written
	const bool empty = (demoStream.str() == "");
	const unsigned int pos = empty? 0 : demoStream.tellp();
#else
	const unsigned int pos = demoStream.tellp();
#endif

	DemoFileHeader tmpHeader;
	memcpy(&tmpHeader, &fileHeader, sizeof(fileHeader));
	if (!updateStreamLength)
		tmpHeader.demoStreamSize = 0;
	tmpHeader.swab(); // to little endian

#ifdef _MSC_VER
	if (!empty)
#endif
	{
		demoStream.seekp(0);
	}


	demoStream.write((char*) &tmpHeader, sizeof(tmpHeader));
	demoStream.seekp(pos);

	return pos;
}

/** @brief Write the CPlayer::Statistics at the current position in the file. */
void CDemoRecorder::WritePlayerStats()
{
	if (fileHeader.numPlayers == 0)
		return;

	int pos = demoStream.tellp();

	for (std::vector< PlayerStatistics >::iterator it = playerStats.begin(); it != playerStats.end(); ++it) {
		PlayerStatistics& stats = *it;
		stats.swab();
		demoStream.write(reinterpret_cast<char*>(&stats), sizeof(PlayerStatistics));
	}
	playerStats.clear();

	fileHeader.playerStatSize = (int)demoStream.tellp() - pos;
}



/** @brief Write the winningAllyTeams at the current position in the file. */
void CDemoRecorder::WriteWinnerList()
{
	if (fileHeader.numTeams == 0)
		return;

	const int pos = demoStream.tellp();

	// Write the array of winningAllyTeams.
	for (std::vector<unsigned char>::const_iterator it = winningAllyTeams.begin(); it != winningAllyTeams.end(); ++it) {
		demoStream.write((char*) &(*it), sizeof(unsigned char));
	}

	winningAllyTeams.clear();

	fileHeader.winningAllyTeamsSize = int(demoStream.tellp()) - pos;
}

/** @brief Write the TeamStatistics at the current position in the file. */
void CDemoRecorder::WriteTeamStats()
{
	if (fileHeader.numTeams == 0)
		return;

	int pos = demoStream.tellp();

	// Write array of dwords indicating number of TeamStatistics per team.
	for (std::vector< std::vector< TeamStatistics > >::iterator it = teamStats.begin(); it != teamStats.end(); ++it) {
		unsigned int c = swabDWord(it->size());
		demoStream.write((char*)&c, sizeof(unsigned int));
	}

	// Write big array of TeamStatistics.
	for (std::vector< std::vector< TeamStatistics > >::iterator it = teamStats.begin(); it != teamStats.end(); ++it) {
		for (std::vector< TeamStatistics >::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
			TeamStatistics& stats = *it2;
			stats.swab();
			demoStream.write(reinterpret_cast<char*>(&stats), sizeof(TeamStatistics));
		}
	}
	teamStats.clear();

	fileHeader.teamStatSize = (int)demoStream.tellp() - pos;
}
