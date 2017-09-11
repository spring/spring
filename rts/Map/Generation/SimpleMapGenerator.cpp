/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleMapGenerator.h"

CSimpleMapGenerator::CSimpleMapGenerator(const std::string& mapName, int mapSeed) :
	CMapGenerator(mapName, mapSeed)
{
	GenerateInfo();
}

void CSimpleMapGenerator::GenerateInfo()
{
	mapSize = int2(5, 5);
}

void CSimpleMapGenerator::GenerateMap()
{
	startPositions.push_back(int2(20, 20));
	startPositions.push_back(int2(500, 500));

	mapDescription = "The Split Canyon";

	int2 gs = GetGridSize();
	std::vector<float>& map = GetHeightMap();
	for(int x = 0; x < gs.x; x++)
	{
		for(int y = 0; y < gs.y; y++)
		{
			map[y * gs.x + x] = sin(y/10.0) * 30.0f + 100.0f + x / 5.0f;
		}
	}
}
