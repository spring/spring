/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BASIC_SKY_H
#define BASIC_SKY_H

#include "ISky.h"

class CBasicSky : public ISky
{
public:
	CBasicSky();
	~CBasicSky();

	void Update() override;
	void Draw() override;
	void DrawSun() override;

	void UpdateSunDir() override;
	void UpdateSkyTexture() override;

private:
	void CreateSkyDomeList();
	void InitSun();
	void UpdateSunFlare();
	void CreateCover(int baseX, int baseY, float* buf);
	void CreateTransformVectors();
	void CreateRandMatrix(int **matrix,float mod);
	void CreateClouds();
	void UpdatePart(int ast, int aed, int a3cstart, int a4cstart);
	void UpdateTexPartDot3(int x, int y, uint8_t (*texp)[4]);
	void UpdateTexPart(int x, int y, uint8_t (*texp)[4]);
	void UpdateSkyDir();
	float3 GetDirFromTexCoord(float x, float y);
	float GetTexCoordFromDir(const float3& dir);
	float3 GetCoord(int x, int y);

protected:
	inline unsigned char GetCloudThickness(int x, int y);

	float3 skydir1; // right
	float3 skydir2; // up

	unsigned int skyTex = 0;
	unsigned int skyDot3Tex = 0;
	unsigned int cloudDot3Tex = 0;
	unsigned int sunFlareTex = 0;

	unsigned int skyTexUpdateIter = 0;

	unsigned int skyDomeList = 0;
	unsigned int sunFlareList = 0;

	float skyAngle = 0.0f;

	float domeHeight = 0.0f;
	float domeWidth = 0.0f;
	float minDomeDist = 100000.0f;

	float sunTexCoordX = 0.0f;
	float sunTexCoordY = 0.0f;

	int*** randMatrix = nullptr;
	int** rawClouds = nullptr;
	int*** blendMatrix = nullptr;

	uint8_t skytexpart[512][4];
	uint8_t cloudThickness[CLOUD_SIZE * CLOUD_SIZE * 4 + 4];

	// temporaries
	uint8_t skytex [512][512][4];
	uint8_t skytex2[256][256][4];

	bool cloudDown[10];

	float covers[4][32];
	int oldCoverBaseX = -5;
	int oldCoverBaseY =  0;

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];

	int ydif[CLOUD_SIZE];
	int updatecounter = 0;
};

#endif // BASIC_SKY_H
