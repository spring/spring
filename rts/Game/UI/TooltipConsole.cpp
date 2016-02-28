/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TooltipConsole.h"
#include "MouseHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/PlayerHandler.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Util.h"


CONFIG(std::string, TooltipGeometry).defaultValue("0.0 0.0 0.41 0.1");
CONFIG(bool, TooltipOutlineFont).defaultValue(true).headlessValue(false);

CTooltipConsole* tooltip = NULL;


CTooltipConsole::CTooltipConsole()
	: enabled(true)
{
	const std::string geo = configHandler->GetString("TooltipGeometry");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f", &x, &y, &w, &h);
	if (vars != 4) {
		x = 0.00f;
		y = 0.00f;
		w = 0.41f;
		h = 0.10f;
	}

	outFont = configHandler->GetBool("TooltipOutlineFont");
}


CTooltipConsole::~CTooltipConsole()
{
}


void CTooltipConsole::Draw()
{
	if (!enabled) {
		return;
	}

	const std::string& s = mouse->GetCurrentTooltip();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!outFont) {
		glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);
		glRectf(x, y, (x + w), (y + h));
	}

	const float fontSize   = (h * globalRendering->viewSizeY) * (smallFont->GetLineHeight() / 5.75f);

	float curX = x + 0.01f;
	float curY = y + h - 0.5f * fontSize * smallFont->GetLineHeight() / globalRendering->viewSizeY;
	glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

	smallFont->Begin();
	smallFont->SetColors(); //default

	if (outFont) {
		smallFont->glPrint(curX, curY, fontSize, FONT_ASCENDER | FONT_OUTLINE | FONT_NORM, s);
	} else {
		smallFont->glPrint(curX, curY, fontSize, FONT_ASCENDER | FONT_NORM, s);
	}

	smallFont->End();
}


bool CTooltipConsole::IsAbove(int x, int y)
{
	if (!enabled) {
		return false;
	}

	const float mx = MouseX(x);
	const float my = MouseY(y);

	return ((mx > (x + 0.01f)) && (mx < (x + w)) &&
	        (my > (y + 0.01f)) && (my < (y + h)));
}


/******************************************************************************/

#define RED       "\xff\xff\x50\x01"
#define BLUE      "\xff\xd3\xdb\xff"
#define GREEN     "\xff\x50\xff\x50"
#define GREY      "\xff\x90\x90\x90"
#define DARKBLUE  "\xff\xc0\xc0\xff"


static void GetDecoyResources(const CUnit* unit,
                              float& mMake, float& mUse,
                              float& eMake, float& eUse)
{
	mMake = mUse = eMake = eUse = 0.0f;
	const UnitDef* rd = unit->unitDef;;
	const UnitDef* ud = rd->decoyDef;
	if (ud == NULL) {
		return;
	}

	mMake += ud->metalMake;
	eMake += ud->energyMake;
	if (ud->tidalGenerator > 0.0f) {
		eMake += (ud->tidalGenerator * mapInfo->map.tidalStrength);
	}

	bool active = ud->activateWhenBuilt;
	if (rd->onoffable && ud->onoffable) {
		active = unit->activated;
	}

	if (active) {
		mMake += ud->makesMetal;
		if (ud->extractsMetal > 0.0f) {
			if (rd->extractsMetal > 0.0f) {
				mMake += unit->metalExtract * (ud->extractsMetal / rd->extractsMetal);
			}
		}
		mUse += ud->metalUpkeep;

		if (ud->windGenerator > 0.0f) {
			if (wind.GetCurrentStrength() > ud->windGenerator) {
				eMake += ud->windGenerator;
			} else {
				eMake += wind.GetCurrentStrength();
			}
		}
		eUse += ud->energyUpkeep;
	}
}


