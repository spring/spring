#include "StdAfx.h"
#include <fstream>
#include "mmgr.h"

#include "ExternalAI/EngineOutHandler.h"
#include "LoadSaveHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Platform/errorhandler.h"
#include "FileSystem/FileSystem.h"
#include "creg/Serializer.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GameServer.h"
#include "Sim/Misc/TeamHandler.h"
#include "LogOutput.h"
#include "Game/WaitCommandsAI.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Game/GameServer.h"
#include "Rendering/InMapDraw.h"
#include "GlobalUnsynced.h"
#include "Exceptions.h"

CLoadSaveHandler::CLoadSaveHandler(void)
{}

CLoadSaveHandler::~CLoadSaveHandler(void)
{}

class CGameStateCollector
{
	CR_DECLARE(CGameStateCollector);

public:
	CGameStateCollector() {}

	void Serialize(creg::ISerializer& s);

	std::string mapName, modName;
};

CR_BIND(CGameStateCollector, );
CR_REG_METADATA(CGameStateCollector, CR_SERIALIZER(Serialize));

static void WriteString(std::ostream& s, std::string& str)
{
	char c;
	// write the string char by char
	for (unsigned int a=0; a < str.length(); a++) {
		c = str[a];
		s.write(&c, sizeof(char));
	}
	// write the string-termination NULL char
	c = '\0';
	s.write(&c,sizeof(char));
}

static void ReadString(std::istream& s, std::string& str)
{
	char c;
	// read until string-termination NULL char is encountered
	s.read(&c, sizeof(char));
	while (c != '\0') {
		str += c;
		s.read(&c, sizeof(char));
	}
}

void CGameStateCollector::Serialize(creg::ISerializer& s)
{
	// GetClass() works because readmap and uh both have to exist already
	s.SerializeObjectInstance(gs, gs->GetClass());
	s.SerializeObjectInstance(gu, gu->GetClass());
	s.SerializeObjectInstance(game, game->GetClass());
	s.SerializeObjectInstance(readmap, readmap->GetClass());
	s.SerializeObjectInstance(qf, qf->GetClass());
	s.SerializeObjectInstance(featureHandler, featureHandler->GetClass());
	s.SerializeObjectInstance(loshandler, loshandler->GetClass());
	s.SerializeObjectInstance(radarhandler, radarhandler->GetClass());
	s.SerializeObjectInstance(airBaseHandler, airBaseHandler->GetClass());
	s.SerializeObjectInstance(&interceptHandler, interceptHandler.GetClass());
	s.SerializeObjectInstance(CCategoryHandler::Instance(), CCategoryHandler::Instance()->GetClass());
	s.SerializeObjectInstance(uh, uh->GetClass());
	s.SerializeObjectInstance(ph, ph->GetClass());
//	std::map<std::string, int> unitRestrictions;
	s.SerializeObjectInstance(&waitCommandsAI, waitCommandsAI.GetClass());
	s.SerializeObjectInstance(&wind, wind.GetClass());
	s.SerializeObjectInstance(inMapDrawer,inMapDrawer->GetClass());
	for (int a=0; a < teamHandler->ActiveTeams(); a++) {
		s.SerializeObjectInstance(grouphandlers[a], grouphandlers[a]->GetClass());
	}
	s.SerializeObjectInstance(eoh, eoh->GetClass());
	s.SerializeObjectInstance(&CBuilderCAI::reclaimers,CBuilderCAI::reclaimers.GetClass());
//	s.Serialize()
}

void PrintSize(const char *txt, int size)
{
	if (size>1024*1024*1024) logOutput.Print("%s %.1f GB",txt,size/(1024.0f*1024*1024)); else
	if (size>     1024*1024) logOutput.Print("%s %.1f MB",txt,size/(1024.0f*1024     )); else
	if (size>          1024) logOutput.Print("%s %.1f KB",txt,size/(1024.0f          )); else
	logOutput.Print("%s %u B",txt,size);
}

