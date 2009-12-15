#ifndef LOADSCRIPT_H
#define LOADSCRIPT_H

#include "Script.h"
#include <string>
#include "LoadSaveHandler.h"

class CLoadScript : public CScript
{
public:
	CLoadScript(const std::string& file);
	~CLoadScript();
	void Update();
	void ScriptSelected();

private:
	bool started;
	CLoadSaveHandler loader;
	std::string file;
};

#endif // LOADSCRIPT_H