std::string CTooltipConsole::MakeUnitString(const CUnit* unit)
{
	string custom = eventHandler.WorldTooltip(unit, NULL, NULL);
	if (!custom.empty()) {
		return custom;
	}

	std::string s;
	s.reserve(512);

	const bool enemyUnit = (teamHandler->AllyTeam(unit->team) != gu->myAllyTeam) &&
	                       !gu->spectatingFullView;

	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = enemyUnit ? unitDef->decoyDef : NULL;
	const UnitDef* effectiveDef = !enemyUnit ? unitDef : (decoyDef ? decoyDef : unitDef);
	const CTeam* team = NULL;

	// don't show the unit type if it is not known
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
	if (enemyUnit &&
	    !(losStatus & LOS_INLOS) &&
	    ((losStatus & prevMask) != prevMask)) {
		return "Enemy unit";
	}

	// show the player name instead of unit name if it has FBI tag showPlayerName
	if (effectiveDef->showPlayerName) {
		team = teamHandler->Team(unit->team);
		s = team->GetControllerName();
	} else {
		s = unit->tooltip;
		if (decoyDef) {
			s = decoyDef->humanName + " - " + decoyDef->tooltip;
		}
	}

	// don't show the unit health and other info if it has
	// the FBI tag hideDamage and is not on our ally team or
	// is not in LOS
	if (!enemyUnit || (!effectiveDef->hideDamage && (losStatus & LOS_INLOS))) {
		SUnitStats stats;
		stats.AddUnit(unit, enemyUnit);
		s += MakeUnitStatsString(stats);
	}

	if (gs->cheatEnabled) {
		s += IntToString(unit->unitDef->techLevel, DARKBLUE "  [TechLevel %i]");
	}

	s += "\n" + teamHandler->Team(unit->team)->GetControllerName();

	return s;
}


std::string CTooltipConsole::MakeUnitStatsString(const SUnitStats& stats)
{
	string s;
	s.resize(512);
	size_t charsPrinted = 0;

	charsPrinted += SNPRINTF(&s[charsPrinted], 512 - charsPrinted, "\nHealth %.0f/%.0f", stats.health, stats.maxHealth);
	charsPrinted = std::min<size_t>(512, charsPrinted);

	if (stats.harvestMetalMax > 0.0f || stats.harvestEnergyMax > 0.0f) {
		charsPrinted += SNPRINTF(&s[charsPrinted], 512 - charsPrinted,
			"\nExperience %.2f Cost %.0f Range %.0f\n"
			BLUE "Metal: "  GREEN "%.1f" GREY "/" RED "-%.1f"
			GREY " ("  GREEN "%.1f" GREY "/" BLUE "%.1f" GREY ") "
			BLUE "Energy: " GREEN "%.1f" GREY "/" RED "-%.1f"
			GREY " ("  GREEN "%.1f" GREY "/" BLUE "%.1f" GREY ")\x08",
			stats.experience, stats.cost, stats.maxRange,
			stats.metalMake,     stats.metalUse,
			stats.harvestMetal,  stats.harvestMetalMax,
			stats.energyMake,    stats.energyUse,
			stats.harvestEnergy, stats.harvestEnergyMax);
		charsPrinted = std::min<size_t>(512, charsPrinted);
	} else {
		charsPrinted += SNPRINTF(&s[charsPrinted], 512 - charsPrinted,
			"\nExperience %.2f Cost %.0f Range %.0f\n"
			BLUE "Metal: "  GREEN "%.1f" GREY "/" RED "-%.1f "
			BLUE "Energy: " GREEN "%.1f" GREY "/" RED "-%.1f\x08",
			stats.experience, stats.cost, stats.maxRange,
			stats.metalMake,  stats.metalUse,
			stats.energyMake, stats.energyUse);
		charsPrinted = std::min<size_t>(512, charsPrinted);
	}

	s.resize(charsPrinted);
	return s;
}


std::string CTooltipConsole::MakeFeatureString(const CFeature* feature)
{
	string custom = eventHandler.WorldTooltip(NULL, feature, NULL);
	if (!custom.empty()) {
		return custom;
	}

	std::string s = feature->def->description;
	if (s.empty()) {
		s = "Feature";
	}

	const float remainingMetal  = feature->resources.metal;
	const float remainingEnergy = feature->resources.energy;

	const char* metalColor  = (remainingMetal  > 0) ? GREEN : RED;
	const char* energyColor = (remainingEnergy > 0) ? GREEN : RED;

	char tmp[512];
	sprintf(tmp,"\n" BLUE "Metal: %s%.0f  " BLUE "Energy: %s%.0f\x08",
		metalColor,  remainingMetal,
		energyColor, remainingEnergy);

	s += tmp;

	return s;
}


