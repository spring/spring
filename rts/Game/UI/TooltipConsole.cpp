#include "StdAfx.h"
#include "TooltipConsole.h"
#include "MouseHandler.h"
#include "OutlineFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "Game/Player.h"
#include "Game/Team.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureDef.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Platform/ConfigHandler.h"
#include "mmgr.h"


CTooltipConsole* tooltip = 0;


CTooltipConsole::CTooltipConsole(void) : disabled(false)
{
	const std::string geo = configHandler.GetString("TooltipGeometry",
	                                                "0.0 0.0 0.41 0.1");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f", &x, &y, &w, &h);
	if (vars != 4) {
		x = 0.00f;
		y = 0.00f;
		w = 0.41f;
		h = 0.10f;
	}

	outFont = !!configHandler.GetInt("TooltipOutlineFont", 0);
}


CTooltipConsole::~CTooltipConsole(void)
{
}


// FIXME -- duplication
static string StripColorCodes(const string& text)
{
	std::string nocolor;
	const int len = (int)text.size();
	for (int i = 0; i < len; i++) {
		if ((unsigned char)text[i] == 255) {
			i = i + 3;
		} else {
			nocolor += text[i];
		}
	}
	return nocolor;
}


void CTooltipConsole::Draw(void)
{
	if (disabled) {
		return;
	}
	
	const std::string s = mouse->GetCurrentTooltip();

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (!outFont) {
		glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);
		glRectf(x, y, (x + w), (y + h));
	}

	const float yScale = 0.015f;
	const float xScale = (yScale / gu->aspectRatio) * 1.2f;
	const float xPixel  = 1.0f / (xScale * (float)gu->viewSizeX);
	const float yPixel  = 1.0f / (yScale * (float)gu->viewSizeY);
	
	glTranslatef(x + 0.01f, y + 0.08f, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

	glEnable(GL_TEXTURE_2D);

	unsigned int p = 0;
	while (p < s.size()) {
		std::string s2;
		int pos = 0;
		for (int a = 0; a < 420; ++a) {
			s2 += s[p];
			if ((s[p++] == '\n') || (p >= s.size())) {
				break;
			}
		}
		if (!outFont) {
			font->glPrintColor("%s", s2.c_str());
		} else {
			const float color[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
			outlineFont.print(xPixel, yPixel, color, StripColorCodes(s2).c_str());
			font->glPrintColor("%s", s2.c_str());
		}
		glTranslatef(0.0f, -1.2f, 0.0f);
	}

	glPopMatrix();
}


bool CTooltipConsole::IsAbove(int x,int y)
{
	if (disabled) {
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
		eMake += (ud->tidalGenerator * readmap->tidalStrength);
	}

	bool active;
	if (rd->onoffable && ud->onoffable) {
		active = unit->activated;
	} else {
		active = ud->activateWhenBuilt;
	}
	
	if (active) {
		if (ud->isMetalMaker) {
			mMake += (ud->makesMetal * uh->metalMakerEfficiency);
		} else {
			mMake += ud->makesMetal;
		}
		if (ud->extractsMetal > 0.0f) {
			if (rd->extractsMetal > 0.0f) {
				mMake += unit->metalExtract * (ud->extractsMetal / rd->extractsMetal);
			}
		}
		mUse += ud->metalUpkeep;
		
		if (ud->windGenerator > 0.0f) {
			if (wind.curStrength > ud->windGenerator) {
				eMake += ud->windGenerator;
			} else {
				eMake += wind.curStrength;
			}
		}
		eUse += ud->energyUpkeep;
	}
}


std::string CTooltipConsole::MakeUnitString(const CUnit* unit)
{
	std::string s;
	
	const bool enemyUnit = (gs->AllyTeam(unit->team) != gu->myAllyTeam) &&
	                       !gu->spectatingFullView;

	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = enemyUnit ? unitDef->decoyDef : NULL;
	const UnitDef* effectiveDef =
		!enemyUnit ? unitDef : (decoyDef ? decoyDef : unitDef);
	
	// don't show the tooltip if it's a radar dot
	if (enemyUnit && !loshandler->InLos(unit, gu->myAllyTeam)) {
		return "Enemy unit";
	}

	// show the player name instead of unit name if it has FBI tag showPlayerName
	if (effectiveDef->showPlayerName) {
		s = gs->players[gs->Team(unit->team)->leader]->playerName.c_str();
	} else {
		if (!decoyDef) {
			s = unit->tooltip;
		} else {
			s = decoyDef->humanName + " " + decoyDef->tooltip;
		}
	}

	// don't show the unit health and other info if it has
	// the FBI tag hideDamage and is not on our ally team
	if (!enemyUnit || !effectiveDef->hideDamage) {
		if (!decoyDef) {
			const float cost = unit->metalCost + (unit->energyCost / 60.0f);
			s += MakeUnitStatsString(
						 unit->health, unit->maxHealth,
						 unit->currentFuel, unitDef->maxFuel,
						 unit->experience, cost, unit->maxRange,
						 unit->metalMake,  unit->metalUse,
						 unit->energyMake, unit->energyUse);
		} else {
			// display adjusted decoy stats
			const float cost = decoyDef->metalCost + (decoyDef->energyCost / 60.0f);
			const float healthScale = (decoyDef->health / unitDef->health);
			float fuelScale;
			if (unitDef->maxFuel > 0.0f) {
				fuelScale = (decoyDef->maxFuel / unitDef->maxFuel);
			} else {
				fuelScale = 0.0f;
			}

			// get the adjusted resource stats
			float metalMake, energyMake, metalUse, energyUse;
			GetDecoyResources(unit, metalMake, metalUse, energyMake, energyUse);
			
			s += MakeUnitStatsString(
						 unit->health * healthScale, unit->maxHealth * healthScale,
						 unit->currentFuel * fuelScale, decoyDef->maxFuel,
						 unit->experience, cost, decoyDef->maxWeaponRange,
						 metalMake,  metalUse,
						 energyMake, energyUse);
		}
	}

	if (gs->cheatEnabled) {
		char buf[32];
		SNPRINTF(buf, 32, DARKBLUE "  [TechLevel %i]", unit->unitDef->techLevel);
		s += buf;
	}
	
	return s;
}


std::string CTooltipConsole::MakeUnitStatsString(
	float health, float maxHealth,
	float currentFuel, float maxFuel,
	float experience, float cost, float maxRange,
	float metalMake,  float metalUse,
	float energyMake, float energyUse)
{
	string s;

	char tmp[512];
	sprintf(tmp,"\nHealth %.0f/%.0f", health, maxHealth);
	s += tmp;

	if (maxFuel > 0.0f) {
		sprintf(tmp," Fuel %.0f/%.0f", currentFuel, maxFuel);
		s += tmp;
	}

	sprintf(tmp, "\nExperience %.2f Cost %.0f Range %.0f\n"
							 BLUE "Metal: "  GREEN "%.1f" GREY "/" RED "-%.1f "
							 BLUE "Energy: " GREEN "%.1f" GREY "/" RED "-%.1f",
					experience, cost, maxRange,
					metalMake,  metalUse,
					energyMake, energyUse);
	s += tmp;

	return s;
}


std::string CTooltipConsole::MakeFeatureString(const CFeature* feature)
{
	std::string s;
	
	if (feature->def->description == "") {
		s = "Feature";
	} else {
		s = feature->def->description;
	}

	const float remainingMetal  = feature->RemainingMetal();
	const float remainingEnergy = feature->RemainingEnergy();

	const std::string metalColor  = (remainingMetal  > 0) ? GREEN : RED;
	const std::string energyColor = (remainingEnergy > 0) ? GREEN : RED;
	
	char tmp[512];
	sprintf(tmp,"\n" BLUE "Metal: %s%.0f  " BLUE "Energy: %s%.0f",
	        metalColor.c_str(),  remainingMetal,
          energyColor.c_str(), remainingEnergy);

	s += tmp;

	return s;
}


std::string CTooltipConsole::MakeGroundString(const float3& pos)
{
	char tmp[512];
	CReadMap::TerrainType* tt = &readmap->terrainTypes[readmap->typemap[min(gs->hmapx*gs->hmapy-1,max(0,((int)pos.z/16)*gs->hmapx+((int)pos.x/16)))]];
	string ttype = tt->name;
	sprintf(tmp, "Pos %.0f %.0f Elevation %.0f\nTerrain type: %s\n"
	             "Speeds T/K/H/S %.2f %.2f %.2f %.2f\nHardness %.0f Metal %.1f",
	        pos.x, pos.z, pos.y, ttype.c_str(),
	        tt->tankSpeed, tt->kbotSpeed, tt->hoverSpeed, tt->shipSpeed,
	        tt->hardness * mapDamage->mapHardness,
	        readmap->metalMap->getMetalAmount((int)(pos.x/16), (int)(pos.z/16)));
	return tmp;
}
