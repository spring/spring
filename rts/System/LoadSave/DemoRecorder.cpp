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


CDemoRecorder::CDemoRecorder(const std::string& mapName, const std::string& modName, bool serverDemo):
demoStream(std::ios::binary | std::ios::out)
{
	SetName(mapName, modName, serverDemo);
	SetFileHeader();

	file = gzopen(demoName.c_str(), "wb9");
}

CDemoRecorder::~CDemoRecorder()
{
	LOG("Writing demo: %s", GetName().c_str());
	WriteWinnerList();
	WritePlayerStats();
	WriteTeamStats();
	WriteFileHeader(true);
	WriteDemoFile();
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

void CDemoRecorder::WriteDemoFile()
{
	// using operator<<(basic_stringbuf*) requires the stream to be opened with std::ios::in
	// stringbuf::{eback(), egptr(), gptr()} are protected so we cannot access them directly
	// (plus data is not guaranteed to be stored contiguously) ==> the only clean OO solution
	// that avoids str()'s copy would be to supply our own stringbuffer backend to demoStream
	// which is slightly overdoing it
	const std::string data = demoStream.str();
	gzwrite(file, data.c_str(), data.size());
	gzflush(file, Z_FINISH);
	gzclose(file);
}

void CDemoRecorder::WriteSetupText(const std::string& text)
{
	int length = text.length();
	while (text[length - 1] == '\0') {
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

void CDemoRecorder::SetName(const std::string& mapName, const std::string& modName, bool serverDemo)
{
	// Returns the current local time as "JJJJMMDD_HHmmSS", eg: "20091231_115959"
	const std::string curTime = CTimeUtil::GetCurrentTimeStr();
	const std::string demoDir = serverDemo? "demos-server/": "demos/";

	// We want this folder to exist
	if (!FileSystem::CreateDirectory(demoDir))
		return;

	std::ostringstream oss;
	std::ostringstream buf;

	oss << demoDir << curTime << "_";
	oss << FileSystem::GetBasename(mapName);
	oss << "_";
	// FIXME: why is this not included?
	// oss << FileSystem::GetBasename(modName);
	// oss << "_";
	oss << SpringVersion::GetSync();
	buf << oss.str() << ".sdfz";

	int n = 0;
	while (FileSystem::FileExists(buf.str()) && (n < 99)) {
		buf.str(""); // clears content
		buf << oss.str() << "_" << n++ << ".sdfz";
	}

	demoName = dataDirsAccess.LocateFile(buf.str(), FileQueryFlags::WRITE);
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

void CDemoRecorder::InitializeStats(int numPlayers, int numTeams)
{
	playerStats.resize(numPlayers);
	teamStats.resize(numTeams);
}


void CDemoRecorder::AddNewPlayer(const std::string& name, int playerNum)
{
	if (playerNum >= playerStats.size()) {
		playerStats.resize(playerNum + 1);
	}
}


/** @brief Set (overwrite) the CPlayer::Statistics for player playerNum */
void CDemoRecorder::SetPlayerStats(int playerNum, const PlayerStatistics& stats)
{
	if (playerNum >= playerStats.size()) {
		playerStats.resize(playerNum + 1);
	}

	playerStats[playerNum] = stats;
}

/** @brief Set (overwrite) the TeamStatistics history for team teamNum */
void CDemoRecorder::SetTeamStats(int teamNum, const std::list< TeamStatistics >& stats)
{
	assert((unsigned)teamNum < teamStats.size()); //FIXME

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
	int pos = demoStream.tellp();

	for (PlayerStatistics& stats: playerStats) {
		stats.swab();
		demoStream.write(reinterpret_cast<char*>(&stats), sizeof(PlayerStatistics));
	}

	fileHeader.numPlayers = playerStats.size();
	fileHeader.playerStatSize = (int)demoStream.tellp() - pos;

	playerStats.clear();
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
	int pos = demoStream.tellp();

	// Write array of dwords indicating number of TeamStatistics per team.
	for (std::vector<TeamStatistics>& history: teamStats) {
		unsigned int c = swabDWord(history.size());
		demoStream.write((char*)&c, sizeof(unsigned int));
	}

	// Write big array of TeamStatistics.
	for (std::vector<TeamStatistics>& history: teamStats) {
		for (TeamStatistics& stats: history) {
			stats.swab();
			demoStream.write(reinterpret_cast<char*>(&stats), sizeof(TeamStatistics));
		}
	}

	fileHeader.numTeams = teamStats.size();
	fileHeader.teamStatSize = (int)demoStream.tellp() - pos;

	teamStats.clear();
}
