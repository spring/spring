#ifndef GLOBALAITESTSCRIPT_H
#define GLOBALAITESTSCRIPT_H

#include "Script.h"

class CGlobalAITestScript :
	public CScript
{
	std::string dllName;
	std::string baseDir;
public:
	CGlobalAITestScript(std::string dll, std::string base);
	~CGlobalAITestScript(void);

	void Update(void);
};

#endif
