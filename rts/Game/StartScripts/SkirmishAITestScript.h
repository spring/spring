#ifndef _SKIRMISHAITESTSCRIPT_H
#define _SKIRMISHAITESTSCRIPT_H

#include "Script.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"

#include <map>
#include <string>

class CSkirmishAITestScript : public CScript
{
	SSAIKey key;
	std::map<std::string, std::string> options;
public:
	CSkirmishAITestScript(const SSAIKey& key,
			const std::map<std::string, std::string>& options = (std::map<std::string, std::string>()));
	~CSkirmishAITestScript(void);

	void GameStart(void);
};

#endif	// _SKIRMISHAITESTSCRIPT_H
