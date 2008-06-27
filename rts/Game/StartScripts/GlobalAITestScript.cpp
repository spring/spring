#include <algorithm>
#include <cctype>
#include "System/StdAfx.h"
#include "GlobalAITestScript.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "Game/Team.h"
#include "Lua/LuaParser.h"
#include "Map/MapParser.h"
#include "Map/ReadMap.h"
#include "Sim/SideParser.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "System/LogOutput.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/FileSystem.h"
#include "mmgr.h"


extern std::string stupidGlobalMapname;


CGlobalAITestScript::CGlobalAITestScript(std::string dll)
	: CScript(std::string("GlobalAI test (") + filesystem.GetFilename(dll) + std::string(")")),
	dllName(dll)
{
}


CGlobalAITestScript::~CGlobalAITestScript(void)
{
}


void CGlobalAITestScript::GameStart(void)
{
	globalAI->CreateGlobalAI(1, dllName.c_str());

	gs->Team(0)->energy        = 1000;
	gs->Team(0)->energyStorage = 1000;
	gs->Team(0)->metal         = 1000;
	gs->Team(0)->metalStorage  = 1000;

	gs->Team(1)->energy        = 1000;
	gs->Team(1)->energyStorage = 1000;
	gs->Team(1)->metal         = 1000;
	gs->Team(1)->metalStorage  = 1000;

	const std::string startUnit0 = sideParser.GetStartUnit(0, "");
	const std::string startUnit1 = sideParser.GetStartUnit(1, startUnit0);
	// default to side 1, in case mod has only 1 side

	if (startUnit0.length() == 0) {
		throw content_error ("Unable to load a commander for the first side");
	}

	MapParser mapParser(stupidGlobalMapname);
	if (!mapParser.IsValid()) {
		throw content_error("MapParser: " + mapParser.GetErrorLog());
	}
	float3 startPos0(1000.0f, 80.0f, 1000.0f);
	float3 startPos1(1200.0f, 80.0f, 1200.0f);
	mapParser.GetStartPos(0, startPos0);
	mapParser.GetStartPos(1, startPos1);

	unitLoader.LoadUnit(startUnit0, startPos0, 0, false, 0, NULL);
	unitLoader.LoadUnit(startUnit1, startPos1, 1, false, 0, NULL);
}
