#ifndef SCRIPTHANDLER_H
#define SCRIPTHANDLER_H
// ScriptHandler.h: interface for the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <list>
#include "Script.h"

class CScriptHandler
{
public:
	std::list<std::string> ScriptList();
	static void SelectScript(std::string s);

	void AddScript(std::string name,CScript* s);
	void LoadScriptFile(const std::string& file);

	static CScriptHandler& Instance();

	CScript* chosenScript;  ///< Pointer to the selected CScript.
	std::string chosenName; ///< Name of the selected script.
private:
	std::map<std::string,CScript*> scripts; ///< Maps script names to CScript pointers.
	std::list<CScript*> loaded_scripts;     ///< Scripts loaded and owned by CScriptHandler
	CScriptHandler();
	CScriptHandler(CScriptHandler const&);
	CScriptHandler& operator=(CScriptHandler const&);
	void LoadScripts();
	~CScriptHandler();
};

#endif /* SCRIPTHANDLER_H */
