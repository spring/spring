/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_TOOLTIP_MAP_H
#define UNIT_TOOLTIP_MAP_H

#include "System/UnorderedMap.hpp"

struct UnitToolTipMap {
public:
	void Clear() {
		tooltips.clear();
		tooltips.reserve(256);
	}

	void Set(int id, std::string&& tip) { tooltips[id] = std::move(tip); }
	const std::string& Get(int id) { return tooltips[id]; }

private:
	spring::unordered_map<int, std::string> tooltips;
};

extern UnitToolTipMap unitToolTipMap;

#endif

