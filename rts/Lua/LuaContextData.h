/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONTEXT_DATA_H
#define LUA_CONTEXT_DATA_H

#include "Lua/LuaAllocState.h"
#include "Lua/LuaGarbageCollectCtrl.h"
#include "LuaMemPool.h"
#if (!defined(UNITSYNC) && !defined(DEDICATED))
#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"

#include "Rendering/GL/MatrixStateTracker.h"
#endif

#include "System/EventClient.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"

class CLuaHandle;
class LuaMemPool;
class LuaParser;

struct luaContextData {
public:
	luaContextData(bool sharedPool, bool stateOwned)
	: owner(nullptr)
	, luamutex(nullptr)
	, memPool(LuaMemPool::AcquirePtr(sharedPool, stateOwned))
	, parser(nullptr)

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

	, allocState{{0}, {0}, {0}, {0}}
	{}

	~luaContextData() {
		// raw cast; LuaHandle is not a known type here
		// ownerless LCD's are common and uninteresting
		if (owner != nullptr)
			memPool->LogStats((((CEventClient*) owner)->GetName()).c_str(), synced? "synced": "unsynced");

		LuaMemPool::ReleasePtr(memPool, owner);
	}

	luaContextData(const luaContextData& lcd) = delete;
	luaContextData(luaContextData&& lcd) = delete;

	luaContextData& operator = (const luaContextData& lcd) = delete;
	luaContextData& operator = (luaContextData&& lcd) = delete;


	void Clear() {
		#if (!defined(UNITSYNC) && !defined(DEDICATED))
		shaders.Clear();
		textures.Clear();
		fbos.Clear();
		rbos.Clear();
		#endif
	}

public:
	CLuaHandle* owner;
	spring::recursive_mutex* luamutex;

	LuaMemPool* memPool;
	LuaParser* parser;

	bool synced;
	bool allowChanges;
	bool drawingEnabled;

	// greater than 0 if currently running a callin; 0 if not
	int running;

	// permission rights
	bool fullCtrl;
	bool fullRead;

	int ctrlTeam;
	int readTeam;
	int readAllyTeam;
	int selectTeam;

	SLuaAllocState allocState;
	SLuaGarbageCollectCtrl gcCtrl;

#if (!defined(UNITSYNC) && !defined(DEDICATED))
	// NOTE:
	//   engine and unitsync will not agree on sizeof(luaContextData)
	//   compiler is not free to rearrange struct members, so declare
	//   any members used by both *ABOVE* this block (to make casting
	//   safe)
	LuaShaders shaders;
	LuaTextures textures;
	LuaFBOs fbos;
	LuaRBOs rbos;

	GLMatrixStateTracker glMatrixTracker;
#endif
};


#endif // LUA_CONTEXT_DATA_H
