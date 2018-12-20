/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleMapGenerator.h"

// #include <string>

// #include "System/Log/ILog.h"

CSimpleMapGenerator::CSimpleMapGenerator(const CGameSetup* setup) : CMapGenerator(setup)
{
	GenerateInfo();
}

CSimpleMapGenerator::~CSimpleMapGenerator() = default;

void CSimpleMapGenerator::GenerateInfo()
{
// The below block of code doesn't work because mapOpts/modOpts are not parsed at this stage?
// Even though it seems like they are?
// .. in any case, this is important and necessary for any serious (Lua-based) map generator
/*

	const auto& mapOpts = setup->GetMapOptions();
	for (const auto& mapOpt : mapOpts) {
		LOG_L(L_WARNING, "%s:%s", mapOpt.first.c_str(), mapOpt.second.c_str());
	}
	for (const auto& modOpt : setup->GetModOptions()) {
		LOG_L(L_WARNING, "%s:%s", modOpt.first.c_str(), modOpt.second.c_str());
	}
	const std::string* newMapXStr = mapOpts.try_get("new_map_x");
	const std::string* newMapZStr = mapOpts.try_get("new_map_z");
	if (newMapXStr == nullptr || newMapZStr == nullptr) {
		mapSize = int2(5, 5);
		return;
	}

	try {
		const int newMapX = std::stoi(*newMapXStr);
		const int newMapZ = std::stoi(*newMapZStr);
		if (newMapX > 0 && newMapZ > 0) {
			mapSize = int2(newMapX, newMapZ);
		}
	} catch (...) {
		mapSize = int2(5, 5);
	}
*/

	// Viola: our hack. We extract map sizeX and sizeZ from mapSeed (assuming it has it packed)
	// This is done mainly as a temporary measure, so we can test it out without touching more files
	int mapSeed = setup->mapSeed;
	const int newMapX = mapSeed / 1000;
	const int newMapZ = mapSeed % 1000;
	// The limit here is at least 1, but I had it crash on small map sizes (<4)
	// there's probably some memory corruption going on (I got 'corrupted size vs. prev_size)
	if (newMapX < 1 || newMapZ < 1) {
		mapSize = int2(5, 5);
	} else {
		mapSize = int2(newMapX, newMapZ);
	}
}

void CSimpleMapGenerator::GenerateMap()
{
	startPositions.emplace_back(20, 20);
	startPositions.emplace_back(500, 500);

	mapDescription = "The Split Canyon";

	int2 gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	for(int x = 0; x < gs.x; x++)
	{
		for(int y = 0; y < gs.y; y++)
		{
			map[y * gs.x + x] = 50.0f;
		}
	}
}
