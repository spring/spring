
#include "StdAfx.h"
#include "GlobalStuff.h"
#include "ModInfo.h"
#include "TdfParser.h"
#include "Platform/ConfigHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "LogOutput.h"

CModInfo* modInfo = 0;

CModInfo::CModInfo(const char* modname)
{
	name = modname;
	humanName = archiveScanner->ModArchiveToModName(modname);

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
		gu->teamNanospray = !!configHandler.GetInt ("TeamNanoSpray", 0);
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
	//Get the transportability options for the mod
	transportGround = 1;
	transportHover = 0;
	transportShip = 0;
	transportAir = 0;
	// See if the mod overrides the defaults:
	try
	{
		TdfParser transportOptions("gamedata/modrules.tdf");
		transportGround = atoi(transportOptions.SGetValueDef("1", "TRANSPORTABILITY\\TransportGround").c_str());
		transportHover = atoi(transportOptions.SGetValueDef("0", "TRANSPORTABILITY\\TransportHover").c_str());
		transportShip = atoi(transportOptions.SGetValueDef("0", "TRANSPORTABILITY\\TransportShip").c_str());
		transportAir = atoi(transportOptions.SGetValueDef("0", "TRANSPORTABILITY\\TransportAir").c_str());
		logOutput << "TransportHover: " << transportHover << "\n";
	}
	catch(content_error) // If the modrules.tdf isnt found
	{
		logOutput << "Content error:" << "\n";
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}

}

CModInfo::~CModInfo(){}
