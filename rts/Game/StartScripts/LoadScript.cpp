#include "StdAfx.h"
#include "LoadScript.h"
#include "LoadSaveHandler.h"
#include "FileSystem/FileHandler.h"
#include "Platform/FileSystem.h"
#include <vector>
#include <iostream>

extern std::string stupidGlobalMapname;

CLoadScript::CLoadScript(std::string file)
	: CScript(std::string("Load ") + filesystem.GetFilename(file)),
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
