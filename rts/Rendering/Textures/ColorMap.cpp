/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <sstream>
#include <cstring>


#include "ColorMap.h"
#include "Bitmap.h"
#include "Bitmap.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Util.h"
#include "System/Exceptions.h"

std::vector<CColorMap*> CColorMap::colorMaps;
std::map<std::string, CColorMap*> CColorMap::colorMapsMap;

CR_BIND(CColorMap,)
CR_REG_METADATA(CColorMap, (
	CR_MEMBER_UN(map),
	CR_MEMBER_UN(xsize),
	CR_MEMBER_UN(nxsize),
	CR_MEMBER_UN(ysize),
	CR_MEMBER_UN(nysize)
))

CColorMap::CColorMap()
	: xsize(0)
	, nxsize(0)
	, ysize(0)
	, nysize(0)
{
}

CColorMap::CColorMap(const std::vector<float>& vec)
{
	if (vec.size() < 8) {
		throw content_error("[ColorMap] too few colors in colormap (need at least two RGBA values)");
	}

	unsigned char* lmap = new unsigned char[vec.size() - (vec.size() % 4)];
	xsize  = (vec.size() - (vec.size() % 4)) / 4;
	ysize  = 1;
	nxsize = xsize - 1;
	nysize = ysize - 1;

	for (int i = 0; i < (xsize * 4); ++i) {
		lmap[i] = (int) (vec[i] * 255);
	}
	LoadMap(lmap, xsize * 4);
	delete[] lmap;
}

CColorMap::CColorMap(const std::string& fileName)
{
	CBitmap bitmap;

	if (!bitmap.Load(fileName)) {
		throw content_error("Could not load texture from file " + fileName);
	}

	if (bitmap.compressed || (bitmap.channels != 4) || (bitmap.xsize < 2)) {
		throw content_error("Unsupported bitmap format in file " + fileName);
	}

	xsize  = bitmap.xsize;
	ysize  = bitmap.ysize;
	nxsize = xsize - 1;
	nysize = ysize - 1;

	LoadMap(&bitmap.mem[0], xsize * ysize * 4);
}

CColorMap::CColorMap(const unsigned char* buf, int num)
{
	xsize  = num / 4;
	ysize  = 1;
	LoadMap(buf, num);
	nxsize = xsize - 1;
	nysize = ysize - 1;
}

CColorMap::~CColorMap()
{
}

void CColorMap::DeleteColormaps()
{
	for (size_t i = 0; i < colorMaps.size(); ++i) {
		delete colorMaps[i];
	}
	colorMaps.clear();
}

void CColorMap::LoadMap(const unsigned char* buf, int num)
{
	map.resize(num);
	std::memcpy(&map[0], buf, num);
}


CColorMap* CColorMap::LoadFromBitmapFile(const std::string& fileName)
{
	const std::string& lowFilename = StringToLower(fileName);

	CColorMap* map = colorMapsMap.find(lowFilename)->second;
	if (colorMapsMap.find(lowFilename) != colorMapsMap.end())
		return map;

	map = new CColorMap(fileName);
	colorMapsMap[lowFilename] = map;
	colorMaps.push_back(map);
	return map;
}

CColorMap* CColorMap::LoadFromFloatVector(const std::vector<float>& vec)
{
	CColorMap* map = new CColorMap(vec);
	colorMaps.push_back(map);
	return map;
}

CColorMap* CColorMap::LoadFromFloatString(const std::string& fString)
{
	std::stringstream stream;
	stream << fString;
	std::vector<float> vec;

	float value;
	while (stream >> value) {
		vec.push_back(value);
	}

	CColorMap* map = CColorMap::LoadFromFloatVector(vec);
	return map;
}

CColorMap* CColorMap::LoadFromDefString(const std::string& dString)
{
	std::stringstream stream;
	std::vector<float> vec;

	stream << dString;
	float value;

	while (stream >> value) {
		vec.push_back(value);
	}

	CColorMap* map = NULL;

	if (vec.empty()) {
		map = CColorMap::LoadFromBitmapFile("bitmaps\\" + dString);
	} else {
		map = CColorMap::LoadFromFloatVector(vec);
	}

	if (map == NULL) {
		throw content_error("[ColorMap::LoadFromDefString] unable to load color-map " + dString);
	}

	return map;
}


CColorMap* CColorMap::Load12f(float r1, float g1, float b1, float a1, float r2, float g2, float b2, float a2, float r3, float g3, float b3, float a3)
{
	std::vector<float> vec;

	vec.push_back(r1);
	vec.push_back(g1);
	vec.push_back(b1);
	vec.push_back(a1);
	vec.push_back(r2);
	vec.push_back(g2);
	vec.push_back(b2);
	vec.push_back(a2);
	vec.push_back(r3);
	vec.push_back(g3);
	vec.push_back(b3);
	vec.push_back(a3);

	return CColorMap::LoadFromFloatVector(vec);
}

unsigned char* CColorMap::GetColor(unsigned char* color, float pos)
{
	if (pos >= 1.0f) {
		*((int*)color) = ((int*)&map[0])[nxsize];
		return color;
	}

	const float fposn = pos * nxsize;
	const int iposn = (int) fposn;
	const float fracn = fposn - iposn;
	const int aa = (int) (fracn * 256);
	const int ia = 256 - aa;

	unsigned char* col1 = (unsigned char*) &map[(iposn    ) * 4];
	unsigned char* col2 = (unsigned char*) &map[(iposn + 1) * 4];

	color[0] = ((col1[0] * ia) + (col2[0] * aa)) >> 8;
	color[1] = ((col1[1] * ia) + (col2[1] * aa)) >> 8;
	color[2] = ((col1[2] * ia) + (col2[2] * aa)) >> 8;
	color[3] = ((col1[3] * ia) + (col2[3] * aa)) >> 8;

	return color;
}
