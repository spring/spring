// ScriptHandler.cpp: implementation of the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ScriptHandler.h"
#include "Game/Game.h"
#include "mmgr.h"

extern CGame* game;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CScriptHandler* CScriptHandler::_instance=0;

void CScriptHandler::SelectScript(std::string s)
{
	CScriptHandler::Instance()->chosenScript=CScriptHandler::Instance()->scripts[s];
	CScriptHandler::Instance()->chosenName=s;
}

CScriptHandler::CScriptHandler()
{
	chosenScript=0;
	list=new CglList("Select script",SelectScript);
}

CScriptHandler* CScriptHandler::Instance()
{
	if(_instance==0)
		_instance=new CScriptHandler;
	return _instance;
}

void CScriptHandler::UnloadInstance()
{
	if(_instance!=0){
		delete _instance;
		_instance=0;
	}
}

CScriptHandler::~CScriptHandler()
{
	delete list;
}

void CScriptHandler::AddScript(string name, CScript *s)
{
	scripts[name]=s;
	list->AddItem(name.c_str(),"");
}
