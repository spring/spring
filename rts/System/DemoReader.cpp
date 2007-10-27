#include "DemoReader.h"

#include <stdexcept>

#include "Sync/Syncify.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"


/////////////////////////////////////
// CDemoReader implementation

CDemoReader::CDemoReader(const std::string& filename)
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

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic)) || fileHeader.version != DEMOFILE_VERSION || fileHeader.headerSize != sizeof(fileHeader) || strcmp(fileHeader.versionString, VERSION_STRING)) {
		delete playbackDemo;
		playbackDemo = NULL;
		throw std::runtime_error(std::string("Demofile corrupt or created by a different version of Spring: ")+filename);
	}

	if (fileHeader.scriptSize != 0) {
		char* buf = new char[fileHeader.scriptSize];
		playbackDemo->Read(buf, fileHeader.scriptSize);

		if (!gameSetup) { // dont overwrite existing gamesetup (when hosting a demo)
			gameSetup = SAFE_NEW CGameSetup();
			gameSetup->Init(buf, fileHeader.scriptSize);
		}
		delete[] buf;
	}

	playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
	chunkHeader.swab();

	demoTimeOffset = gu->modGameTime - chunkHeader.modGameTime - 0.1f;
	nextDemoRead = gu->modGameTime - 0.01f;
	bytesRemaining = fileHeader.demoStreamSize - sizeof(chunkHeader);
}

unsigned CDemoReader::GetData(unsigned char *buf, const unsigned length)
{
	if (ReachedEnd())
		return 0;

	// when paused, modGameTime wont increase so no seperate check needed
	if (nextDemoRead < gu->modGameTime) {
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
