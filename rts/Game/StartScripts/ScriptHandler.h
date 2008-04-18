#ifndef SCRIPTHANDLER_H
#define SCRIPTHANDLER_H
// ScriptHandler.h: interface for the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <list>
#include "Script.h"
#include "Rendering/GL/glList.h"

#ifndef NO_LUA
#include "Script/LuaBinder.h"
#endif

class CScriptHandler
{
public:
	static void SelectScript(std::string s);
	CglList* GenList(ListSelectCallback callb);

	void AddScript(std::string name,CScript* s);

	static CScriptHandler& Instance();

	CScript* chosenScript;  ///< Pointer to the selected CScript.
	std::string chosenName; ///< Name of the selected script.
private:
#ifndef NO_LUA
	std::list<CLuaBinder*> lua_binders;
#endif
	void StartLua();
	std::map<std::string,CScript*> scripts; ///< Maps script names to CScript pointers.
	std::list<CScript*> loaded_scripts;     ///< Scripts loaded and owned by CScriptHandler
	static ListSelectCallback callback;
	CScriptHandler();
	CScriptHandler(CScriptHandler const&);
	CScriptHandler& operator=(CScriptHandler const&);
	void LoadScripts();
	virtual ~CScriptHandler();
};

#endif /* SCRIPTHANDLER_H */
