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

#ifdef WIN32
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#include <iostream>
#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
#define __stdcall
#define MB_OK 0
static inline void MessageBox(void*, const char* msg, const char* capt, unsigned int)
{
	std::cerr << "unitsync: " << capt << ": " << msg << std::endl;
}
#endif

#endif
