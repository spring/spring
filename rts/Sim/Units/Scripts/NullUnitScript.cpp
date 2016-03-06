/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NullUnitScript.h"
#include "System/Log/ILog.h"


// keep one global copy so we don't need to allocate a lot of
// near empty objects for mods that use Lua unit scripts.
CNullUnitScript CNullUnitScript::value;

CR_BIND_DERIVED(CNullUnitScript, CUnitScript, )

CR_REG_METADATA(CNullUnitScript, )

void CNullUnitScript::ShowScriptError(const std::string& msg)
{
	LOG_L(L_ERROR, "%s", msg.c_str());
	LOG_L(L_ERROR, "why are you using CNullUnitScript anyway?");
}

