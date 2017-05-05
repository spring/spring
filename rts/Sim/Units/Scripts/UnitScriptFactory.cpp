/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitScriptFactory.h"

#include "CobEngine.h"
#include "CobInstance.h"
#include "LuaUnitScript.h"
#include "NullUnitScript.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Misc/SimObjectMemPool.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

static SimObjectMemPool<sizeof(CLuaUnitScript)> memPool;


void CUnitScriptFactory::InitStatic()
{
	memPool.clear();
	memPool.reserve(128);

	static_assert(sizeof(CLuaUnitScript) >= sizeof(CCobInstance   ), "");
	static_assert(sizeof(CLuaUnitScript) >= sizeof(CNullUnitScript), "");
}


CUnitScript* CUnitScriptFactory::CreateScript(CUnit* unit, const UnitDef* udef)
{
	CUnitScript* script = &CNullUnitScript::value;
	CCobFile* file = cobFileHandler->GetCobFile(udef->scriptName);

	// NOTE: Lua scripts are not loaded here, see LuaUnitScript::CreateScript
	if (file != nullptr)
		return (CreateCOBScript(unit, file));

	LOG_L(L_WARNING, "[UnitScriptFactory::%s] could not load COB script \"%s\" for unit \"%s\"", __func__, udef->scriptName.c_str(), udef->name.c_str());
	return script;
}


CUnitScript* CUnitScriptFactory::CreateCOBScript(CUnit* unit, CCobFile* F)
{
	return (memPool.alloc<CCobInstance>(F, unit));
}

CUnitScript* CUnitScriptFactory::CreateLuaScript(CUnit* unit, lua_State* L)
{
	return (memPool.alloc<CLuaUnitScript>(L, unit));
}


void CUnitScriptFactory::FreeScript(CUnitScript*& script)
{
	assert(script != &CNullUnitScript::value);
	memPool.free(script);
	script = nullptr;
}

