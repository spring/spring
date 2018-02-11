/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIScriptHandler.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/SkirmishAIKey.h"

#include <stdexcept>

CAIScriptHandler& CAIScriptHandler::Instance()
{
	static CAIScriptHandler instance;
	return instance;
}


bool CAIScriptHandler::IsSkirmishAITestScript(const std::string& scriptName) const
{
	const ScriptMap::const_iterator scriptsIt = scripts.find(scriptName);

	if (scriptsIt == scripts.end())
		return false;

	return (dynamic_cast<const CSkirmishAIScript*>(scriptsIt->second) != NULL);
}


const SkirmishAIData& CAIScriptHandler::GetSkirmishAIData(const std::string& scriptName) const
{
	const ScriptMap::const_iterator scriptsIt = scripts.find(scriptName);

	if (scriptsIt == scripts.end())
		throw std::runtime_error("start-script \"" + scriptName + "\" does not exist");

	const CSkirmishAIScript* aiScript = dynamic_cast<const CSkirmishAIScript*>(scriptsIt->second);

	if (aiScript == nullptr)
		throw std::runtime_error("start-script \"" + scriptName + "\" is not a CSkirmishAIScript");

	return aiScript->aiData;
}


void CAIScriptHandler::Add(CScript* script)
{
	scripts.insert(ScriptMap::value_type(script->name, script));
	scriptNames.push_back(script->name);
}


CAIScriptHandler::CAIScriptHandler()
{

	// add the C interface Skirmish AIs
	// Lua AIs can not be added, as the selection would get invalid when
	// selecting another mod.
	const AILibraryManager::T_skirmishAIKeys& skirmishAIKeys = aiLibManager->GetSkirmishAIKeys();

	AILibraryManager::T_skirmishAIKeys::const_iterator i = skirmishAIKeys.begin();
	AILibraryManager::T_skirmishAIKeys::const_iterator e = skirmishAIKeys.end();

	for (; i != e; ++i) {
		SkirmishAIData aiData;
		aiData.shortName = i->GetShortName();
		aiData.version   = i->GetVersion();
		aiData.isLuaAI   = false;

		Add(new CSkirmishAIScript(aiData));
	}
}


CAIScriptHandler::~CAIScriptHandler()
{
	for (ScriptMap::iterator it = scripts.begin(); it != scripts.end(); ++it) {
		delete it->second;
	}
}
