/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAP_GENERATOR_INFO_H_
#define _MAP_GENERATOR_INFO_H_

struct CMapGeneratorInfo {
	std::vector<int2> startPositions;
	int2 mapSize;
	std::string mapDescription;
	std::string mapName;
	int mapSeed;
};


#endif
