#ifndef SCRIPTHANDLER_H
#define SCRIPTHANDLER_H
// ScriptHandler.h: interface for the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <map>
#include "Script.h"
#include "glList.h"

using namespace std;

class CScriptHandler  
{
public:
	void AddScript(string name,CScript* s);
	static void SelectScript(std::string s);

	static CScriptHandler* Instance();
	static void UnloadInstance();

	CScript* chosenScript;
	CglList* list;
	string chosenName;
	std::map<std::string,CScript*> scripts;
private:
	CScriptHandler();
	virtual ~CScriptHandler();

	static CScriptHandler* _instance;

};

#endif /* SCRIPTHANDLER_H */
