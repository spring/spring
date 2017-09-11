/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LUA_MAP_GENERATOR_H_
#define _LUA_MAP_GENERATOR_H_

#include "MapGenerator.h"
#include "MapGeneratorInfo.h"

class CLuaMapGenerator : public CMapGenerator
{
public:
	CLuaMapGenerator(const CMapGeneratorInfo& g);

private:
	virtual void GenerateMap() {}

	virtual const int2& GetMapSize() const
	{ return genInfo.mapSize; }

	virtual const std::vector<int2>& GetStartPositions() const
	{ return genInfo.startPositions; }

	virtual const std::string& GetMapDescription() const
	{ return genInfo.mapDescription; }

	CMapGeneratorInfo genInfo;
};

#endif // _LUA_MAP_GENERATOR_H_
