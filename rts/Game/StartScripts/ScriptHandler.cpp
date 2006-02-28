// ScriptHandler.cpp: implementation of the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ScriptHandler.h"
#include "Game/Game.h"
#include "mmgr.h"
#include "FileSystem/FileHandler.h"
#include "LoadScript.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CScriptHandler::SelectScript(std::string s)
{
	CScriptHandler::Instance().chosenScript=CScriptHandler::Instance().scripts[s];
	CScriptHandler::Instance().chosenName=s;
}

CScriptHandler::CScriptHandler()
{
  
	chosenScript=0;
	list=new CglList("Select script",SelectScript);
}

void CScriptHandler::LoadFiles() {
  std::vector<std::string> f=CFileHandler::FindFiles("*.ssf");

  for(std::vector<std::string>::iterator fi=f.begin(), e = f.end();fi!=e;++fi){
    file_scripts.push_back(new CLoadScript(*fi));
  }
}

CScriptHandler& CScriptHandler::Instance()
{
  static bool created = false;
  static CScriptHandler instance;
  if( !created ) {
    created = true;
    instance.LoadFiles();
  }
  return instance;
}


CScriptHandler::~CScriptHandler()
{
  while(!file_scripts.empty()) {
    delete file_scripts.back();
    file_scripts.pop_back();
  }
	delete list;
}

void CScriptHandler::AddScript(string name, CScript *s)
{
	scripts[name]=s;
	list->AddItem(name.c_str(),"");
}
