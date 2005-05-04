#pragma once
#include "script.h"

class CAmphibOpScript :
	public CScript
{
public:
	CAmphibOpScript(void);
	~CAmphibOpScript(void);
	void Update(void);
	std::string GetMapName(void);
};
