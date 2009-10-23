#include "StdAfx.h"
#include <iostream>
#include <vector>

#include "mmgr.h"

#include "LoadScript.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/Team.h"
#include "Sim/Units/UnitDefHandler.h"
#include "LoadSaveHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/FileSystem.h"


CLoadScript::CLoadScript(const std::string& file):
	CScript(std::string("Load ") + filesystem.GetFilename(file)),
	file(file)
{
}

CLoadScript::~CLoadScript()
{
}

void CLoadScript::Update()
{
	if (!started) {
		loader.LoadGame();
		started = true;
	}
}

void CLoadScript::ScriptSelected()
{
	// this is the first time we get called after getting choosen
	loader.LoadGameStartInfo(file);
	started = false;
	loadGame = true;
}
