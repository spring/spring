#ifndef BASEWATER_H
#define BASEWATER_H

#include "BaseWater.h"
#include <vector>
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"

class CDynWater : public CBaseWater
{
public:
	CDynWater(void);
	~CDynWater(void);

	virtual void Draw();
	virtual void UpdateWater(CGame* game);
	virtual void Update();
	virtual void AddExplosion(const float3& pos, float strength, float size);
	int GetID() const { return 2; }

private:
	void DrawReflection(CGame* game);
	void DrawRefraction(CGame* game);
	void DrawWaves(void);
	void DrawHeightTex(void);
	void DrawWaterSurface(void);
	void UpdateCamRestraints(void);
	void DrawDetailNormalTex(void);
	void AddShipWakes();
	void AddExplosions();
	void DrawUpdateSquare(float dx,float dy, int* resetTexs);
	void DrawSingleUpdateSquare(float startx,float starty,float endx,float endy);

	int refractSize;
	GLuint reflectTexture;
	GLuint refractTexture;
	GLuint rawBumpTexture[3];
	GLuint detailNormalTex;
	GLuint foamTex;
	float3 waterSurfaceColor;

	GLuint waveHeight32;
	GLuint waveTex1;
	GLuint waveTex2;
	GLuint waveTex3;
	GLuint frameBuffer;
	GLuint zeroTex;
	GLuint fixedUpTex;

	unsigned int waveFP;
	unsigned int waveVP;
	unsigned int waveFP2;
	unsigned int waveVP2;
	unsigned int waveNormalFP;
	unsigned int waveNormalVP;
	unsigned int waveCopyHeightFP;
	unsigned int waveCopyHeightVP;
	unsigned int dwGroundRefractVP;
	unsigned int dwGroundReflectIVP;
	unsigned int dwDetailNormalVP;
	unsigned int dwDetailNormalFP;
	unsigned int dwAddSplashVP;
	unsigned int dwAddSplashFP;

	GLuint splashTex;
	GLuint boatShape;
	GLuint hoverShape;

	int lastWaveFrame;
	bool firstDraw;

	unsigned int waterFP;
	unsigned int waterVP;

	FBO reflectFBO;
	FBO refractFBO;

	float3 reflectForward;
	float3 reflectRight;
	float3 reflectUp;

	float3 refractForward;
	float3 refractRight;
	float3 refractUp;

	float3 camPosBig;
	float3 oldCamPosBig;
	float3 camPosBig2;

	int camPosX,camPosZ;

	void AddFrustumRestraint(float3 side);
	struct fline {
		float base;
		float dir;
	};
	std::vector<fline> right,left;

	struct Explosion{
		Explosion(float3 pos,float strength,float radius): pos(pos),strength(strength),radius(radius){};
		float3 pos;
		float strength;
		float radius;
	};
	std::vector<Explosion> explosions;
	void DrawOuterSurface(void);
};

#endif
