/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCRIPTHANDLER_H_
#define SCRIPTHANDLER_H_

#include "ExternalAI/SkirmishAIData.h"

class CScriptHandler
{
public:
	typedef std::vector<std::string> ScriptList;

	static const std::string SKIRMISH_AI_SCRIPT_NAME_PRELUDE;

	static CScriptHandler& Instance();

	const ScriptList& GetScriptList() const { return scriptNames; }

	bool IsSkirmishAITestScript(const std::string& script) const;
	const SkirmishAIData& GetSkirmishAIData(const std::string& script) const;

private:
	struct CScript {
		std::string name;
		CScript(const std::string& n) : name(n) {}
		virtual ~CScript() {}
	};

	struct CSkirmishAIScript : CScript {
		SkirmishAIData aiData;
		CSkirmishAIScript(const SkirmishAIData& aiData)
		: CScript(SKIRMISH_AI_SCRIPT_NAME_PRELUDE + aiData.shortName + " " + aiData.version)
		, aiData(aiData) {}
	};

	typedef std::map<std::string, CScript*> ScriptMap;

	void Add(CScript* script);

	CScriptHandler();
	~CScriptHandler();

	ScriptMap scripts;
	ScriptList scriptNames;
};

#endif /* SCRIPTHANDLER_H_ */
