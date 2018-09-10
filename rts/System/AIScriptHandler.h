/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCRIPTHANDLER_H_
#define SCRIPTHANDLER_H_

#include "ExternalAI/SkirmishAIData.h"

class CAIScriptHandler
{
public:
	typedef std::vector< std::pair<std::string, SkirmishAIData> > ScriptMap;

	static CAIScriptHandler& Instance();

	bool IsSkirmishAITestScript(const std::string& scriptName) const;

	const SkirmishAIData& GetSkirmishAIData(const std::string& scriptName) const;
	const ScriptMap& GetScriptMap() const { return scriptMap; }

private:
	CAIScriptHandler();

	ScriptMap scriptMap;
};

#endif /* SCRIPTHANDLER_H_ */
