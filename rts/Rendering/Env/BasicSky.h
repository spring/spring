/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASIC_SKY_H__
#define __BASIC_SKY_H__

#include "Rendering/GL/myGL.h"
#include "BaseSky.h"

class CBasicSky : public CBaseSky
{
public:
	CBasicSky();
	virtual ~CBasicSky();

	void Update();
	void UpdateSunFlare();
	void Draw();
	void DrawSun();

private:
	void InitSun();
	void CreateCover(int baseX, int baseY, float* buf);
	void CreateTransformVectors();
	void CreateRandMatrix(int **matrix,float mod);
	void CreateClouds();
	void UpdatePart(int ast, int aed, int a3cstart, int a4cstart);

	float3 GetCoord(int x, int y);

protected:
	inline unsigned char GetCloudThickness(int x, int y);
};

#endif // __BASIC_SKY_H__
