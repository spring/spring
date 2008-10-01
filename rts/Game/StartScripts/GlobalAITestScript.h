#ifndef GLOBALAITESTSCRIPT_H
#define GLOBALAITESTSCRIPT_H

#include "Script.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"

class CGlobalAITestScript :
	public CScript
{
	SSAIKey skirmishAISpecifyer;
public:
	CGlobalAITestScript(const SSAIKey& skirmishAISpecifyer);
	~CGlobalAITestScript(void);

	void GameStart(void);
};

#endif
