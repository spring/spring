#ifndef LOADSCRIPT_H
#define LOADSCRIPT_H

#include "Script.h"
#include <string>
#include "System/LoadSaveHandler.h"

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
