#ifndef _SKIRMISHAITESTSCRIPT_H
#define _SKIRMISHAITESTSCRIPT_H

#include "Script.h"
#include "ExternalAI/SkirmishAIData.h"

#include <map>
#include <string>

class CSkirmishAITestScript : public CScript
{
	SkirmishAIData aiData;
	static const int player_Id = 0;
	static const int player_teamId = 0;
	static const int skirmishAI_teamId = 1;
public:
	CSkirmishAITestScript(const SkirmishAIData& aiData);
	~CSkirmishAITestScript(void);

	void GameStart(void);
};

#endif // _SKIRMISHAITESTSCRIPT_H
