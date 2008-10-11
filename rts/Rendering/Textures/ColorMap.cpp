#include "StdAfx.h"
#include <sstream>
#include "mmgr.h"

#include "ColorMap.h"
#include "Bitmap.h"
#include "Rendering/Textures/Bitmap.h"
#include "FileSystem/FileHandler.h"
#include "System/Util.h"
#include "System/Exceptions.h"

std::vector<CColorMap *> CColorMap::colorMaps;
std::map<std::string, CColorMap *> CColorMap::colorMapsMap;

CR_BIND(CColorMap, );

CColorMap::CColorMap(void)
{
	map = 0;
}

CColorMap::CColorMap(std::vector<float> &vec)
{
	map=0;

	if(vec.size()<8) //needs at least two colors
		throw content_error("Too few colors in colormap.");

	unsigned char *lmap = new unsigned char[vec.size() - vec.size()%4];
	xsize = (vec.size() - vec.size()%4)/4;
	ysize = 1;
	nxsize = xsize-1;
	nysize = ysize-1;

	for(int i=0; i<xsize*4; i++)
	{
		lmap[i] = int(vec[i]*255);
	}
	LoadMap(lmap, xsize*4);
	delete [] lmap;
}

CColorMap::CColorMap(std::string filename)
{
	map=0;

	CBitmap bitmap;

	if (!bitmap.Load(filename))
		throw content_error("Could not load texture from file " + filename);

	if(bitmap.type != CBitmap::BitmapTypeStandardRGBA || (bitmap.xsize<2))
		throw content_error("Unsupported bitmap format in file " + filename);

	xsize = bitmap.xsize;
	ysize = bitmap.ysize;
	nxsize = xsize-1;
	nysize = ysize-1;

	LoadMap(bitmap.mem, xsize*ysize*4);
}

CColorMap::CColorMap(const unsigned char *buf, int num)
{
	map=0;
	xsize = num/4;
	ysize = 1;
	LoadMap(buf, num);
	nxsize = xsize-1;
	nysize = ysize-1;
}

CColorMap::~CColorMap(void)
{
	delete [] map;
}

void CColorMap::DeleteColormaps()
{
	for(int i=0; i<colorMaps.size(); i++)
	{
		delete colorMaps[i];
	}
	colorMaps.clear();
}

void CColorMap::LoadMap(const unsigned char *buf, int num)
{
	delete [] map;

	map = new unsigned char[num];
	memcpy(map, buf, num);
}

CColorMap* CColorMap::LoadFromBitmapFile(std::string filename)
{
	std::string lowfilename = filename;
	StringToLowerInPlace(lowfilename);
	CColorMap *map;
	if(colorMapsMap.find(lowfilename)==colorMapsMap.end())
	{
		map = new CColorMap(filename);
		colorMapsMap[lowfilename] = map;
		colorMaps.push_back(map);
	}
	else
	{
		map = colorMapsMap.find(lowfilename)->second;
	}
	return map;
}

CColorMap* CColorMap::LoadFromFloatVector(std::vector<float> &vec)
{
	CColorMap *map = new CColorMap(vec);
	colorMaps.push_back(map);
	return map;
}

CColorMap* CColorMap::LoadFromFloatString(std::string fstring)
{
	std::stringstream stream;
	stream << fstring;
	std::vector<float> vec;

	float value;
	while(stream >> value)
	{
		vec.push_back(value);
	}

	return CColorMap::LoadFromFloatVector(vec);
}

CColorMap* CColorMap::LoadFromDefString(std::string dstring)
{
	std::stringstream stream;
	stream << dstring;
	std::vector<float> vec;

	float value;
	while(stream >> value)
	{
		vec.push_back(value);
	}

	if(vec.size()>0)
		return CColorMap::LoadFromFloatVector(vec);
	else
		return CColorMap::LoadFromBitmapFile("bitmaps\\" + dstring);
}

CColorMap* CColorMap::Load8f(float r1,float g1,float b1,float a1,float r2,float g2,float b2,float a2)
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
	return CColorMap::LoadFromFloatVector(vec);
}

CColorMap* CColorMap::Load12f(float r1,float g1,float b1,float a1,float r2,float g2,float b2,float a2,float r3,float g3,float b3,float a3)
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

unsigned char* CColorMap::GetColor(unsigned char *color, float pos)
{
	if(pos>=1.0f)
	{
		*((int*)color)=(((int*)map)[nxsize]);
		return color;
	}

	float fposn = pos*nxsize;
	int iposn = (int)fposn;
	float fracn = fposn-iposn;
	int aa = int(fracn*256);
	int ia = 256-aa;

	unsigned char *col1 = (unsigned char*)&map[iposn*4];
	unsigned char *col2 = (unsigned char*)&map[(iposn+1)*4];

	color[0] = (col1[0]*ia + col2[0]*aa)>>8;
	color[1] = (col1[1]*ia + col2[1]*aa)>>8;
	color[2] = (col1[2]*ia + col2[2]*aa)>>8;
	color[3] = (col1[3]*ia + col2[3]*aa)>>8;

	return color;
}
