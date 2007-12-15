#include "DemoReader.h"

#include <limits.h>
#include <stdexcept>

#ifndef DEDICATED
#include "Sync/Syncify.h"
#include "Game/GameSetup.h"
#endif
#include "Game/GameVersion.h"


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
#ifndef DEBUG
		|| strcmp(fileHeader.versionString, VERSION_STRING)
#endif
	) {
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile corrupt or created by a different version of Spring: ")+filename);
	}

	if (fileHeader.scriptSize != 0) {
#ifndef DEDICATED
		char* buf = new char[fileHeader.scriptSize];
		playbackDemo->Read(buf, fileHeader.scriptSize);

		if (!gameSetup) { // dont overwrite existing gamesetup (when hosting a demo)
			gameSetup = SAFE_NEW CGameSetup();
			gameSetup->Init(buf, fileHeader.scriptSize);
		}
		delete[] buf;
#endif
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

unsigned CDemoReader::GetData(unsigned char *buf, const unsigned length, float curTime)
{
	if (ReachedEnd())
		return 0;

	// when paused, modGameTime wont increase so no seperate check needed
	if (nextDemoRead < curTime) {
		playbackDemo->Read((void*)(buf), chunkHeader.length);
		unsigned ret = chunkHeader.length;
		bytesRemaining -= chunkHeader.length;

		playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
		chunkHeader.swab();
		nextDemoRead = chunkHeader.modGameTime + demoTimeOffset;
		bytesRemaining -= sizeof(chunkHeader);

		return ret;
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
