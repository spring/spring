#ifndef SMFREADMAP_H
#define SMFREADMAP_H

#include "ReadMap.h"
#include <string>

class CSmfReadMap :
	public CReadMap
{
public:
	CSmfReadMap(std::string mapname);
	~CSmfReadMap(void);

	void RecalcTexture(int x1, int x2, int y1, int y2);

	unsigned int shadowTex;

	float3 waterAbsorb;
	float3 waterBaseColor;
	float3 waterMinColor;
	float3 waterSurfaceColor;
	float maxMetal;

	float3 waterPlaneColor;
	bool hasWaterPlane;
	std::string waterTexture;

	std::string detailTexName;

	unsigned char waterHeightColors[1024*4];
protected:
	float3 GetLightValue(int x, int y);
	void ParseSMD(std::string filename);
};

#endif
