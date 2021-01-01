/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>

#include "AllyTeam.h"
#include "System/StringHash.h"
#include "System/creg/STL_Map.h"

CR_BIND(AllyTeam, )

CR_REG_METADATA(AllyTeam, (
	CR_MEMBER(startRectTop),
	CR_MEMBER(startRectBottom),
	CR_MEMBER(startRectLeft),
	CR_MEMBER(startRectRight),
	CR_MEMBER(allies),
	CR_MEMBER(customValues)
))


void AllyTeam::SetValue(const std::string& key, const std::string& value)
{
	switch (hashString(key.c_str())) {
		case hashString("startrecttop"): {
			startRectTop = std::atof(value.c_str());
		} break;
		case hashString("startrectbottom"): {
			startRectBottom = std::atof(value.c_str());
		} break;
		case hashString("startrectleft"): {
			startRectLeft = std::atof(value.c_str());
		} break;
		case hashString("startrectright"): {
			startRectRight = std::atof(value.c_str());
		} break;
		default: {
			customValues[key] = value;
		} break;
	}
}
