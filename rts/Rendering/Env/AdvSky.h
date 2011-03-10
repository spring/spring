/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ADV_SKY_H__
#define __ADV_SKY_H__

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "BaseSky.h"

class CAdvSky : public CBaseSky
{
public:
	CAdvSky();
	virtual ~CAdvSky();

	void Update();
	void UpdateSunFlare();
	void Draw();
	void DrawSun();

private:
	void InitSun();
	void CreateCover(int baseX, int baseY, float* buf);
	void CreateTransformVectors();
	void CreateRandMatrix(int** matrix, float mod);
	void CreateRandDetailMatrix(unsigned char* matrix,int size);
	void CreateClouds();
	void UpdatePart(int ast, int aed, int a3cstart, int a4cstart);

	float3 GetCoord(int x, int y);
	void CreateDetailTex();

	GLuint cdtex;

	unsigned int cloudFP;

	bool drawFlare;

	GLuint detailTextures[12];
	bool cloudDetailDown[5];

	unsigned char* cloudTexMem;

private:
	FBO fbo;
};

#endif // __ADV_SKY_H__
