/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ADV_SKY_H__
#define __ADV_SKY_H__

#include "Rendering/GL/myGL.h"
#include "BaseSky.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

class CAdvSky : public CBaseSky
{
public:
	CAdvSky();
	virtual ~CAdvSky();
	void CreateCover(int baseX,int baseY,float* buf);
	void InitSun();
	void CreateTransformVectors();
	void CreateRandMatrix(int **matrix,float mod);
	void CreateRandDetailMatrix(unsigned char* matrix,int size);
	void DrawShafts();
	void ResetCloudShadow(int texunit);
	void SetCloudShadow(int texunit);
	void CreateClouds();
	void UpdatePart(int ast, int aed, int a3cstart, int a4cstart);
	void Update();
	void DrawSun();
	void Draw();
	float3 GetCoord(int x,int y);
	void CreateDetailTex(void);

	GLuint skyTex;
	GLuint skyDot3Tex;
	GLuint cloudDot3Tex;
	unsigned int displist;

	GLuint sunTex;
	GLuint sunFlareTex;
	unsigned int sunFlareList;
	GLuint cdtex;

	unsigned int cloudFP;

	int ***randMatrix;
	int **rawClouds;
	int ***blendMatrix;

	int ydif[CLOUD_SIZE];
	int updatecounter;

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];

	bool cloudDown[10];
	bool drawFlare;

	GLuint detailTextures[12];
	bool cloudDetailDown[5];

	unsigned char *cloudThickness;
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

#endif // __ADV_SKY_H__

