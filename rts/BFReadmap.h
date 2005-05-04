// BFReadMap.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BF_READ_MAP_H__
#define __BF_READ_MAP_H__

#include "archdef.h"

#include "ReadMap.h"
#include <string>

class CBFReadmap :
	public CReadMap
{
public:
	CBFReadmap(std::string mapname);
	~CBFReadmap(void);

	void RecalcTexture(int x1, int x2, int y1, int y2);

	unsigned int shadowTex;

	float3 waterAbsorb;
	float3 waterBaseColor;
	float3 waterMinColor;
	float maxMetal;

	unsigned char waterHeightColors[1024*4];
protected:
	float3 GetLightValue(int x, int y);
	void ParseSMD(std::string filename);
};

#endif __BF_READ_MAP_H__
