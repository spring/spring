#include "StdAfx.h"
#include "DemoReader.h"

#include <limits.h>
#include <stdexcept>
#include <assert.h>
#include "mmgr.h"

#include "Net/RawPacket.h"
#include "Game/GameVersion.h"

/////////////////////////////////////
// CDemoReader implementation

CDemoReader::CDemoReader(const std::string& filename, float curTime)
{
	playbackDemo.open(filename.c_str(), std::ios::binary);

	if (!playbackDemo.is_open()) {
		// file not found -> exception
		throw std::runtime_error(std::string("Demofile not found: ")+filename);
	}

	playbackDemo.read((char*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic))
		|| fileHeader.version != DEMOFILE_VERSION
		|| fileHeader.headerSize != sizeof(fileHeader)
		|| fileHeader.playerStatElemSize != sizeof(PlayerStatistics)
		// || fileHeader.teamStatElemSize != sizeof(TeamStatistics)
		// Don't compare spring version in debug mode: we don't want to make
		// debugging SVN demos impossible (because VERSION_STRING is different
		// each build.)
#ifndef _DEBUG
		|| (SpringVersion::Get().find("+") == std::string::npos && strcmp(fileHeader.versionString, SpringVersion::Get().c_str()))
#endif
	) {
		throw std::runtime_error(std::string("Demofile corrupt or created by a different version of Spring: ")+filename);
	}

	if (fileHeader.scriptSize != 0) {
		char* buf = new char[fileHeader.scriptSize];
		playbackDemo.read(buf, fileHeader.scriptSize);
		setupScript = std::string(buf);
		delete[] buf;
	}

	const int streamStartPos = playbackDemo.tellg();

	// Loop through all players and read and output the statistics for each.
	for (int playerNum = 0; playerNum < fileHeader.numPlayers; ++playerNum)
	{
		PlayerStatistics buf;
		playbackDemo.read((char*)&buf, sizeof(buf));
		playerStats.push_back(buf);
	}

	{ // Team statistics follow player statistics.
		teamStats.resize(fileHeader.numTeams);
		// Read the array containing the number of team stats for each team.
		std::vector<int> numStatsPerTeam(fileHeader.numTeams, 0);
		playbackDemo.read((char*)(numStatsPerTeam[0]), numStatsPerTeam.size());

		// Loop through all team stats for each team and read and output them.
		// We keep track of the gametime while reading the stats for a team so we
		// can output it too.
		for (int teamNum = 0; teamNum < fileHeader.numTeams; ++teamNum)
		{
			for (int i = 0; i < numStatsPerTeam[teamNum]; ++i)
			{
				TeamStatistics buf;
				playbackDemo.read((char*)&buf, sizeof(buf));
				teamStats[teamNum].push_back(buf);
			}
		}
	}

	playbackDemo.seekg(streamStartPos);

	playbackDemo.read((char*)&chunkHeader, sizeof(chunkHeader));
	chunkHeader.swab();

	demoTimeOffset = curTime - chunkHeader.modGameTime - 0.1f;
	nextDemoRead = curTime - 0.01f;
}

netcode::RawPacket* CDemoReader::GetData(float curTime)
{
	if (ReachedEnd())
		return 0;

	// when paused, modGameTime wont increase so no seperate check needed
	if (nextDemoRead < curTime) {
		netcode::RawPacket* buf = new netcode::RawPacket(chunkHeader.length);
		playbackDemo.read((char*)(buf->data), chunkHeader.length);

		playbackDemo.read((char*)&chunkHeader, sizeof(chunkHeader));
		chunkHeader.swab();
		nextDemoRead = chunkHeader.modGameTime + demoTimeOffset;

		return buf;
	} else {
		return 0;
	}
}

bool CDemoReader::ReachedEnd() const
{
	if (playbackDemo.eof())
		return true;
	else
		return false;
}

float CDemoReader::GetNextReadTime() const
{
	return chunkHeader.modGameTime;
}
