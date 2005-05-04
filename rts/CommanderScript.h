#pragma once
#include "script.h"

class CCommanderScript :
	public CScript
{
public:
	CCommanderScript(void);
	virtual ~CCommanderScript(void);
	void Update(void);
};