void CLoadSaveHandler::SaveGame(const std::string& file)
{
	LoadStartPicture(teamHandler->Team(gu->myTeam)->side);
	PrintLoadMsg("Saving game");
	try {
		std::ofstream ofs(filesystem.LocateFile(file, FileSystem::WRITE).c_str(), std::ios::out|std::ios::binary);
		if (ofs.bad() || !ofs.is_open()) {
			throw content_error("Unable to save game to file \"" + file + "\"");
		}

		std::string scriptText;
		if (gameSetup) {
			scriptText = gameSetup->gameSetupText;
		}

		WriteString(ofs, scriptText);

		WriteString(ofs, modName);
		WriteString(ofs, mapName);

		CGameStateCollector *gsc = new CGameStateCollector();

		creg::COutputStreamSerializer os;
		os.SavePackage(&ofs, gsc, gsc->GetClass());
		PrintSize("Game",ofs.tellp());
		int aistart = ofs.tellp();
		for (int a=0; a < teamHandler->ActiveTeams();a++)
			grouphandlers[a]->Save(&ofs);
		eoh->Save(&ofs);
		PrintSize("AIs",((int)ofs.tellp())-aistart);
	} catch (content_error &e) {
		logOutput.Print("Save faild(content error): %s",e.what());
	} catch (std::exception &e) {
		logOutput.Print("Save faild: %s",e.what());
	} catch (char* &e) {
		logOutput.Print("Save faild: %s",e);
	} catch (...) {
		logOutput.Print("Save faild(unknwon error)");
	}
	UnloadStartPicture();
}

/// this just loads the mapname and some other early stuff
void CLoadSaveHandler::LoadGameStartInfo(const std::string& file)
{
	ifs = new std::ifstream (filesystem.LocateFile(file).c_str(), std::ios::in|std::ios::binary);

	// in case these contained values alredy
	// (this is the case when loading a game through the spring menu eg),
	// we set them to empty strings, as ReadString() does append,
	// and we would end up with the correct value but two times
	// eg: "AbcAbc" instead of "Abc"
	scriptText = "";
	modName = "";
	mapName = "";

	ReadString(*ifs, scriptText);
	if (!scriptText.empty() && !gameSetup) {
		CGameSetup* temp = new CGameSetup();
		if (!temp->Init(scriptText)) {
			delete temp;
			temp = 0;
		} else {
			temp->saveName = file;
			gameSetup = temp;
		}
	}

	ReadString(*ifs, modName);
	ReadString(*ifs, mapName);
}

/// this should be called on frame 0 when the game has started
void CLoadSaveHandler::LoadGame()
{
	LoadStartPicture(teamHandler->Team(gu->myTeam)->side);
	PrintLoadMsg("Loading game");
	creg::CInputStreamSerializer inputStream;
	void *pGSC = 0;
	creg::Class* gsccls = 0;
	inputStream.LoadPackage(ifs, pGSC, gsccls);

	assert (pGSC && gsccls == CGameStateCollector::StaticClass());

	CGameStateCollector *gsc = (CGameStateCollector *)pGSC;
	delete gsc; // the only job of gsc is to collect gamestate data
	for (int a=0; a < teamHandler->ActiveTeams();a++)
		grouphandlers[a]->Load(ifs);
	eoh->Load(ifs);
	delete ifs;
	for (int a=0; a < teamHandler->ActiveTeams();a++) { // For old savegames
		if (teamHandler->Team(a)->isDead && eoh->IsSkirmishAI(a)) {
			eoh->DestroySkirmishAI(a);
		}
	}
	gs->paused = false;
	if (gameServer) {
		gameServer->isPaused = false;
		gameServer->syncErrorFrame = 0;
	}
	UnloadStartPicture();
}

std::string CLoadSaveHandler::FindSaveFile(const char* name)
{
	std::string name2 = name;
#ifdef _WIN32
	if (name2.find(":\\")==std::string::npos)
		name2 = "Saves\\" + name2;
#else
	if (name2.find("/")==std::string::npos)
		name2 = "Saves/" + name2;
#endif
	return name2;
}
