/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <sstream>
#include <zlib.h>

#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "CregLoadSaveHandler.h"
#include "Map/ReadMap.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Game/GlobalUnsynced.h"
#include "Game/WaitCommandsAI.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Lua/LuaGaia.h"
#include "Lua/LuaRules.h"
#include "Net/GameServer.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/BuildingMaskMap.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "Sim/Units/Scripts/CobEngine.h"
#include "Sim/Units/Scripts/UnitScriptEngine.h"
#include "Sim/Units/Scripts/NullUnitScript.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "System/SafeUtil.h"
#include "System/Platform/errorhandler.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/FileSystem/GZFileHandler.h"
#include "System/Threading/ThreadPool.h"
#include "System/creg/SerializeLuaState.h"
#include "System/creg/Serializer.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"



CCregLoadSaveHandler::CCregLoadSaveHandler()
{}

CCregLoadSaveHandler::~CCregLoadSaveHandler() = default;

#ifdef USING_CREG
class CGameStateCollector
{
	CR_DECLARE_STRUCT(CGameStateCollector)

public:
	CGameStateCollector() = default;

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
	s->SerializeObjectInstance(&quadField, quadField.GetClass());
	s->SerializeObjectInstance(&unitHandler, unitHandler.GetClass());
	s->SerializeObjectInstance(cobEngine, cobEngine->GetClass());
	s->SerializeObjectInstance(unitScriptEngine, unitScriptEngine->GetClass());
	s->SerializeObjectInstance(&CNullUnitScript::value, CNullUnitScript::value.GetClass());
	s->SerializeObjectInstance(&featureHandler, featureHandler.GetClass());
	s->SerializeObjectInstance(losHandler, losHandler->GetClass());
	s->SerializeObjectInstance(&interceptHandler, interceptHandler.GetClass());
	s->SerializeObjectInstance(CCategoryHandler::Instance(), CCategoryHandler::Instance()->GetClass());
	s->SerializeObjectInstance(&buildingMaskMap, buildingMaskMap.GetClass());
	s->SerializeObjectInstance(&projectileHandler, projectileHandler.GetClass());
	CPlasmaRepulser::SerializeShieldSegmentCollectionPool(s);
	s->SerializeObjectInstance(&waitCommandsAI, waitCommandsAI.GetClass());
	s->SerializeObjectInstance(&envResHandler, envResHandler.GetClass());
	s->SerializeObjectInstance(&moveDefHandler, moveDefHandler.GetClass());
	s->SerializeObjectInstance(&teamHandler, teamHandler.GetClass());
	for (int a = 0; a < teamHandler.ActiveTeams(); a++) {
		s->SerializeObjectInstance(&uiGroupHandlers[a], uiGroupHandlers[a].GetClass());
	}
	s->SerializeObjectInstance(&commandDescriptionCache, commandDescriptionCache.GetClass());
	s->SerializeObjectInstance(eoh, eoh->GetClass());
}


class CLuaStateCollector
{
	CR_DECLARE_STRUCT(CLuaStateCollector)

public:
	CLuaStateCollector() = default;
	bool valid;
	lua_State* L = nullptr;
	lua_State* L_GC = nullptr;
	void Serialize(creg::ISerializer* s);
};

CR_BIND(CLuaStateCollector, )
CR_REG_METADATA(CLuaStateCollector, (
	CR_MEMBER(valid),
	CR_IGNORED(L),
	CR_IGNORED(L_GC),
	CR_SERIALIZER(Serialize)
))

