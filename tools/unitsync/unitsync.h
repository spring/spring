#ifndef UNITSYNC_H
#define UNITSYNC_H

#include "resource.h"
#include <string>

#define STRBUF_SIZE 100000

struct StartPos
{
	int x;
	int z;
};

struct MapInfo
{
	char* description;
	int tidalStrength;
	int gravity;
	float maxMetal;
	int extractorRadius;
	int minWind;
	int maxWind;

	// 0.61b1+
	int width;
	int height;
	int posCount;
	StartPos positions[16];		// I'd rather not allocate memory, this should be enough
};

const char *GetStr(std::string str);

#endif
