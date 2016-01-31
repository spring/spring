/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <fstream>

#include "ExternalAI/EngineOutHandler.h"
#include "CregLoadSaveHandler.h"
#include "Map/ReadMap.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/WaitCommandsAI.h"
#include "Net/GameServer.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Game/UI/Groups/GroupHandler.h"

#include "System/Platform/errorhandler.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/creg/Serializer.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"


CCregLoadSaveHandler::CCregLoadSaveHandler()
	: ifs(NULL)
{}

CCregLoadSaveHandler::~CCregLoadSaveHandler()
{}

#ifdef USING_CREG
class CGameStateCollector
{
	CR_DECLARE_STRUCT(CGameStateCollector)

public:
	CGameStateCollector() {}

	void Serialize(creg::ISerializer* s);
};

CR_BIND(CGameStateCollector, )
CR_REG_METADATA(CGameStateCollector, (
	CR_SERIALIZER(Serialize)
))

void CGameStateCollector::Serialize(creg::ISerializer* s)
{
	s->SerializeObjectInstance(gs, gs->GetClass());
	s->SerializeObjectInstance(gu, gu->GetClass());
	s->SerializeObjectInstance(gameSetup, gameSetup->GetClass());
	s->SerializeObjectInstance(game, game->GetClass());
	s->SerializeObjectInstance(readMap, readMap->GetClass());
	s->SerializeObjectInstance(quadField, quadField->GetClass());
	s->SerializeObjectInstance(unitHandler, unitHandler->GetClass());
	s->SerializeObjectInstance(featureHandler, featureHandler->GetClass());
	s->SerializeObjectInstance(losHandler, losHandler->GetClass());
	s->SerializeObjectInstance(&interceptHandler, interceptHandler.GetClass());
	s->SerializeObjectInstance(CCategoryHandler::Instance(), CCategoryHandler::Instance()->GetClass());
	s->SerializeObjectInstance(projectileHandler, projectileHandler->GetClass());
	s->SerializeObjectInstance(&waitCommandsAI, waitCommandsAI.GetClass());
	s->SerializeObjectInstance(&wind, wind.GetClass());
	s->SerializeObjectInstance(moveDefHandler, moveDefHandler->GetClass());
	s->SerializeObjectInstance(teamHandler, teamHandler->GetClass());
	for (int a=0; a < teamHandler->ActiveTeams(); a++) {
		s->SerializeObjectInstance(grouphandlers[a], grouphandlers[a]->GetClass());
	}
	s->SerializeObjectInstance(eoh, eoh->GetClass());
}

static void WriteString(std::ostream& s, const std::string& str)
{
	assert(str.length() < (1 << 16));
	s.write(str.c_str(), str.length() + 1);
}

static void PrintSize(const char* txt, int size)
{
	if (size > (1024 * 1024 * 1024)) {
		LOG("%s %.1f GB", txt, size / (1024.0f * 1024 * 1024));
	} else if (size >  (1024 * 1024)) {
		LOG("%s %.1f MB", txt, size / (1024.0f * 1024));
	} else if (size > 1024) {
		LOG("%s %.1f KB", txt, size / (1024.0f));
	} else {
		LOG("%s %u B",    txt, size);
	}
}
#endif //USING_CREG

static void ReadString(std::istream& s, std::string& str)
{
	char cstr[1 << 16]; // 64kB
	s.getline(cstr, sizeof(cstr), 0);
	str += cstr;
}



void CCregLoadSaveHandler::SaveGame(const std::string& file)
{
#ifdef USING_CREG
	LOG("Saving game");
	try {
		std::ofstream ofs(dataDirsAccess.LocateFile(file, FileQueryFlags::WRITE).c_str(), std::ios::out|std::ios::binary);
		if (ofs.bad() || !ofs.is_open()) {
			throw content_error("Unable to save game to file \"" + file + "\"");
		}

		// write our own header. SavePackage() will add its own
		WriteString(ofs, gameSetup->setupText);
		WriteString(ofs, modName);
		WriteString(ofs, mapName);

		CGameStateCollector gsc = CGameStateCollector();

		// save creg state
		creg::COutputStreamSerializer os;
		os.SavePackage(&ofs, &gsc, gsc.GetClass());
		PrintSize("Game", ofs.tellp());

		// save ai state
		int aistart = ofs.tellp();
		eoh->Save(&ofs);
		PrintSize("AIs", ((int)ofs.tellp()) - aistart);

		//FIXME add lua state
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "Save failed(content error): %s", ex.what());
	} catch (const std::exception& ex) {
		LOG_L(L_ERROR, "Save failed: %s", ex.what());
	} catch (const char*& exStr) {
		LOG_L(L_ERROR, "Save failed: %s", exStr);
	} catch (const std::string& str) {
		LOG_L(L_ERROR, "Save failed: %s", str.c_str());
	} catch (...) {
		LOG_L(L_ERROR, "Save failed(unknown error)");
	}
#else //USING_CREG
	LOG_L(L_ERROR, "Save failed: creg is disabled");
#endif //USING_CREG
}

/// this just loads the mapname and some other early stuff
void CCregLoadSaveHandler::LoadGameStartInfo(const std::string& file)
{
	ifs = new std::ifstream(dataDirsAccess.LocateFile(FindSaveFile(file)), std::ios::in|std::ios::binary);

	// in case these contained values alredy
	// (this is the case when loading a game through the spring menu eg),
	// we set them to empty strings, as ReadString() does append,
	// and we would end up with the correct value but two times
	// eg: "AbcAbc" instead of "Abc"

	//FIXME remove
	scriptText = "";
	modName = "";
	mapName = "";

	// read our own header.
	ReadString(*ifs, scriptText);
	ReadString(*ifs, modName);
	ReadString(*ifs, mapName);

	CGameSetup::LoadSavedScript(file, scriptText);
}

/// this should be called on frame 0 when the game has started
void CCregLoadSaveHandler::LoadGame()
{
#ifdef USING_CREG
	ENTER_SYNCED_CODE();

	void* pGSC = NULL;
	creg::Class* gsccls = NULL;

	// load creg state
	creg::CInputStreamSerializer inputStream;
	inputStream.LoadPackage(ifs, pGSC, gsccls);
	assert(pGSC && gsccls == CGameStateCollector::StaticClass());

	CGameStateCollector* gsc = static_cast<CGameStateCollector*>(pGSC);
	delete gsc; // the only job of gsc is to collect gamestate data
	gsc = NULL;

	// load ai state
	eoh->Load(ifs);
	//for (int a=0; a < teamHandler->ActiveTeams(); a++) { // For old savegames
	//	if (teamHandler->Team(a)->isDead && eoh->IsSkirmishAI(a)) {
	//		eoh->DestroySkirmishAI(skirmishAIId(a), 2 /* = team died */);
	//	}
	//}

	// cleanup
	delete ifs;
	ifs = NULL;

	gs->paused = false;
	if (gameServer) {
		gameServer->isPaused = false;
		gameServer->syncErrorFrame = 0;
	}

	LEAVE_SYNCED_CODE();
#else //USING_CREG
	LOG_L(L_ERROR, "Load failed: creg is disabled");
#endif //USING_CREG
}
