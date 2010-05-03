/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "mmgr.h"
#include "LoadSaveHandler.h"
#include "CregLoadSaveHandler.h"
#include "LuaLoadSaveHandler.h"


ILoadSaveHandler* ILoadSaveHandler::Create()
{
	return new CLuaLoadSaveHandler();
}


ILoadSaveHandler::~ILoadSaveHandler()
{
}


std::string ILoadSaveHandler::FindSaveFile(const std::string& name)
{
	std::string name2 = name;
#ifdef _WIN32
	if (name2.find(":\\")==std::string::npos)
		name2 = "Saves\\" + name2;
#else
	if (name2.find("/")==std::string::npos)
		name2 = "Saves/" + name2;
#endif
	return name2;
}
