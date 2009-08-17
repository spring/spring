#include "TeamBase.h"

#include <cstdlib>
#include <sstream>

#include "Util.h"
#include "LogOutput.h"
TeamBase::TeamBase() :
leader(-1),
handicap(1),
startPos(100,100,100),
teamAllyteam(-1),
startMetal(-1.0),
startEnergy(-1.0),
readyToStart(false)
{
}

void TeamBase::SetValue(const std::string& key, const std::string& value)
{
	if (key == "handicap")
		handicap = std::atof(value.c_str()) / 100 + 1;
	else if (key == "teamleader")
		leader = std::atoi(value.c_str());
	else if (key == "side")
		side = StringToLower(value);
	else if (key == "allyteam")
		teamAllyteam = std::atoi(value.c_str());
	else if (key == "startmetal")
		startMetal = std::atof(value.c_str());
	else if (key == "startenergy")
		startEnergy = std::atof(value.c_str());
	else if (key == "rgbcolor")
	{
		std::istringstream buf(value);
		for (size_t b = 0; b < 3; ++b)
		{
			float tmp;
			buf >> tmp;
			color[b] = tmp * 255;
		}
		color[3] = 255;
	}
	else
		customValues[key] = value;
}
