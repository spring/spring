/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <sstream>
#include <cstring>

#include "ColorMap.h"
#include "Bitmap.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Util.h"
#include "System/Exceptions.h"

std::deque<CColorMap> CColorMap::colorMaps;
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
	: map(2, SColor(128, 128, 128))
	, xsize(2)
	, nxsize(1)
	, ysize(1)
	, nysize(0)
{
}

CColorMap::CColorMap(const std::vector<float>& vec)
{
	if (vec.size() < 8) {
		throw content_error("[ColorMap] too few colors in colormap (need at least two RGBA values)");
	}

	xsize  = (vec.size() - (vec.size() % 4)) / 4;
	ysize  = 1;
	nxsize = xsize - 1;
	nysize = ysize - 1;

	SColor* cmap = new SColor[xsize];
	for (int i = 0; i < xsize; ++i) {
		cmap[i] = SColor(&vec[i * 4]);
	}
	LoadMap(&cmap[0].r, xsize);
	delete[] cmap;
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

	LoadMap(&bitmap.mem[0], xsize * ysize);
}


void CColorMap::LoadMap(const unsigned char* buf, int num)
{
	map.resize(num);
	std::memcpy(&map[0], buf, num * 4);
}


CColorMap* CColorMap::LoadFromBitmapFile(const std::string& fileName)
{
	const std::string& lowFilename = StringToLower(fileName);

	CColorMap* map = colorMapsMap.find(lowFilename)->second;
	if (colorMapsMap.find(lowFilename) != colorMapsMap.end())
		return map;

	colorMaps.emplace_back(fileName);
	colorMapsMap[lowFilename] = &colorMaps.back();
	return &colorMaps.back();
}

CColorMap* CColorMap::LoadFromFloatVector(const std::vector<float>& vec)
{
	colorMaps.emplace_back(vec);
	return &colorMaps.back();
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


void CColorMap::GetColor(unsigned char* color, float pos)
{
	if (pos >= 1.0f) {
		*reinterpret_cast<SColor*>(color) = map.back();
		return;
	}

	const float fposn = pos * nxsize;
	const int iposn = (int) fposn;
	const float fracn = fposn - iposn;
	const int aa = (int) (fracn * 256);
	const int ia = 256 - aa;

	unsigned char* col1 = (unsigned char*) &map[iposn    ];
	unsigned char* col2 = (unsigned char*) &map[iposn + 1];

	color[0] = ((col1[0] * ia) + (col2[0] * aa)) >> 8;
	color[1] = ((col1[1] * ia) + (col2[1] * aa)) >> 8;
	color[2] = ((col1[2] * ia) + (col2[2] * aa)) >> 8;
	color[3] = ((col1[3] * ia) + (col2[3] * aa)) >> 8;
}
