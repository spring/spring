// ScriptHandler.h: interface for the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPTHANDLER_H__15A7CC17_B675_4CA6_B782_B566E8B009FA__INCLUDED_)
#define AFX_SCRIPTHANDLER_H__15A7CC17_B675_4CA6_B782_B566E8B009FA__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <map>
#include "script.h"
#include "gllist.h"

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

#endif // !defined(AFX_SCRIPTHANDLER_H__15A7CC17_B675_4CA6_B782_B566E8B009FA__INCLUDED_)