void CLuaStateCollector::Serialize(creg::ISerializer* s) {
	if (!valid)
		return;

	creg::SerializeLuaState(s, &L);
	creg::SerializeLuaThread(s, &L_GC);
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


static void SaveLuaState(CSplitLuaHandle* handle, creg::COutputStreamSerializer& os, std::stringstream& oss)
{
	CLuaStateCollector lsc;
	lsc.valid = (handle != nullptr) && handle->syncedLuaHandle.IsValid();
	if (lsc.valid) {
		lsc.L = handle->syncedLuaHandle.GetLuaState();
		lsc.L_GC = handle->syncedLuaHandle.GetLuaGCState();
		lua_gc(lsc.L_GC, LUA_GCCOLLECT, 0);
	}
	os.SavePackage(&oss, &lsc, lsc.GetClass());
}


static void LoadLuaState(CSplitLuaHandle* handle, creg::CInputStreamSerializer& is, std::stringstream& iss)
{
	void* plsc;
	creg::Class* plsccls = nullptr;

	if ((handle != nullptr) && handle->syncedLuaHandle.IsValid())
		creg::CopyLuaContext(handle->syncedLuaHandle.GetLuaState());

	is.LoadPackage(&iss, plsc, plsccls);
	assert(plsc && plsccls == CLuaStateCollector::StaticClass());
	CLuaStateCollector* lsc = static_cast<CLuaStateCollector*>(plsc);

	if ((handle != nullptr) && handle->syncedLuaHandle.IsValid() && lsc->valid)
		handle->SwapSyncedHandle(lsc->L, lsc->L_GC);

	spring::SafeDelete(lsc);
	return;

}


void CCregLoadSaveHandler::SaveGame(const std::string& path)
{
#ifdef USING_CREG
	LOG("[LSH::%s] saving game to \"%s\"", __func__, path.c_str());

	try {
		std::stringstream oss;

		// write our own header. SavePackage() will add its own
		WriteString(oss, SpringVersion::GetSync());
		WriteString(oss, gameSetup->setupText);
		WriteString(oss, modName);
		WriteString(oss, mapName);


		{
			creg::COutputStreamSerializer os;

			// save lua state first as lua unit scripts depend on it
			const int luaStart = oss.tellp();
			SaveLuaState(luaGaia, os, oss);
			SaveLuaState(luaRules, os, oss);
			PrintSize("Lua", ((int)oss.tellp()) - luaStart);

			// save creg state
			const int gameStart = oss.tellp();
			CGameStateCollector gsc;
			os.SavePackage(&oss, &gsc, gsc.GetClass());
			PrintSize("Game", ((int)oss.tellp()) - gameStart);


			// save AI state
			const int aiStart = oss.tellp();

			for (const auto& ai: skirmishAIHandler.GetAllSkirmishAIs()) {
				std::stringstream aiData;
				eoh->Save(&aiData, ai.first);

				std::streamsize aiSize = aiData.tellp();
				os.SerializeInt(&aiSize, sizeof(aiSize));
				if (aiSize > 0)
					oss << aiData.rdbuf();
			}
			PrintSize("AIs", ((int)oss.tellp()) - aiStart);
		}

		{
			gzFile file = gzopen(dataDirsAccess.LocateFile(path, FileQueryFlags::WRITE).c_str(), "wb5");

			if (file == nullptr) {
				LOG_L(L_ERROR, "[LSH::%s] could not open save-file", __func__);
				return;
			}

			std::string data = std::move(oss.str());
			std::function<void(gzFile, std::string&&)> func = [](gzFile file, std::string&& data) {
				gzwrite(file, data.c_str(), data.size());
				gzflush(file, Z_FINISH);
				gzclose(file);
			};

			// gzFile is just a plain typedef (struct gzFile_s {}* gzFile), can be copied
			// need to keep a reference to the future around or its destructor will block
			ThreadPool::AddExtJob(std::move(std::async(std::launch::async, std::move(func), file, std::move(data))));
		}

		//FIXME add lua state
	} catch (const content_error& ex) {
		LOG_L(L_ERROR, "[LSH::%s] content error \"%s\"", __func__, ex.what());
	} catch (const std::exception& ex) {
		LOG_L(L_ERROR, "[LSH::%s] exception \"%s\"", __func__, ex.what());
	} catch (const char*& exStr) {
		LOG_L(L_ERROR, "[LSH::%s] cstr error \"%s\"", __func__, exStr);
	} catch (const std::string& str) {
		LOG_L(L_ERROR, "[LSH::%s] str error \"%s\"", __func__, str.c_str());
	} catch (...) {
		LOG_L(L_ERROR, "[LSH::%s] unknown error", __func__);
	}
#else //USING_CREG
	LOG_L(L_ERROR, "[LSH::%s] creg is disabled", __func__);
#endif //USING_CREG
}

/// this just loads the mapname and some other early stuff
void CCregLoadSaveHandler::LoadGameStartInfo(const std::string& path)
{
	CGZFileHandler saveFile(dataDirsAccess.LocateFile(FindSaveFile(path)), SPRING_VFS_RAW_FIRST);
	std::stringbuf *sbuf = iss.rdbuf();
	char buf[4096];
	int len;
	while ((len = saveFile.Read(buf, sizeof(buf))) > 0)
		sbuf->sputn(buf, len);

	//Check for compatible save versions
	std::string saveVersion;
	ReadString(iss, saveVersion);
	if (saveVersion != SpringVersion::GetSync()) {
		if (SpringVersion::IsRelease()) {
			throw content_error("File was saved by an incompatible engine version: " + saveVersion);
		} else {
			LOG_L(L_WARNING, "File was saved by a different engine version: %s", saveVersion.c_str());
		}
	}


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
	ReadString(iss, scriptText);
	ReadString(iss, modName);
	ReadString(iss, mapName);

	CGameSetup::LoadSavedScript(path, scriptText);
}

/// this should be called on frame 0 when the game has started
void CCregLoadSaveHandler::LoadGame()
{
#ifdef USING_CREG
	ENTER_SYNCED_CODE();
	{
		creg::CInputStreamSerializer inputStream;

		// load lua state first, as lua unit scripts depend on it
		LoadLuaState(luaGaia, inputStream, iss);
		LoadLuaState(luaRules, inputStream, iss);

		// load creg state
		void* pGSC = nullptr;
		creg::Class* gsccls = nullptr;

		inputStream.LoadPackage(&iss, pGSC, gsccls);
		assert(pGSC && gsccls == CGameStateCollector::StaticClass());

		// the only job of gsc is to collect gamestate data
		CGameStateCollector* gsc = static_cast<CGameStateCollector*>(pGSC);
		spring::SafeDelete(gsc);

		// load ai state
		for (const auto& ai: skirmishAIHandler.GetAllSkirmishAIs()) {
			std::streamsize aiSize;
			inputStream.SerializeInt(&aiSize, sizeof(aiSize));

			std::vector<char> buffer(aiSize);
			std::stringstream aiData;
			iss.read(buffer.data(), buffer.size());
			aiData.write(buffer.data(), buffer.size());

			eoh->Load(&aiData, ai.first);
		}
	}

	// cleanup
	iss.str("");

	gs->paused = false;
	if (gameServer != nullptr) {
		gameServer->isPaused = false;
		gameServer->syncErrorFrame = 0;
	}

	LEAVE_SYNCED_CODE();
#else //USING_CREG
	LOG_L(L_ERROR, "Load failed: creg is disabled");
#endif //USING_CREG
}
