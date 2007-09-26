#include "DemoReader.h"


#include "Sync/Syncify.h"
#include "LogOutput.h"
#include "Game/Team.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Lua/LuaGaia.h" // FIXME: should not be here
#include "Lua/LuaRules.h"
#include "Platform/ConfigHandler.h"


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
		logOutput.Print("Demo file not found: %s",filename.c_str());
		delete playbackDemo;
		playbackDemo = NULL;
		return;
	}

	playbackDemo->Read((void*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic)) ||
		   fileHeader.version != DEMOFILE_VERSION ||
		   fileHeader.headerSize != sizeof(fileHeader) ||
		   strcmp(fileHeader.versionString, VERSION_STRING)) {
		logOutput.Print("Demofile corrupt or created by a different version of Spring: %s", filename.c_str());
		delete playbackDemo;
		playbackDemo = NULL;
		return;
		   }

		   logOutput.Print("Playing demo from %s",filename.c_str());
		   if (fileHeader.scriptPtr != 0) {
			   char* buf = new char[fileHeader.scriptSize];
			   playbackDemo->Seek(fileHeader.scriptPtr);
			   playbackDemo->Read(buf, fileHeader.scriptSize);

			   if (!gameSetup) { // dont overwrite existing gamesetup (when hosting a demo)
				   gameSetup = SAFE_NEW CGameSetup();
				   gameSetup->Init(buf, fileHeader.scriptSize);
			   }
			   delete[] buf;

		   } else {

			   logOutput.Print("Demo file does not contain fileHeader data");
				// Didn't get a CGameSetup script
				// FIXME: duplicated in Main.cpp
			   const string luaGaiaStr  = configHandler.GetString("LuaGaia",  "1");
			   const string luaRulesStr = configHandler.GetString("LuaRules", "1");
			   gs->useLuaGaia  = CLuaGaia::SetConfigString(luaGaiaStr);
			   gs->useLuaRules = CLuaRules::SetConfigString(luaRulesStr);
			   if (gs->useLuaGaia) {
				   gs->gaiaTeamID = gs->activeTeams;
				   gs->gaiaAllyTeamID = gs->activeAllyTeams;
				   gs->activeTeams++;
				   gs->activeAllyTeams++;
				   CTeam* team = gs->Team(gs->gaiaTeamID);
				   team->color[0] = 255;
				   team->color[1] = 255;
				   team->color[2] = 255;
				   team->color[3] = 255;
				   team->gaia = true;
				   gs->SetAllyTeam(gs->gaiaTeamID, gs->gaiaAllyTeamID);
			   }
		   }

		   gu->spectating           = true;
		   gu->spectatingFullView   = true;
		   gu->spectatingFullSelect = true;

		   playbackDemo->Seek(fileHeader.demoStreamPtr);
		   playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
		   chunkHeader.swab();

		   demoTimeOffset = gu->modGameTime - chunkHeader.modGameTime - 0.1f;
		   nextDemoRead = gu->modGameTime - 0.01f;
		   bytesRemaining = fileHeader.demoStreamSize - sizeof(chunkHeader);
}

unsigned CDemoReader::GetData(unsigned char *buf, const unsigned length)
{
	if(gs->paused)
	{
		return 0;
	}

	if (ReachedEnd())
	{
		return 0;
	}

	
	if (nextDemoRead < gu->modGameTime) {
		playbackDemo->Read((void*)(buf), chunkHeader.length);
		unsigned ret = chunkHeader.length;
		bytesRemaining -= chunkHeader.length;

		playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
		chunkHeader.swab();
		nextDemoRead = chunkHeader.modGameTime + demoTimeOffset;
		bytesRemaining -= sizeof(chunkHeader);

		if (bytesRemaining <= 0 || playbackDemo->Eof()) {
			logOutput.Print("End of demo");
		}
		return ret;
	}
	else
	{
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
