// Sky.h: interface for the CSky class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BASIC_SKY_H__
#define __BASIC_SKY_H__

#include "BaseSky.h"

class CBasicSky : public CBaseSky
{
public:
	void CreateCover(int baseX,int baseY,float* buf);
	void InitSun();
	void CreateTransformVectors();
	void CreateRandMatrix(int matrix[32][32],float mod);
	void DrawShafts();
	void ResetCloudShadow(int texunit);
	void SetCloudShadow(int texunit);
	void CreateClouds();
	void Update();
	void DrawSun();
	void Draw();
	float3 GetCoord(int x,int y);
	float3 GetTexCoord(int x,int y);
	CBasicSky();
	virtual ~CBasicSky();

	unsigned int skyTex;
	unsigned int skyDot3Tex;
	unsigned int cloudDot3Tex;
	unsigned int displist;

	unsigned int sunTex;
	unsigned int sunFlareTex;
	unsigned int sunFlareList;

	int randMatrix[16][32][32];

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];

	int lastCloudUpdate;
	bool cloudDown[10];

	unsigned char *cloudThickness;

	float covers[4][32];
	int oldCoverBaseX;
	int oldCoverBaseY;

	float3 sundir1;
	float3 sundir2;

	float3 modSunDir;

	float domeheight;
	float domeWidth;

	float3 GetDirFromTexCoord(float x, float y);
	float GetTexCoordFromDir(float3 dir);

	float sunTexCoordX,sunTexCoordY;

	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;

protected:
	inline unsigned char GetCloudThickness(int x,int y);
};

#endif // __BASIC_SKY_H__
