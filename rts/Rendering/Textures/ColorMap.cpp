#include "StdAfx.h"
#include "ColorMap.h"
#include "Bitmap.h"
#include "Rendering/Textures/Bitmap.h"
#include "FileSystem/FileHandler.h"
#include <sstream>

CColorMap::CColorMap(void)
{
	map = 0;
}

CColorMap::CColorMap(std::vector<float> &vec)
{
	map=0;
	LoadFromFloatVector(vec);
}

CColorMap::CColorMap(std::string filename)
{
	map=0;
	LoadFromBitmapFile(filename);
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
	if(map)
		delete [] map;
}

void CColorMap::LoadMap(const unsigned char *buf, int num)
{
	if(map)
		delete [] map;

	map = new unsigned char[num];
	memcpy(map, buf, num);
}

void CColorMap::LoadFromBitmapFile(std::string filename)
{
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
	int aa = fracn*256;
	int ia = 256-aa;

	unsigned char *col1 = (unsigned char*)&map[iposn*4];
	unsigned char *col2 = (unsigned char*)&map[(iposn+1)*4];

	color[0] = (col1[0]*ia + col2[0]*aa)>>8;
	color[1] = (col1[1]*ia + col2[1]*aa)>>8;
	color[2] = (col1[2]*ia + col2[2]*aa)>>8;
	color[3] = (col1[3]*ia + col2[3]*aa)>>8;

	return color;
}

void CColorMap::LoadFromFloatVector(std::vector<float> &vec)
{
	if(vec.size()<8) //needs at least two colors
		throw content_error("To few colors in colormap.");

	unsigned char *lmap = new unsigned char[vec.size() - vec.size()%4];
	xsize = (vec.size() - vec.size()%4)/4;
	ysize = 1;
	nxsize = xsize-1;
	nysize = ysize-1;

	for(int i=0; i<xsize*4; i++)
	{
		lmap[i] = vec[i]*255;
	}
	LoadMap(lmap, xsize*4);
	delete [] lmap;
}

void CColorMap::LoadFromFloatString(std::string fstring)
{
	std::stringstream stream;
	stream << fstring;
	std::vector<float> vec;

	float value;
	while(stream >> value)
	{
		vec.push_back(value);
	}

	LoadFromFloatVector(vec);
}

void CColorMap::Load8f(float r1,float g1,float b1,float a1,float r2,float g2,float b2,float a2)
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
	LoadFromFloatVector(vec);
}

void CColorMap::Load12f(float r1,float g1,float b1,float a1,float r2,float g2,float b2,float a2,float r3,float g3,float b3,float a3)
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
	LoadFromFloatVector(vec);
}