/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIScriptHandler.h"

#include "ExternalAI/Interface/aidefines.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/SkirmishAIKey.h"

#include <algorithm>
#include <stdexcept>

CAIScriptHandler& CAIScriptHandler::Instance()
{
	static CAIScriptHandler instance;
	return instance;
}


bool CAIScriptHandler::IsSkirmishAITestScript(const std::string& scriptName) const
{
	using P = decltype(scriptMap)::value_type;

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(scriptMap.begin(), scriptMap.end(), P{scriptName, SkirmishAIData{}}, pred);

	return (iter != scriptMap.end() && iter->first == scriptName);
}


const SkirmishAIData& CAIScriptHandler::GetSkirmishAIData(const std::string& scriptName) const
{
	using P = decltype(scriptMap)::value_type;

	const auto pred = [](const P& a, const P& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(scriptMap.begin(), scriptMap.end(), P{scriptName, SkirmishAIData{}}, pred);

	if (iter == scriptMap.end() || iter->first != scriptName)
		throw std::runtime_error("start-script \"" + scriptName + "\" does not exist");

	return (iter->second);
}


CAIScriptHandler::CAIScriptHandler()
{
	// add the C interface Skirmish AI's
	// Lua AI's can not be added since they would
	// get invalidated when selecting another mod
	const AILibraryManager::T_skirmishAIKeys& skirmishAIKeys = aiLibManager->GetSkirmishAIKeys();

	scriptMap.clear();
	scriptMap.reserve(skirmishAIKeys.size());

	for (const auto& aiKey: skirmishAIKeys) {
		SkirmishAIData aiData;

		aiData.shortName = aiKey.GetShortName();
		aiData.version   = aiKey.GetVersion();
		aiData.isLuaAI   = false;

		scriptMap.emplace_back(std::move("Player vs. AI: " + aiData.shortName + " " + aiData.version), std::move(aiData));
	}

	std::sort(scriptMap.begin(), scriptMap.end(), [](const decltype(scriptMap)::value_type& a, const decltype(scriptMap)::value_type& b) { return (a.first < b.first); });
}

