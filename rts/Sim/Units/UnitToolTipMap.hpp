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

	void Set(int id, const std::string& tip) { tooltips[id] = tip; }
	const std::string& Get(int id) const {
		const auto it = tooltips.find(id);

		// unreachable
		if (it == tooltips.end())
			return dummy;

		return (it->second);
	}

private:
	spring::unordered_map<int, std::string> tooltips;
	std::string dummy;
};

extern UnitToolTipMap unitToolTipMap;

#endif

