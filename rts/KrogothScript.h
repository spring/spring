#pragma once
#include "Script.h"

class CKrogothScript :
	public CScript
{
public:
	CKrogothScript(void);
	~CKrogothScript(void);
	void Update(void);
	std::string GetMapName(void);
};

