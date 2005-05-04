// Sky.h: interface for the CSky class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADVSKY_H__9A754BA1_AB87_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_ADVSKY_H__9A754BA1_AB87_11D4_AD55_0080ADA84DE3__INCLUDED_

	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "basesky.h"

class CAdvSky : public CBaseSky
{
public:
	CAdvSky();
	virtual ~CAdvSky();
	void CreateCover(int baseX,int baseY,float* buf);
	void InitSun();
	void CreateTransformVectors();
	void CreateRandMatrix(int matrix[32][32],float mod);
	void CreateRandDetailMatrix(unsigned char* matrix,int size);
	void DrawShafts();
	void ResetCloudShadow(int texunit);
	void SetCloudShadow(int texunit);
	void CreateClouds();
	void Update();
	void DrawSun();
	void Draw();
	float3 GetCoord(int x,int y);
	void CreateDetailTex(void);

	unsigned int skyTex;
	unsigned int skyDot3Tex;
	unsigned int cloudDot3Tex;
	unsigned int displist;

	unsigned int sunTex;
	unsigned int sunFlareTex;
	unsigned int sunFlareList;
	unsigned int cdtex;

	unsigned int cloudFP;

	int randMatrix[16][32][32];

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];

	int lastCloudUpdate;
	bool cloudDown[10];
	bool drawFlare;

	unsigned int detailTextures[12];
	bool cloudDetailDown[5];

	unsigned char *cloudThickness2;
	unsigned char *cloudTexMem;

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
	float fogStart;
};

#endif // !defined(AFX_SKY_H__9A754BA1_AB87_11D4_AD55_0080ADA84DE3__INCLUDED_)
