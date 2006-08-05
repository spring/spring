
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
	try {
		TdfParser tdfparser("gamedata/particles.tdf");
		tdfparser.GetDef(allowTeamColors, "1", "nanospray\\allow_team_colours");
	}
	catch(content_error)
	{
		allowTeamColors = true;
	}
	if(allowTeamColors) {
		// Load the users preference for team coloured nanospray
		gu->teamNanospray = configHandler.GetInt ("TeamNanoSpray", 0);
	}

	// Get the reclaim options for the mod
	multiReclaim = 0;
	reclaimMethod = 1;
	// See if the mod overrides the defaults:
	try
	{
		TdfParser reclaimOptions("gamedata/modrules.tdf");
		multiReclaim = atoi(reclaimOptions.SGetValueDef("0", "RECLAIM\\MultiReclaim").c_str());
		reclaimMethod = atoi(reclaimOptions.SGetValueDef("1", "RECLAIM\\ReclaimMethod").c_str());
	}
	catch(content_error) // If the modrules.tdf isnt found
	{
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}

}

CModInfo::~CModInfo(){}
