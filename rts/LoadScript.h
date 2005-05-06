#pragma once
#include "Script.h"
#include <string>
#include "LoadSaveHandler.h"

class CLoadScript :
	public CScript
{
public:
	CLoadScript(std::string file);
	~CLoadScript(void);
	void Update(void);
	std::string GetMapName(void);

	CLoadSaveHandler loader;
	std::string file;
};

