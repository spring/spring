// ScriptHandler.cpp: implementation of the CScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ScriptHandler.h"
#include "Game/Game.h"
#include "mmgr.h"
#include "FileSystem/FileHandler.h"
#include "LoadScript.h"
#include "CommanderScript.h"
#include "CommanderScript2.h"
#include "AirScript.h"
#include "GlobalAITestScript.h"
#include "SpawnScript.h"
#include "EmptyScript.h"
#include "TestScript.h"


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

void CScriptHandler::LoadScripts() {

  loaded_scripts.push_back( new CCommanderScript() );
  loaded_scripts.push_back( new CCommanderScript2() );
  loaded_scripts.push_back( new CAirScript() );
  loaded_scripts.push_back( new CEmptyScript() );
  loaded_scripts.push_back( new CSpawnScript() );
  loaded_scripts.push_back( new CTestScript() );

#ifdef WIN32
  std::vector<std::string> f=CFileHandler::FindFiles("aidll\\globalai\\*.dll");
#elif defined(__APPLE__)
  std::vector<std::string> f=CFileHandler::FindFiles("aidll/globalai/*.dylib");
#else
  std::vector<std::string> f=CFileHandler::FindFiles("aidll/globalai/*.so");
#endif
  for(std::vector<std::string>::iterator fi=f.begin(), e = f.end();fi!=e;++fi){
    string name = fi->substr(fi->find_last_of('\\') + 1);
    loaded_scripts.push_back(new CGlobalAITestScript(name, "./"));
  }


  f=CFileHandler::FindFiles("*.ssf");
  for(std::vector<std::string>::iterator fi=f.begin(), e = f.end();fi!=e;++fi){
    loaded_scripts.push_back(new CLoadScript(*fi));
  }
}

CScriptHandler& CScriptHandler::Instance()
{
  static bool created = false;
  static CScriptHandler instance;
  if( !created ) {
    created = true;
    instance.LoadScripts();
  }
  return instance;
}


CScriptHandler::~CScriptHandler()
{
  while(!loaded_scripts.empty()) {
    delete loaded_scripts.back();
    loaded_scripts.pop_back();
  }
	delete list;
}

void CScriptHandler::AddScript(string name, CScript *s)
{
	scripts[name]=s;
	list->AddItem(name.c_str(),"");
}
