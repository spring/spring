/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <cerrno>
#include <cstring>
#include <memory>

#include "DemoRecorder.h"
#include "Game/GameVersion.h"
#include "Sim/Misc/TeamStatistics.h"
#include "System/TimeUtil.h"
#include "System/StringUtil.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Threading/ThreadPool.h"

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif


// server and client memory-streams
static std::string demoStreams[2];
static spring::mutex demoMutex;


CDemoRecorder::CDemoRecorder(const std::string& mapName, const std::string& modName, bool serverDemo): isServerDemo(serverDemo)
{
	std::lock_guard<spring::mutex> lock(demoMutex);

	SetStream();
	SetName(mapName, modName);
	SetFileHeader();
	WriteFileHeader(false);

	file = gzopen(demoName.c_str(), "wb9");
}

CDemoRecorder::~CDemoRecorder()
{
	WriteWinnerList();
	WritePlayerStats();
	WriteTeamStats();
	WriteFileHeader(true);
	WriteDemoFile();
}


void CDemoRecorder::SetStream()
{
	demoStreams[isServerDemo].clear();
	demoStreams[isServerDemo].reserve(8 * 1024 * 1024);
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
}

void CDemoRecorder::WriteDemoFile()
{
	// zlib FAQ claims the lib is thread-safe, "however any library routines that zlib uses and
	// any application-provided memory allocation routines must also be thread-safe. zlib's gz*
	// functions use stdio library routines, and most of zlib's functions use the library memory
	// allocation routines by default" (so code below should be OK)
	// gz* should usually be finished before ctor runs again when reloading, but take no chances
	std::string& data = demoStreams[isServerDemo];
	std::function<void(gzFile, std::string&)> func = [](gzFile file, std::string& data) {
		std::lock_guard<spring::mutex> lock(demoMutex);

		gzwrite(file, data.c_str(), data.size());
		gzflush(file, Z_FINISH);
		gzclose(file);
	};

	LOG("[%s] writing %s-demo \"%s\" (%u bytes)", __func__, (isServerDemo? "server": "client"), demoName.c_str(), static_cast<unsigned int>(data.size()));

	#ifndef WIN32
	// NOTE: can not use ThreadPool for this directly here, workers are already gone
	// FIXME: does not currently (august 2017) compile on Windows mingw buildbots
	ThreadPool::AddExtJob(spring::thread(std::move(func), file, std::ref(data)));
	#else
	ThreadPool::AddExtJob(std::move(std::async(std::launch::async, std::move(func), file, std::ref(data))));
	#endif
}

void CDemoRecorder::WriteSetupText(const std::string& text)
{
	int length = text.length();
	while (text[length - 1] == '\0') {
		--length;
	}

	fileHeader.scriptSize = length;
	demoStreams[isServerDemo].append(text.c_str(), length);
}

void CDemoRecorder::SaveToDemo(const unsigned char* buf, const unsigned length, const float modGameTime)
{
	DemoStreamChunkHeader chunkHeader;

	chunkHeader.modGameTime = modGameTime;
	chunkHeader.length = length;
	chunkHeader.swab();
	demoStreams[isServerDemo].append(reinterpret_cast<const char*>(&chunkHeader), sizeof(chunkHeader));
	demoStreams[isServerDemo].append(reinterpret_cast<const char*>(buf), length);
	fileHeader.demoStreamSize += (length + sizeof(chunkHeader));
}

void CDemoRecorder::SetName(const std::string& mapName, const std::string& modName)
{
	// Returns the current local time as "JJJJMMDD_HHmmSS", eg: "20091231_115959"
	const std::string curTime = CTimeUtil::GetCurrentTimeStr();
	const std::string demoDir = isServerDemo? "demos-server/": "demos/";

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
	// must be here so WriteWinnerList works
	teamStats.resize(fileHeader.numTeams = numTeams);
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
	if (playerNum >= playerStats.size())
		playerStats.resize(playerNum + 1);

	playerStats[playerNum] = stats;
}

/** @brief Set (overwrite) the TeamStatistics history for team teamNum */
void CDemoRecorder::SetTeamStats(int teamNum, const std::vector<TeamStatistics>& stats)
{
	assert((unsigned)teamNum < teamStats.size()); //FIXME

	teamStats[teamNum].clear();
	teamStats[teamNum].reserve(stats.size());

	for (const auto& stat: stats) {
		teamStats[teamNum].push_back(stat);
	}
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
	DemoFileHeader tmpHeader;
	memcpy(&tmpHeader, &fileHeader, sizeof(fileHeader));

	if (!updateStreamLength)
		tmpHeader.demoStreamSize = 0;

	// to little endian
	tmpHeader.swab();

	if (demoStreams[isServerDemo].empty()) {
		demoStreams[isServerDemo].append(reinterpret_cast<const char*>(&tmpHeader), sizeof(tmpHeader));
	} else {
		assert(demoStreams[isServerDemo].size() >= sizeof(tmpHeader));
		memcpy(&demoStreams[isServerDemo][0], reinterpret_cast<const char*>(&tmpHeader), sizeof(tmpHeader)); // no non-const .data() until C++17
	}

	return (demoStreams[isServerDemo].size());
}

/** @brief Write the CPlayer::Statistics at the current position in the file. */
void CDemoRecorder::WritePlayerStats()
{
	const size_t pos = demoStreams[isServerDemo].size();

	for (PlayerStatistics& stats: playerStats) {
		stats.swab();
		demoStreams[isServerDemo].append(reinterpret_cast<const char*>(&stats), sizeof(PlayerStatistics));
	}

	fileHeader.numPlayers = playerStats.size();
	fileHeader.playerStatSize = int(demoStreams[isServerDemo].size() - pos);

	playerStats.clear();
}



/** @brief Write the winningAllyTeams at the current position in the file. */
void CDemoRecorder::WriteWinnerList()
{
	if (fileHeader.numTeams == 0)
		return;

	const size_t pos = demoStreams[isServerDemo].size();

	// Write the array of winningAllyTeams.
	for (size_t i = 0; i < winningAllyTeams.size(); i++) { // NOLINT{modernize-loop-convert}
		demoStreams[isServerDemo].append(reinterpret_cast<const char*>(&winningAllyTeams[i]), sizeof(unsigned char));
	}

	winningAllyTeams.clear();

	fileHeader.winningAllyTeamsSize = int(demoStreams[isServerDemo].size() - pos);
}

/** @brief Write the TeamStatistics at the current position in the file. */
void CDemoRecorder::WriteTeamStats()
{
	const size_t pos = demoStreams[isServerDemo].size();

	// Write array of dwords indicating number of TeamStatistics per team.
	for (std::vector<TeamStatistics>& history: teamStats) {
		unsigned int c = swabDWord(history.size());
		demoStreams[isServerDemo].append(reinterpret_cast<const char*>(&c), sizeof(unsigned int));
	}

	// Write big array of TeamStatistics.
	for (std::vector<TeamStatistics>& history: teamStats) {
		for (TeamStatistics& stats: history) {
			stats.swab();
			demoStreams[isServerDemo].append(reinterpret_cast<const char*>(&stats), sizeof(TeamStatistics));
		}
	}

	fileHeader.teamStatSize = int(demoStreams[isServerDemo].size() - pos);

	teamStats.clear();
}
