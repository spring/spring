
#include "StdAfx.h"
#include "GlobalStuff.h"
#include "ModInfo.h"
#include "TdfParser.h"
#include "Platform/ConfigHandler.h"
#include "FileSystem/ArchiveScanner.h"
#include "LogOutput.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Builder.h"

CModInfo* modInfo = 0;

CModInfo::CModInfo(const char* modname)
{
	filename = modname;

	humanName = archiveScanner->ModArchiveToModName(modname);

	const CArchiveScanner::ModData md = archiveScanner->ModArchiveToModData(modname);

	shortName   = md.shortName;
	version     = md.version;
	mutator     = md.mutator;
	description = md.description;

	// determine whether the modder allows the user to use team coloured nanospray
	try {
		TdfParser parser("gamedata/particles.tdf");
		parser.GetDef(allowTeamColors, "1", "nanospray\\allow_team_colours");
	} catch(content_error) {
		allowTeamColors = true;
	}
	if (allowTeamColors) {
		// Load the users preference for team coloured nanospray
		gu->teamNanospray = !!configHandler.GetInt ("TeamNanoSpray", 0);
	}


	// Get the reclaim options for the mod
	multiReclaim = 0;
	reclaimMethod = 1;
	// See if the mod overrides the defaults:
	try {
		TdfParser parser("gamedata/modrules.tdf");
		multiReclaim  = atoi(parser.SGetValueDef("0", "RECLAIM\\MultiReclaim").c_str());
		reclaimMethod = atoi(parser.SGetValueDef("1", "RECLAIM\\ReclaimMethod").c_str());
	}
	catch(content_error) {
		logOutput << "Mod uses default reclaim rules" << "\n";
		// If the modrules.tdf isnt found
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}

	// Get the fire-at-dead-units options
	fireAtKilled = 0;
	fireAtCrashing = 0;
	// See if the mod overrides them
	try {
		TdfParser parser("gamedata/modrules.tdf");
		fireAtKilled   = atoi(parser.SGetValueDef("0", "FIREATDEAD\\FireAtKilled").c_str());
		fireAtCrashing = atoi(parser.SGetValueDef("0", "FIREATDEAD\\FireAtCrashing").c_str());
	}
	catch(content_error) { // If the modrules.tdf isnt found
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}

	// Get the transportability options for the mod
	transportGround = 1;
	transportHover = 0;
	transportShip = 0;
	transportAir = 0;
	// See if the mod overrides the defaults:
	try {
		TdfParser parser("gamedata/modrules.tdf");
		transportGround = atoi(parser.SGetValueDef("1", "TRANSPORTABILITY\\TransportGround").c_str());
		transportHover  = atoi(parser.SGetValueDef("0", "TRANSPORTABILITY\\TransportHover").c_str());
		transportShip   = atoi(parser.SGetValueDef("0", "TRANSPORTABILITY\\TransportShip").c_str());
		transportAir    = atoi(parser.SGetValueDef("0", "TRANSPORTABILITY\\TransportAir").c_str());
	}
	catch(content_error) {
		logOutput << "Mod uses default transportability rules" << "\n";
		// If the modrules.tdf isnt found
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}

	// See if the mod overrides the defaults:
	try {
		TdfParser parser("gamedata/modrules.tdf");
		bool use2DBuildDists = !!atoi(parser.SGetValueDef("0", "DISTANCE\\BuilderUse2D").c_str());
		CBuilder::Use2DDistances(use2DBuildDists);
	}
	catch(content_error) {
		logOutput << "Mod uses default build distance rules" << "\n";
		// If the modrules.tdf isnt found
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}

	// See if the mod overrides the defaults:
	try {
		TdfParser parser("gamedata/modrules.tdf");
		const float expPowerScale  = atof(parser.SGetValueDef("1.0", "EXPERIENCE\\PowerScale").c_str());
		const float expHealthScale = atof(parser.SGetValueDef("0.7", "EXPERIENCE\\HealthScale").c_str());
		const float expReloadScale = atof(parser.SGetValueDef("0.4", "EXPERIENCE\\HealthScale").c_str());
		CUnit::SetExpPowerScale(expPowerScale);
		CUnit::SetExpHealthScale(expHealthScale);
		CUnit::SetExpReloadScale(expReloadScale);
	}
	catch(content_error) {
		logOutput << "Mod uses default experience rules" << "\n";
		// If the modrules.tdf isnt found
		// We already set the defaults so we should be able to ignore this
		// Other optional mod rules MUST set their defaults...
	}
}

CModInfo::~CModInfo() {}
