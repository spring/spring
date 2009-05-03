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
	if (key == "StartRectTop")
		startRectTop = std::atof(value.c_str());
	else if (key == "StartRectBottom")
		startRectBottom = std::atof(value.c_str());
	else if (key == "StartRectLeft")
		startRectLeft = std::atof(value.c_str());
	else if (key == "StartRectRight")
		startRectRight = std::atof(value.c_str());
	else
		customValues[key] = value;
}
