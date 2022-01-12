/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SIMPLE_MAP_GENERATOR_H_
#define _SIMPLE_MAP_GENERATOR_H_

#include "MapGenerator.h"

class CSimpleMapGenerator : public CMapGenerator
{
public:
	CSimpleMapGenerator(const CGameSetup* setup);
	virtual ~CSimpleMapGenerator();

private:
	void GenerateInfo();

	virtual void GenerateMap() override;

	virtual const int2& GetMapSize() const override
	{ return mapSize; }

	virtual const std::vector<int2>& GetStartPositions() const override
	{ return startPositions; }

	virtual const std::string& GetMapDescription() const override
	{ return mapDescription; }

	std::vector<int2> startPositions;
	int2 mapSize;
	std::string mapDescription;
};

#endif // _SIMPLE_MAP_GENERATOR_H_
