// Sky.h: interface for the CSky class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BASIC_SKY_H__
#define __BASIC_SKY_H__

#include "Rendering/GL/myGL.h"
#include "BaseSky.h"

#define CLOUD_SIZE 256 // must be divisible by 4 and 8

class CBasicSky : public CBaseSky
{
public:
	void CreateCover(int baseX,int baseY,float* buf);
	void InitSun();
	void CreateTransformVectors();
	void CreateRandMatrix(int **matrix,float mod);
	void DrawShafts();
	void ResetCloudShadow(int texunit);
	void SetCloudShadow(int texunit);
	void CreateClouds();
	void UpdatePart(int ast, int aed, int a3cstart, int a4cstart);
	void Update();
	void DrawSun();
	void Draw();
	float3 GetCoord(int x,int y);
	float3 GetTexCoord(int x,int y);
	CBasicSky();
	virtual ~CBasicSky();

	GLuint skyTex;
	GLuint skyDot3Tex;
	GLuint cloudDot3Tex;
	unsigned int displist;

	GLuint sunTex;
	GLuint sunFlareTex;
	unsigned int sunFlareList;

	int ***randMatrix;
	int **rawClouds;
	int ***blendMatrix;

	int ydif[CLOUD_SIZE];

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
