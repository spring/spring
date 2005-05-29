#ifndef LOADSCRIPT_H
#define LOADSCRIPT_H
/*pragma once removed*/
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


#endif /* LOADSCRIPT_H */
