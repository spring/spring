#include "StdAfx.h"
#include "LoadScript.h"
#include "LoadSaveHandler.h"
#include "FileHandler.h"
#include <vector>
#include <iostream>

extern std::string stupidGlobalMapname;

class CLoadScriptHandler
{
public:
	std::vector<CLoadScript*> scripts;

	CLoadScriptHandler()
	{
		std::vector<std::string> f=CFileHandler::FindFiles("*.ssf");

		for(std::vector<std::string>::iterator fi=f.begin();fi!=f.end();++fi){
			scripts.push_back(new CLoadScript(*fi));
		}
	};
	~CLoadScriptHandler()
	{
		for(std::vector<CLoadScript*>::iterator fi=scripts.begin();fi!=scripts.end();++fi){
			delete *fi;
		}
	};
};

static CLoadScriptHandler lsh;


CLoadScript::CLoadScript(std::string file)
: CScript(std::string("Load ")+file),
	file(file)
{
}

CLoadScript::~CLoadScript(void)
{
}

void CLoadScript::Update(void)
{
	if(gs->frameNum==0){
		loader.LoadGame2();
	}
}

std::string CLoadScript::GetMapName(void)
{
	loader.LoadGame(file);		//this is the first time we get called after getting choosen
	loadGame=true;
	return stupidGlobalMapname;
}
