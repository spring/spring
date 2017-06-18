/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONTEXT_DATA_H
#define LUA_CONTEXT_DATA_H

#include "LuaMemPool.h"
#if (!defined(UNITSYNC) && !defined(DEDICATED))
#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
#include "LuaDisplayLists.h"
#endif
#include "System/EventClient.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"

class CLuaHandle;
class LuaMemPool;
class LuaParser;

struct luaContextData {
public:
	luaContextData()
	: owner(nullptr)
	, luamutex(nullptr)

	, synced(false)
	, allowChanges(false)
	, drawingEnabled(false)

	, running(0)

	, fullCtrl(false)
	, fullRead(false)

	, ctrlTeam(CEventClient::NoAccessTeam)
	, readTeam(0)
	, readAllyTeam(0)
	, selectTeam(CEventClient::NoAccessTeam)

	, memPool(new LuaMemPool())
	, parser(nullptr) {}

	~luaContextData() {
		// raw cast; LuaHandle is not a known type here
		// owner-less LCD's are common and uninteresting
		if (owner != nullptr)
			memPool->LogStats((((CEventClient*) owner)->GetName()).c_str(), synced? "synced": "unsynced");

		delete memPool;
		memPool = nullptr;
	}

	void Clear() {
		#if (!defined(UNITSYNC) && !defined(DEDICATED))
		shaders.Clear();
		textures.Clear();
		fbos.Clear();
		rbos.Clear();
		displayLists.Clear();
		#endif
	}

public:
	CLuaHandle* owner;
	spring::recursive_mutex* luamutex;

	bool synced;
	bool allowChanges;
	bool drawingEnabled;

	int running; //< is currently running? (0: not running; >0: is running)

	// permission rights
	bool fullCtrl;
	bool fullRead;

	int  ctrlTeam;
	int  readTeam;
	int  readAllyTeam;
	int  selectTeam;

#if (!defined(UNITSYNC) && !defined(DEDICATED))
	LuaShaders shaders;
	LuaTextures textures;
	LuaFBOs fbos;
	LuaRBOs rbos;
	CLuaDisplayLists displayLists;

	GLMatrixStateTracker glMatrixTracker;
#endif

	LuaMemPool* memPool;
	LuaParser* parser;
};


#endif // LUA_CONTEXT_DATA_H
