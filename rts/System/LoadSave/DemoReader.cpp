/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DemoReader.h"


#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Net/RawPacket.h"
#include "Game/GameVersion.h"

#include <limits.h>
#include <stdexcept>
#include <cassert>
#include <cstring>

CONFIG(bool, DisableDemoVersionCheck).defaultValue(false);

CDemoReader::CDemoReader(const std::string& filename, float curTime)
	: playbackDemo(NULL)
{
	playbackDemo = new CFileHandler(filename, SPRING_VFS_PWD_ALL);

	if (!playbackDemo->FileExists()) {
		// file not found -> exception
		throw user_error(std::string("Demofile not found: ")+filename);
	}

	playbackDemo->Read((char*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic))
		|| fileHeader.version != DEMOFILE_VERSION
		|| fileHeader.headerSize != sizeof(fileHeader)
		|| fileHeader.playerStatElemSize != sizeof(PlayerStatistics)
		|| fileHeader.teamStatElemSize != sizeof(TeamStatistics)
		// Don't compare spring version in debug mode: we don't want to make
		// debugging dev-version demos impossible (because the version is different
		// each build.)
#ifndef _DEBUG
		|| (SpringVersion::IsRelease() && strcmp(fileHeader.versionString, SpringVersion::GetSync().c_str()))
#endif
		) {
			std::string demoMsg = std::string("Demofile corrupt or created by a different version of Spring: ")+filename;
			if (!configHandler->GetBool("DisableDemoVersionCheck"))
				throw std::runtime_error(demoMsg);
			LOG_L(L_WARNING, "%s", demoMsg.c_str());
	}

	if (fileHeader.scriptSize != 0) {
		char* buf = new char[fileHeader.scriptSize];
		playbackDemo->Read(buf, fileHeader.scriptSize);
		setupScript = std::string(buf, fileHeader.scriptSize);
		delete[] buf;
	}

	playbackDemo->Read((char*)&chunkHeader, sizeof(chunkHeader));
	chunkHeader.swab();

	demoTimeOffset = curTime - chunkHeader.modGameTime - 0.1f;
	nextDemoReadTime = curTime - 0.01f;

	long curPos = playbackDemo->GetPos();
	playbackDemo->Seek(0, std::ios::end);
	playbackDemoSize = playbackDemo->GetPos();
	if (fileHeader.demoStreamSize != 0) {
		bytesRemaining = fileHeader.demoStreamSize;
	}
	else {
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


netcode::RawPacket* CDemoReader::GetData(float readTime)
{
	if (ReachedEnd())
		return NULL;

	// when paused, modGameTime does not increase (ie. we
	// always pass the same readTime value) so no seperate
	// check needed
	if (readTime >= nextDemoReadTime) {
		netcode::RawPacket* buf = new netcode::RawPacket(chunkHeader.length);
		if (playbackDemo->Read((char*)(buf->data), chunkHeader.length) < chunkHeader.length) {
			bytesRemaining = 0;
			return NULL;
		}
		bytesRemaining -= chunkHeader.length;

		if (bytesRemaining > 0 && !playbackDemo->Eof()) {
			long curPos = playbackDemo->GetPos();
			if (readTime == nextDemoReadTime) {
				playbackDemo->Seek(0, std::ios::end);
				if (playbackDemoSize != playbackDemo->GetPos())
					readTime = -nextDemoReadTime;
				playbackDemo->Seek(curPos);
			}
			if (curPos < playbackDemoSize) {
				// read next chunk header
				if (playbackDemo->Read((char*)&chunkHeader, sizeof(chunkHeader)) < sizeof(chunkHeader)) {
					bytesRemaining = 0;
					return NULL;
				}
				chunkHeader.swab();
				nextDemoReadTime = chunkHeader.modGameTime + demoTimeOffset;
				bytesRemaining -= sizeof(chunkHeader);
			}
		}

		return (readTime < 0) ? NULL : buf;
	} else {
		return NULL;
	}
}

bool CDemoReader::ReachedEnd()
{
	if (bytesRemaining <= 0 || playbackDemo->Eof() ||
		(playbackDemo->GetPos() > playbackDemoSize) )
		return true;
	else
		return false;
}


void CDemoReader::LoadStats()
{
	// Stats are not available if Spring crashed while writing the demo.
	if (fileHeader.demoStreamSize == 0) {
		return;
	}

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
		playbackDemo->Read((char*) &buf, sizeof(buf));
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
				playbackDemo->Read((char*) &buf, sizeof(buf));
				buf.swab();
				teamStats[teamNum].push_back(buf);
			}
		}
	}

	playbackDemo->Seek(curPos);
}
