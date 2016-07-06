/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitScriptFactory.h"

#include "CobEngine.h"
#include "CobInstance.h"
#include "NullUnitScript.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"


/******************************************************************************/
/******************************************************************************/


CUnitScript* CUnitScriptFactory::CreateScript(const std::string& name, CUnit* unit)
{
	std::string ext = FileSystem::GetExtension(name);
	CUnitScript* script = NULL;

	if (ext == "cob") {
		CCobFile* file = cobFileHandler->GetCobFile(name);
		if (file) {
			script = new CCobInstance(file, unit);
		} else {
			LOG_L(L_WARNING, "Could not load COB script for unit \"%s\" from: %s", unit->unitDef->name.c_str(), name.c_str());
		}
	}

	if (!script) {
		script = &CNullUnitScript::value;
	}

	return script;
}


/******************************************************************************/
/******************************************************************************/
