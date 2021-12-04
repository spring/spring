/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ADV_SKY_H
#define ADV_SKY_H

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "ISky.h"

class CAdvSky : public ISky
{
public:
	CAdvSky();
	virtual ~CAdvSky();

	void Update();
	void Draw();
	void DrawSun();

	void UpdateSunDir();
	void UpdateSkyTexture();

private:
	void CreateSkyDomeList();
	void InitSun();
	void UpdateSunFlare();
	void CreateCover(int baseX, int baseY, float* buf) const;
	void CreateTransformVectors();
	void CreateRandMatrix(int** matrix, float mod);
	void CreateRandDetailMatrix(unsigned char* matrix,int size);
	void CreateClouds();
	void UpdatePart(int ast, int aed, int a3cstart, int a4cstart);
	void UpdateTexPartDot3(int x, int y, unsigned char (*texp)[4]);
	void UpdateTexPart(int x, int y, unsigned char (*texp)[4]);
	void UpdateSkyDir();
	float3 GetDirFromTexCoord(float x, float y);
	float GetTexCoordFromDir(const float3& dir);
	float3 GetCoord(int x, int y);
	void CreateDetailTex();

protected:
	FBO fbo;

	float3 skydir1; // right
	float3 skydir2; // up

	GLuint cdtex;

	unsigned int cloudFP;

	bool drawFlare;

	GLuint detailTextures[12];
	bool cloudDetailDown[5];

	unsigned char* cloudTexMem;

	unsigned int skyTex;
	unsigned int skyDot3Tex;
	unsigned int cloudDot3Tex;
	unsigned int sunTex;
	unsigned int sunFlareTex;

	unsigned char (* skytexpart)[4];
	unsigned int skyTexUpdateIter;

	unsigned int skyDomeList;
	unsigned int sunFlareList;

	float skyAngle;

	float domeheight;
	float domeWidth;

	float sunTexCoordX;
	float sunTexCoordY;

	int*** randMatrix;
	int** rawClouds;
	int*** blendMatrix;

	unsigned char* cloudThickness;

	float covers[4][32];
	int oldCoverBaseX;
	int oldCoverBaseY;

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];
	bool cloudDown[10];

	int ydif[CLOUD_SIZE];
	int updatecounter;
};

#endif // ADV_SKY_H
