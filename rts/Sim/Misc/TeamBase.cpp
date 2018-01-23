/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamBase.h"

#include <cstdlib> // atoi
#include <cstring> // memset
#include <sstream>

#include "System/StringUtil.h"
#include "System/StringHash.h"
#include "System/creg/STL_Map.h"


CR_BIND(TeamBase, )
CR_REG_METADATA(TeamBase, (
	CR_MEMBER(leader),
	CR_MEMBER(color),
	CR_MEMBER(teamStartNum),
	CR_MEMBER(teamAllyteam),
	CR_MEMBER(incomeMultiplier),
	CR_MEMBER(side),
	CR_MEMBER(startPos),
	CR_MEMBER(customValues)
))


unsigned char TeamBase::teamDefaultColor[TeamBase::NUM_DEFAULT_TEAM_COLORS][4] =
{
	{  90,  90, 255, 255}, // blue
	{ 200,   0,   0, 255}, // red
	{ 255, 255, 255, 255}, // white
	{  38, 155,  32, 255}, // green
	{   7,  31, 125, 255}, // blue
	{ 150,  10, 180, 255}, // purple
	{ 255, 255,   0, 255}, // yellow
	{  50,  50,  50, 255}, // black
	{ 152, 200, 220, 255}, // ltblue
	{ 171, 171, 131, 255}  // tan
};

TeamBase::TeamBase() :
	leader(-1),
	teamStartNum(-1),
	teamAllyteam(-1),
	incomeMultiplier(1.0f)
{
	std::memset(    color, 255, sizeof(color));
	std::memset(origColor, 255, sizeof(color));
}

void TeamBase::SetValue(const std::string& key, const std::string& value)
{
	switch (hashString(key.c_str())) {
		case hashString("handicap"): {
			// "handicap" is used for backwards compatibility only;
			// the key was renamed to advantage and is now a direct
			// factor rather than a percentage (see SetAdvantage())
			SetAdvantage(std::atof(value.c_str()) / 100.0f);
		} break;

		case hashString("advantage"): {
			SetAdvantage(std::atof(value.c_str()));
		} break;
		case hashString("incomemultiplier"): {
			SetIncomeMultiplier(std::atof(value.c_str()));
		} break;
		case hashString("teamleader"): {
			leader = std::atoi(value.c_str());
		} break;
		case hashString("side"): {
			side = StringToLower(value);
		} break;
		case hashString("allyteam"): {
			teamAllyteam = std::atoi(value.c_str());
		} break;
		case hashString("rgbcolor"): {
			std::istringstream buf(value);
			for (size_t b = 0; b < 3; ++b) {
				float tmp;
				buf >> tmp;

				origColor[b] = (color[b] = tmp * 255);
			}

			origColor[3] = (color[3] = 255);
		} break;
		case hashString("startposx"): {
			if (!value.empty())
				startPos.x = atoi(value.c_str());
		} break;
		case hashString("startposz"): {
			if (!value.empty())
				startPos.z = atoi(value.c_str());
		} break;

		default: {
			customValues[key] = value;
		} break;
	}
}


void TeamBase::SetAdvantage(float advantage) {

	advantage = std::max(0.0f, advantage);

	SetIncomeMultiplier(advantage + 1.0f);
}

void TeamBase::SetIncomeMultiplier(float incomeMultiplier) {
	this->incomeMultiplier = std::max(0.0f, incomeMultiplier);
}
