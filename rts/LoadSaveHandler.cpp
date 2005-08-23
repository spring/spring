#include "StdAfx.h"
#include "LoadSaveHandler.h"
#include <windows.h>
#include "myGL.h"
#include <fstream>
#include "SaveInterface.h"
#include "LoadInterface.h"
#include "ReadMap.h"
#include "FeatureHandler.h"
#include "UnitHandler.h"
#include <boost/filesystem/path.hpp>

extern std::string stupidGlobalMapname;

CLoadSaveHandler::CLoadSaveHandler(void)
{
}

CLoadSaveHandler::~CLoadSaveHandler(void)
{
}

void CLoadSaveHandler::SaveGame(std::string file)
{
	boost::filesystem::path fn(file);
	std::ofstream ofs(fn.native_file_string().c_str(),std::ios::out|std::ios::binary);
	PrintLoadMsg("Saving game");
	if(ofs.bad() || !ofs.is_open()){
		MessageBox(0,"Couldnt save game to file",file.c_str(),0);
		return;
	}

	CSaveInterface save(&ofs);

	save.lsString(stupidGlobalMapname);

	//load,load2 border
	readmap->LoadSaveMap(&save,false);
	featureHandler->LoadSaveFeatures(&save,false);
	uh->LoadSaveUnits(&save,false);
}

//this just loads the mapname and some other early stuff
void CLoadSaveHandler::LoadGame(std::string file)
{
	boost::filesystem::path fn(file);
	ifs=new ifstream(fn.native_file_string().c_str(),std::ios::in|std::ios::binary);
	load=new CLoadInterface(ifs);

	load->lsString(stupidGlobalMapname);
}

//this should be called on frame 0 when the game has started
void CLoadSaveHandler::LoadGame2(void)
{
	readmap->LoadSaveMap(load,true);
	featureHandler->LoadSaveFeatures(load,true);
	uh->LoadSaveUnits(load,true);
	delete load;
	delete ifs;
}
