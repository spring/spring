#ifndef GLOBALAITESTSCRIPT_H
#define GLOBALAITESTSCRIPT_H

#include "Script.h"

class CGlobalAITestScript :
	public CScript
{
	std::string dllName;
public:
	CGlobalAITestScript(std::string dll);
	~CGlobalAITestScript(void);

	void GameStart(void);
};

#endif
