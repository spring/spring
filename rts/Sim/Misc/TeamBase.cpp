/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamBase.h"

#include <cstdlib>
#include <sstream>

#include "Util.h"
#include "LogOutput.h"


unsigned char TeamBase::teamDefaultColor[10][4] =
{
	{ 90, 90, 255, 255}, //blue
	{ 200, 0, 0, 255}, //red
	{ 255, 255, 255, 255}, //white
	{ 38, 155, 32, 255}, //green
	{ 7, 31, 125, 255}, //blue
	{ 150, 10, 180, 255}, //purple
	{ 255, 255, 0, 255}, //yellow
	{ 50, 50, 50, 255}, //black
	{ 152, 200, 220, 255}, //ltblue
	{ 171, 171, 131, 255} //tan
};

TeamBase::TeamBase() :
leader(-1),
handicap(1),
startPos(-100,-100,-100),
teamStartNum(-1),
teamAllyteam(-1)
{
	color[0] = 255;
	color[1] = 255;
	color[2] = 255;
	color[3] = 255;
}

void TeamBase::SetValue(const std::string& key, const std::string& value)
{
	if (key == "handicap") {
		handicap = std::atof(value.c_str()) / 100 + 1;
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
