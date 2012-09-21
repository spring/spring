/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCRIPTHANDLER_H_
#define SCRIPTHANDLER_H_

#include "ExternalAI/SkirmishAIData.h"

class CScriptHandler
{
private:
	struct CScript {
		std::string name;

		CScript(const std::string& n): name(n) {}
		virtual ~CScript() {}
	};

	struct CSkirmishAIScript : CScript {
		SkirmishAIData aiData;

		CSkirmishAIScript(const SkirmishAIData& data): CScript("Player vs. AI: " + data.shortName + " " + data.version) {
			aiData = data;
		}
	};

public:
	typedef std::vector<std::string> ScriptList;
	typedef std::map<std::string, CScript*> ScriptMap;

	static CScriptHandler& Instance();

	bool IsSkirmishAITestScript(const std::string& scriptName) const;
	const SkirmishAIData& GetSkirmishAIData(const std::string& scriptName) const;

	const ScriptList& GetScriptList() const { return scriptNames; }

private:
	void Add(CScript* script);

	CScriptHandler();
	~CScriptHandler();

	ScriptMap scripts;
	ScriptList scriptNames;
};

#endif /* SCRIPTHANDLER_H_ */
