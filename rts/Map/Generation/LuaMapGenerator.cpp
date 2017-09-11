/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaMapGenerator.h"

CLuaMapGenerator::CLuaMapGenerator(const CMapGeneratorInfo& g) :
	CMapGenerator(g.mapName, g.mapSeed),
	genInfo(g)
{
}
