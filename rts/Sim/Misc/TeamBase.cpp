/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamBase.h"

#include <cstdlib>
#include <sstream>

#include "System/Util.h"
#include "System/creg/STL_Map.h"


CR_BIND(TeamBase, );
CR_REG_METADATA(TeamBase, (
	CR_MEMBER(leader),
	CR_MEMBER(color),
	CR_MEMBER(incomeMultiplier),
	CR_MEMBER(side),
	CR_MEMBER(startPos),
	CR_MEMBER(teamStartNum),
	CR_MEMBER(teamAllyteam),
	CR_MEMBER(customValues)
));


unsigned char TeamBase::teamDefaultColor[10][4] =
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
	incomeMultiplier(1.0f),
	startPos(-100.0f, -100.0f, -100.0f),
	teamStartNum(-1),
	teamAllyteam(-1)
{
	color[0] = 255;
	color[1] = 255;
	color[2] = 255;
	color[3] = 255;
}

TeamBase::~TeamBase() {
}


void TeamBase::SetValue(const std::string& key, const std::string& value)
{
	if (key == "handicap") {
		// handicap is used for backwards compatibility only
		// it was renamed to advantage and is now a direct factor, not % anymore
		// see SetAdvantage()
		SetAdvantage(std::atof(value.c_str()) / 100.0f);
	}
	else if (key == "advantage") {
		SetAdvantage(std::atof(value.c_str()));
	}
	else if (key == "incomemultiplier") {
		SetIncomeMultiplier(std::atof(value.c_str()));
	}
	else if (key == "teamleader") {
		leader = std::atoi(value.c_str());
	}
	else if (key == "side") {
		side = StringToLower(value);
	}
	else if (key == "allyteam") {
		teamAllyteam = std::atoi(value.c_str());
	}
	else if (key == "rgbcolor") {
		std::istringstream buf(value);
		for (size_t b = 0; b < 3; ++b) {
			float tmp;
			buf >> tmp;
			color[b] = tmp * 255;
		}
		color[3] = 255;
	}
	else if (key == "startposx") {
		if (!value.empty())
			startPos.x = atoi(value.c_str());
	}
	else if (key == "startposz") {
		if (!value.empty())
			startPos.z = atoi(value.c_str());
	}
	else {
		customValues[key] = value;
	}
}


void TeamBase::SetAdvantage(float advantage) {

	advantage = std::max(0.0f, advantage);

	SetIncomeMultiplier(advantage + 1.0f);
}

void TeamBase::SetIncomeMultiplier(float incomeMultiplier) {
	this->incomeMultiplier = std::max(0.0f, incomeMultiplier);
}
