
#include "StdAfx.h"
#include "GlobalStuff.h"
#include "ModInfo.h"
#include "Game/GameSetup.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaSyncedRead.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "System/LogOutput.h"
#include "System/Platform/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/ArchiveScanner.h"


CModInfo modInfo;


void CModInfo::Init(const char* modname)
{
	filename = modname;

	humanName = archiveScanner->ModArchiveToModName(modname);

	const CArchiveScanner::ModData md = archiveScanner->ModArchiveToModData(modname);

	shortName   = md.shortName;
	version     = md.version;
	mutator     = md.mutator;
	description = md.description;

	// initialize the parser
	LuaParser parser("gamedata/modrules.lua",
	                 SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	// customize the defs environment
	parser.GetTable("Spring");
	parser.AddFunc("GetModOptions", LuaSyncedRead::GetModOptions);
	parser.EndTable();
	parser.Execute();
	if (!parser.IsValid()) {
		logOutput.Print("Error loading modrules, using defaults");
		logOutput.Print(parser.GetErrorLog());
	}
	const LuaTable root = parser.GetRoot();

	// determine whether the modder allows the user to use team coloured nanospray
	const LuaTable nanosprayTbl = root.SubTable("nanospray");
	allowTeamColors = nanosprayTbl.GetBool("allow_team_colors", true);
	if (allowTeamColors) {
		// Load the users preference for team coloured nanospray
		gu->teamNanospray = !!configHandler.GetInt("TeamNanoSpray", 0);
	}

	// constructions
	const LuaTable constructionTbl = root.SubTable("construction");
	constructionDecay = constructionTbl.GetBool("constructionDecay", true);
	constructionDecayTime = (int)(constructionTbl.GetFloat("constructionDecayTime", 6.66) * 30);
	constructionDecaySpeed = constructionTbl.GetFloat("constructionDecaySpeed", 0.03);

	// reclaim
	const LuaTable reclaimTbl = root.SubTable("reclaim");
	multiReclaim  = reclaimTbl.GetInt("multiReclaim",  0);
	reclaimMethod = reclaimTbl.GetInt("reclaimMethod", 1);

	// fire-at-dead-units
	const LuaTable fireAtDeadTbl = root.SubTable("fireAtDead");
	fireAtKilled   = fireAtDeadTbl.GetBool("fireAtKilled", false);
	fireAtCrashing = fireAtDeadTbl.GetBool("fireAtCrashing", false);

	// transportability
	const LuaTable transportTbl = root.SubTable("transportability");
	transportAir    = transportTbl.GetInt("transportAir",   false);
	transportShip   = transportTbl.GetInt("transportShip",  false);
	transportHover  = transportTbl.GetInt("transportHover", false);
	transportGround = transportTbl.GetInt("transportGround", true);

	// experience
	const LuaTable experienceTbl = root.SubTable("experience");
	CUnit::SetExpMultiplier (experienceTbl.GetFloat("experienceMult", 1.0f));
	CUnit::SetExpPowerScale (experienceTbl.GetFloat("powerScale",  1.0f));		
	CUnit::SetExpHealthScale(experienceTbl.GetFloat("healthScale", 0.7f));		
	CUnit::SetExpReloadScale(experienceTbl.GetFloat("reloadScale", 0.4f));

	// flanking bonus
	const LuaTable flankingBonusTbl = root.SubTable("flankingBonus");
	flankingBonusModeDefault = flankingBonusTbl.GetInt("defaultMode", 1);
	
	// sensors
	const LuaTable sensors = root.SubTable("sensors");
	requireSonarUnderWater = sensors.GetBool("requireSonarUnderWater", true);
	/// LoS
	const LuaTable los = sensors.SubTable("los");
	// losMipLevel is used as index to readmap->mipHeightmap,
	// so the max value is CReadMap::numHeightMipMaps - 1
	losMipLevel = los.GetInt("losMipLevel", 1);
	losMul = los.GetFloat("losMul", 1.0f);
	if ((losMipLevel < 0) || (losMipLevel > 6)) {
		throw content_error("Sensors\\Los\\LosMipLevel out of bounds. "
		                    "The minimum value is 0. The maximum value is 6.");
	}
	// airLosMipLevel doesn't have such restrictions, it's just used in various
	// bitshifts with signed integers
	airMipLevel = los.GetInt("airMipLevel", 2);
	if ((airMipLevel < 0) || (airMipLevel > 30)) {
		throw content_error("Sensors\\Los\\AirLosMipLevel out of bounds. "
		                    "The minimum value is 0. The maximum value is 30.");
	}
	airLosMul = los.GetFloat("airLosMul", 1.0f);
}
