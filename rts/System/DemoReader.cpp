#include "StdAfx.h"

#include <limits.h>
#include <stdexcept>
#include "mmgr.h"

#include "DemoReader.h"

#ifndef DEDICATED
#include "Sync/Syncify.h"
#endif
#include "Game/GameSetup.h"
#include "Net/RawPacket.h"
#include "Game/GameVersion.h"
#include "FileSystem/FileHandler.h"

/////////////////////////////////////
// CDemoReader implementation

CDemoReader::CDemoReader(const std::string& filename, float curTime)
{
	std::string firstTry = "demos/" + filename;

	playbackDemo = new CFileHandler(firstTry);

	if (!playbackDemo->FileExists()) {
		delete playbackDemo;
		playbackDemo = new CFileHandler(filename);
	}

	if (!playbackDemo->FileExists()) {
		// file not found -> exception
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile not found: ")+filename);
	}

	playbackDemo->Read((void*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic))
		|| fileHeader.version != DEMOFILE_VERSION
		|| fileHeader.headerSize != sizeof(fileHeader)
		// Don't compare spring version in debug mode: we don't want to make
		// debugging SVN demos impossible (because VERSION_STRING is different
		// each build.)
#ifndef _DEBUG
		|| strcmp(fileHeader.versionString, VERSION_STRING)
#endif
	) {
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile corrupt or created by a different version of Spring: ")+filename);
	}

	if (fileHeader.scriptSize != 0) {
		char* buf = new char[fileHeader.scriptSize];
		playbackDemo->Read(buf, fileHeader.scriptSize);
		if (!gameSetup) { // dont overwrite existing gamesetup (when hosting a demo)
			CGameSetup* temp = new CGameSetup();
			temp->Init(buf, fileHeader.scriptSize);
			temp->demoName = filename;
			temp->numDemoPlayers = GetFileHeader().maxPlayerNum+1;
			temp->maxSpeed = 10;
			gameSetup = temp;
		}
		delete[] buf;
	}

	playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
	chunkHeader.swab();

	demoTimeOffset = curTime - chunkHeader.modGameTime - 0.1f;
	nextDemoRead = curTime - 0.01f;

	if (fileHeader.demoStreamSize != 0) {
		bytesRemaining = fileHeader.demoStreamSize - sizeof(chunkHeader);
	} else {
		// Spring crashed while recording the demo: replay until EOF.
		bytesRemaining = INT_MAX;
	}
}

netcode::RawPacket* CDemoReader::GetData(float curTime)
{
	if (ReachedEnd())
		return 0;

	// when paused, modGameTime wont increase so no seperate check needed
	if (nextDemoRead < curTime) {
		netcode::RawPacket* buf = new netcode::RawPacket(chunkHeader.length);
		playbackDemo->Read((void*)(buf->data), chunkHeader.length);
		bytesRemaining -= chunkHeader.length;

		playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
		chunkHeader.swab();
		nextDemoRead = chunkHeader.modGameTime + demoTimeOffset;
		bytesRemaining -= sizeof(chunkHeader);

		return buf;
	} else {
		return 0;
	}
}

bool CDemoReader::ReachedEnd() const
{
	if (bytesRemaining <= 0 || playbackDemo->Eof())
		return true;
	else
		return false;
}

float CDemoReader::GetNextReadTime() const
{
	return chunkHeader.modGameTime;
}
