
#include "StdAfx.h"
#include "GlobalStuff.h"
#include "ModInfo.h"
#include "TdfParser.h"
#include "Platform/ConfigHandler.h"

CModInfo* modInfo = 0;

CModInfo::CModInfo(const char* modname)
{
	name = modname;

	// determine whether the modder allows the user to use team coloured nanospray
	TdfParser tdfparser("gamedata/particles.tdf");
	tdfparser.GetDef(allowTeamColors, "1", "nanospray\\allow_team_colours");
	if(allowTeamColors) {
		// Load the users preference for team coloured nanospray
		gu->teamNanospray = configHandler.GetInt ("TeamNanoSpray", 0);
	}
}

CModInfo::~CModInfo(){}
