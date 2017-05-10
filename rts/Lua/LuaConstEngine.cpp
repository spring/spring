/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaConstEngine.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/GameVersion.h"
#include "System/Platform/Misc.h"

bool LuaConstEngine::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "version"        ,                                    SpringVersion::GetSync()          );
	LuaPushNamedString(L, "versionFull"    , (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetFull()      : "");
	LuaPushNamedString(L, "versionPatchSet", (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetPatchSet()  : "");
	LuaPushNamedString(L, "buildFlags"     , (!CLuaHandle::GetHandleSynced(L))? SpringVersion::GetAdditional(): "");

	#if 0
	LuaPushNamedNumber(L, "nativeWordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::NativeWordSize() * 8: 0); // engine
	LuaPushNamedNumber(L, "systemWordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::SystemWordSize() * 8: 0); // op-sys
	#else
	LuaPushNamedNumber(L, "wordSize", (!CLuaHandle::GetHandleSynced(L))? Platform::NativeWordSize() * 8: 0);
	#endif
	return true;
}

