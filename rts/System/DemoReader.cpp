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
		// || fileHeader.playerStatElemSize != sizeof(PlayerStatistics)
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
