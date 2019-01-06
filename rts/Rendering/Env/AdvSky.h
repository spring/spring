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
	~CAdvSky();

	void Update() override;
	void Draw() override;
	void DrawSun() override;

	void UpdateSunDir() override;
	void UpdateSkyTexture() override;

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
	void UpdateTexPartDot3(int x, int y, uint8_t (*texp)[4]);
	void UpdateTexPart(int x, int y, uint8_t (*texp)[4]);
	void UpdateSkyDir();
	float3 GetDirFromTexCoord(float x, float y);
	float GetTexCoordFromDir(const float3& dir);
	float3 GetCoord(int x, int y);
	void CreateDetailTex();

protected:
	FBO fbo;

	float3 skydir1; // right
	float3 skydir2; // up

	GLuint cdtex = 0;
	GLuint detailTextures[12];

	unsigned int skyTex = 0;
	unsigned int skyDot3Tex = 0;
	unsigned int cloudDot3Tex = 0;
	unsigned int sunTex = 0;
	unsigned int sunFlareTex = 0;

	unsigned int skyTexUpdateIter = 0;

	unsigned int skyDomeList = 0;
	unsigned int sunFlareList = 0;

	unsigned int cloudFP = 0;

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
	uint8_t cloudThickness[CLOUD_SIZE * CLOUD_SIZE + 1];
	uint8_t cloudTexMem[CLOUD_SIZE * CLOUD_SIZE * 4];

	// temporaries
	uint8_t skytex [512][512][4];
	uint8_t skytex2[256][256][4];

	float covers[4][32];
	int oldCoverBaseX = -5;
	int oldCoverBaseY =  0;

	unsigned char alphaTransform[1024];
	unsigned char thicknessTransform[1024];

	bool cloudDown[10];
	bool cloudDetailDown[5];

	int ydif[CLOUD_SIZE];
	int updatecounter = 0;
};

#endif // ADV_SKY_H
