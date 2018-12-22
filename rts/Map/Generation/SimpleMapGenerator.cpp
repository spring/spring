/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "SimpleMapGenerator.h"
#include "System/Log/ILog.h"


CSimpleMapGenerator::CSimpleMapGenerator(const CGameSetup* setup) : CMapGenerator(setup)
{
	GenerateInfo();
}

CSimpleMapGenerator::~CSimpleMapGenerator() = default;

void CSimpleMapGenerator::GenerateInfo()
{
	const auto& mapOpts = setup->GetMapOptionsCont();

	for (const auto& mapOpt: mapOpts) {
		LOG_L(L_WARNING, "[MapGen::%s] mapOpt<%s,%s>", __func__, mapOpt.first.c_str(), mapOpt.second.c_str());
	}
	for (const auto& modOpt: setup->GetModOptionsCont()) {
		LOG_L(L_WARNING, "[MapGen::%s] modOpt<%s,%s>", __func__, modOpt.first.c_str(), modOpt.second.c_str());
	}

	const std::string* newMapXStr = mapOpts.try_get("new_map_x");
	const std::string* newMapZStr = mapOpts.try_get("new_map_z");

	if (newMapXStr == nullptr || newMapZStr == nullptr) {
		mapSize = int2(1, 1);
		return;
	}


	try {
		const int newMapX = std::stoi(*newMapXStr);
		const int newMapZ = std::stoi(*newMapZStr);

		if (newMapX > 0 && newMapZ > 0)
			mapSize = int2(newMapX, newMapZ);

	} catch (...) {
		mapSize = int2(1, 1);
	}
}

void CSimpleMapGenerator::GenerateMap()
{
	startPositions.emplace_back(20, 20);
	startPositions.emplace_back(500, 500);

	mapDescription = "Simple Random Map";

	const int2 gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	std::fill(map.begin(), map.end(), 50.0f);
}
