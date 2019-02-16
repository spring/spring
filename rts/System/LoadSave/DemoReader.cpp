/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DemoReader.h"

#ifndef TOOLS
#include "System/Config/ConfigHandler.h"
CONFIG(bool, DisableDemoVersionCheck).defaultValue(false).description("Allow to play every replay file (may crash / cause undefined behaviour in replays)");
#endif
#include "System/Exceptions.h"
#include "System/FileSystem/GZFileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Net/RawPacket.h"
#include "Game/GameVersion.h"

#include <climits>
#include <stdexcept>
#include <cassert>
#include <cstring>


CDemoReader::CDemoReader(const std::string& filename, float curTime): playbackDemo(new CGZFileHandler(filename, SPRING_VFS_PWD_ALL))
{
	if (FileSystem::GetExtension(filename) != "sdfz")
		throw content_error("Unknown demo extension: " + FileSystem::GetExtension(filename));

	// file not found -> exception
	if (!playbackDemo->FileExists())
		throw user_error("Demofile not found: " + filename);

	playbackDemo->Read((char*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic)) != 0
		|| fileHeader.version != DEMOFILE_VERSION
		|| fileHeader.headerSize != sizeof(fileHeader)
		|| fileHeader.playerStatElemSize != sizeof(PlayerStatistics)
		|| fileHeader.teamStatElemSize != sizeof(TeamStatistics)
		// Don't compare spring version in debug mode: we don't want to make
		// debugging dev-version demos impossible (because the version is different
		// each build.)
#ifndef _DEBUG
		|| (SpringVersion::IsRelease() && strcmp(fileHeader.versionString, SpringVersion::GetSync().c_str()) != 0)
#endif
		) {
			const std::string demoMsg = std::string("Demofile ") + filename + " corrupt or created by a different version of Spring, expects version " + fileHeader.versionString + ".";
#ifndef TOOLS
			if (!configHandler->GetBool("DisableDemoVersionCheck"))
				throw std::runtime_error(demoMsg);
#endif
			LOG_L(L_WARNING, "%s", demoMsg.c_str());
	}

	if (fileHeader.scriptSize != 0) {
		std::vector<char> buf(fileHeader.scriptSize);
		playbackDemo->Read(buf.data(), buf.size());
		setupScript = std::string(buf.data(), buf.size());
	}

	playbackDemo->Read((char*)&chunkHeader, sizeof(chunkHeader));
	chunkHeader.swab();

	demoTimeOffset = curTime - chunkHeader.modGameTime - 0.1f;
	nextDemoReadTime = curTime - 0.01f;

	const long curPos = playbackDemo->GetPos();
	playbackDemo->Seek(0, std::ios::end);
	playbackDemoSize = playbackDemo->GetPos();

	if (fileHeader.demoStreamSize != 0) {
		bytesRemaining = fileHeader.demoStreamSize;
	} else {
		// Spring crashed while recording the demo: replay until EOF,
		// but at most filesize bytes to block watching demo of running game.
		// For this we must determine the file size.
		// (if this had still used CFileHandler that would have been easier ;-))
		bytesRemaining = playbackDemoSize - curPos;
	}
	playbackDemo->Seek(curPos);
}


CDemoReader::~CDemoReader()
{
	delete playbackDemo;
}


netcode::RawPacket* CDemoReader::GetData(const float readTime)
{
	if (ReachedEnd())
		return nullptr;

	// when paused, modGameTime does not increase (ie. we
	// always pass the same readTime value) so no seperate
	// check needed
	if (readTime >= nextDemoReadTime) {
		netcode::RawPacket* buf = new netcode::RawPacket(chunkHeader.length);
		if (playbackDemo->Read((char*)(buf->data), chunkHeader.length) < chunkHeader.length) {
			delete buf;
			bytesRemaining = 0;
			return nullptr;
		}
		bytesRemaining -= chunkHeader.length;

		if (!ReachedEnd()) {
			// read next chunk header
			if (playbackDemo->Read((char*)&chunkHeader, sizeof(chunkHeader)) < sizeof(chunkHeader)) {
				delete buf;
				bytesRemaining = 0;
				return nullptr;
			}
			chunkHeader.swab();
			nextDemoReadTime = chunkHeader.modGameTime + demoTimeOffset;
			bytesRemaining -= sizeof(chunkHeader);
		}
		if (readTime < 0) {
			delete buf;
			return nullptr;
		}
		return buf;
	}

	return nullptr;
}

bool CDemoReader::ReachedEnd()
{
	return (bytesRemaining <= 0 || playbackDemo->Eof() || (playbackDemo->GetPos() > playbackDemoSize));
}


void CDemoReader::LoadStats()
{
	// Stats are not available if Spring crashed while writing the demo.
	if (fileHeader.demoStreamSize == 0)
		return;

	const int curPos = playbackDemo->GetPos();
	playbackDemo->Seek(fileHeader.headerSize + fileHeader.scriptSize + fileHeader.demoStreamSize);

	winningAllyTeams.clear();
	playerStats.clear();
	teamStats.clear();

	for (int allyTeamNum = 0; allyTeamNum < fileHeader.winningAllyTeamsSize; ++allyTeamNum) {
		unsigned char winnerAllyTeam;
		playbackDemo->Read((char*) &winnerAllyTeam, sizeof(unsigned char));
		winningAllyTeams.push_back(winnerAllyTeam);
	}

	for (int playerNum = 0; playerNum < fileHeader.numPlayers; ++playerNum) {
		PlayerStatistics buf;
		playbackDemo->Read(reinterpret_cast<char*>(&buf), sizeof(PlayerStatistics));
		buf.swab();
		playerStats.push_back(buf);
	}

	{ // Team statistics follow player statistics.
		teamStats.resize(fileHeader.numTeams);
		// Read the array containing the number of team stats for each team.
		std::vector<int> numStatsPerTeam(fileHeader.numTeams, 0);
		playbackDemo->Read((char*) (&numStatsPerTeam[0]), numStatsPerTeam.size());

		for (int teamNum = 0; teamNum < fileHeader.numTeams; ++teamNum) {
			for (int i = 0; i < numStatsPerTeam[teamNum]; ++i) {
				TeamStatistics buf;
				playbackDemo->Read(reinterpret_cast<char*>(&buf), sizeof(TeamStatistics));
				buf.swab();
				teamStats[teamNum].push_back(buf);
			}
		}
	}

	playbackDemo->Seek(curPos);
}
