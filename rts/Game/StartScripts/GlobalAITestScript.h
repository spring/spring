#ifndef GLOBALAITESTSCRIPT_H
#define GLOBALAITESTSCRIPT_H

#include "Script.h"
#include "ExternalAI/Interface/SAIInterfaceLibrary.h"

class CGlobalAITestScript :
	public CScript
{
	SSAIKey skirmishAISpecifier;
public:
	CGlobalAITestScript(const SSAIKey& skirmishAISpecifier);
	~CGlobalAITestScript(void);

	void GameStart(void);
};

#endif
