#include "StdAfx.h"
#include "LoadSaveHandler.h"
#include "Rendering/GL/myGL.h"
#include "SaveInterface.h"
#include "LoadInterface.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Platform/errorhandler.h"
#include "Platform/FileSystem.h"
#include "mmgr.h"

extern std::string stupidGlobalMapname;

CLoadSaveHandler::CLoadSaveHandler(void): ifs(NULL), load(NULL)
{
}

CLoadSaveHandler::~CLoadSaveHandler(void)
{
	delete ifs;
	delete load;
}

void CLoadSaveHandler::SaveGame(std::string file)
{
	std::ofstream ofs(filesystem.LocateFile(file, FileSystem::WRITE).c_str(), std::ios::out|std::ios::binary);

	PrintLoadMsg("Saving game");
	if (ofs.bad() || !ofs.is_open()) {
		handleerror(0,"Couldnt save game to file",file.c_str(),0);
		return;
	}

	CSaveInterface save(&ofs);

	save.lsString(mapName);
	save.lsString(modName);

	//load,load2 border
	readmap->LoadSaveMap(&save,false);
	featureHandler->LoadSaveFeatures(&save,false);
	uh->LoadSaveUnits(&save,false);
}

//this just loads the mapname and some other early stuff
void CLoadSaveHandler::LoadGame(std::string file)
{
	ifs=SAFE_NEW std::ifstream(filesystem.LocateFile(file).c_str(), std::ios::in|std::ios::binary);
	load=SAFE_NEW CLoadInterface(ifs);

	load->lsString(mapName);
	load->lsString(modName);
}

//this should be called on frame 0 when the game has started
void CLoadSaveHandler::LoadGame2(void)
{
	readmap->LoadSaveMap(load,true);
	featureHandler->LoadSaveFeatures(load,true);
	uh->LoadSaveUnits(load,true);
	delete load;
	delete ifs;
	load = NULL;
	ifs = NULL;
}
