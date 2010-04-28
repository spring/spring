/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AllyTeam.h"

#include <cstdlib>

AllyTeam::AllyTeam() :
startRectTop(0),
startRectBottom(1),
startRectLeft(0),
startRectRight(1)
{
}

void AllyTeam::SetValue(const std::string& key, const std::string& value)
{
	if (key == "startrecttop")
		startRectTop = std::atof(value.c_str());
	else if (key == "startrectbottom")
		startRectBottom = std::atof(value.c_str());
	else if (key == "startrectleft")
		startRectLeft = std::atof(value.c_str());
	else if (key == "startrectright")
		startRectRight = std::atof(value.c_str());
	else
		customValues[key] = value;
}
