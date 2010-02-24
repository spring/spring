/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ScriptHandler.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SkirmishAIKey.h"

#include <stdexcept>


const std::string CScriptHandler::SKIRMISH_AI_SCRIPT_NAME_PRELUDE = "Skirmish AI: ";


CScriptHandler& CScriptHandler::Instance()
{
	static CScriptHandler instance;
	return instance;
}


bool CScriptHandler::IsSkirmishAITestScript(const std::string& script) const
{
	return (script.substr(0, SKIRMISH_AI_SCRIPT_NAME_PRELUDE.size())
			== SKIRMISH_AI_SCRIPT_NAME_PRELUDE);
}


const SkirmishAIData& CScriptHandler::GetSkirmishAIData(const std::string& script) const
{
	ScriptMap::const_iterator s = scripts.find(script);
	if (s == scripts.end()) {
		throw std::runtime_error("Not a script");
	}

	const CSkirmishAIScript* aiScript = dynamic_cast<const CSkirmishAIScript*> (s->second);
	if (aiScript == NULL) {
		throw std::runtime_error("Not a CSkirmishAIScript");
	}

	return aiScript->aiData;
}


void CScriptHandler::Add(CScript* script)
{
	scripts.insert(ScriptMap::value_type(script->name, script));
	scriptNames.push_back(script->name);
}


CScriptHandler::CScriptHandler()
{
	// default script
	Add(new CScript("Commanders"));

	// add the C interface Skirmish AIs
	IAILibraryManager::T_skirmishAIKeys::const_iterator ai, e;
	const IAILibraryManager::T_skirmishAIKeys& skirmishAIKeys = aiLibManager->GetSkirmishAIKeys();
	for(ai = skirmishAIKeys.begin(), e = skirmishAIKeys.end(); ai != e; ++ai) {
		SkirmishAIData aiData;
		aiData.shortName = ai->GetShortName();
		aiData.version   = ai->GetVersion();
		aiData.isLuaAI   = false;
		Add(new CSkirmishAIScript(aiData));
	}
	// Lua AIs can not be added, as the selection would get invalid when
	// selecting an other mod.
}


CScriptHandler::~CScriptHandler()
{
	for (ScriptMap::iterator it = scripts.begin(); it != scripts.end(); ++it) {
		delete it->second;
	}
}
