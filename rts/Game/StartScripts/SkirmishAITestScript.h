#ifndef _SKIRMISHAITESTSCRIPT_H
#define _SKIRMISHAITESTSCRIPT_H

#include "Script.h"
#include "ExternalAI/SkirmishAIKey.h"

#include <map>
#include <string>

class CSkirmishAITestScript : public CScript
{
	SkirmishAIKey key;
	std::map<std::string, std::string> options;
	static const int player_teamId = 0;
	static const int skirmishAI_teamId = 1;
public:
	CSkirmishAITestScript(const SkirmishAIKey& key,
			const std::map<std::string, std::string>& options = (std::map<std::string, std::string>()));
	~CSkirmishAITestScript(void);

	void GameStart(void);
};

#endif // _SKIRMISHAITESTSCRIPT_H