std::string CTooltipConsole::MakeGroundString(const float3& pos)
{
	string custom = eventHandler.WorldTooltip(NULL, NULL, &pos);
	if (!custom.empty()) {
		return custom;
	}

	const int px = pos.x / 16;
	const int pz = pos.z / 16;
	const int typeMapIdx = std::min(mapDims.hmapx * mapDims.hmapy - 1, std::max(0, pz * mapDims.hmapx + px));
	const unsigned char* typeMap = readMap->GetTypeMapSynced();
	const CMapInfo::TerrainType* tt = &mapInfo->terrainTypes[typeMap[typeMapIdx]];

	char tmp[512];
	sprintf(tmp,
		"Pos %.0f %.0f Elevation %.0f\n"
		"Terrain type: %s\n"
		"Speeds T/K/H/S %.2f %.2f %.2f %.2f\n"
		"Hardness %.0f Metal %.1f",
		pos.x, pos.z, pos.y, tt->name.c_str(),
		tt->tankSpeed, tt->kbotSpeed, tt->hoverSpeed, tt->shipSpeed,
		tt->hardness * mapDamage->mapHardness,
		readMap->metalMap->GetMetalAmount(px, pz)
	);
	return tmp;
}


/***********************************************************************/
/***********************************************************************/

SUnitStats::SUnitStats()
: health(0.0f)
, maxHealth(0.0f)
, experience(0.0f)
, cost(0.0f)
, maxRange(0.0f)
, metalMake(0.0f)
, metalUse(0.0f)
, energyMake(0.0f)
, energyUse(0.0f)
, harvestMetal(0.0f)
, harvestMetalMax(0.0f)
, harvestEnergy(0.0f)
, harvestEnergyMax(0.0f)
, count(0)
{}


void SUnitStats::AddUnit(const CUnit* unit, bool enemy)
{
	const UnitDef* decoyDef = enemy ? unit->unitDef->decoyDef : nullptr;

	++count;

	if (!decoyDef) {
		health           += unit->health;
		maxHealth        += unit->maxHealth;
		experience        = (experience * (count - 1) + unit->experience) / count; // average xp
		cost             += unit->cost.metal + (unit->cost.energy / 60.0f);
		maxRange          = std::max(maxRange, unit->maxRange);
		metalMake        += unit->resourcesMake.metal;
		metalUse         += unit->resourcesUse.metal;
		energyMake       += unit->resourcesMake.energy;
		energyUse        += unit->resourcesUse.energy;
		harvestMetal     += unit->harvested.metal;
		harvestMetalMax  += unit->harvestStorage.metal;
		harvestEnergy    += unit->harvested.energy;
		harvestEnergyMax += unit->harvestStorage.energy;
	} else {
		// display adjusted decoy stats
		const float healthScale = (decoyDef->health / unit->unitDef->health);

		float metalMake_, metalUse_, energyMake_, energyUse_;
		GetDecoyResources(unit, metalMake_, metalUse_, energyMake_, energyUse_);

		health           += unit->health * healthScale;
		maxHealth        += unit->maxHealth * healthScale;
		experience        = (experience * (count - 1) + unit->experience) / count;
		cost             += decoyDef->metal + (decoyDef->energy / 60.0f);
		maxRange          = std::max(maxRange, decoyDef->maxWeaponRange);
		metalMake        += metalMake_;
		metalUse         += metalUse_;
		energyMake       += energyMake_;
		energyUse        += energyUse_;
		//harvestMetal     += unit->harvested.metal;
		//harvestMetalMax  += unit->harvestStorage.metal;
		//harvestEnergy    += unit->harvested.energy;
		//harvestEnergyMax += unit->harvestStorage.energy;
	}
}
