#ifndef BASEWATER_H
#define BASEWATER_H

#include "BaseWater.h"
#include <vector>

class CDynWater :
	public CBaseWater
{
public:
	CDynWater(void);
	~CDynWater(void);

	virtual void Draw();
	virtual void UpdateWater(CGame* game);
	virtual void Update();
	virtual void AddExplosion(const float3& pos, float strength, float size);

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
	unsigned int reflectTexture;
	unsigned int refractTexture;
	unsigned int rawBumpTexture[3];
	unsigned int detailNormalTex;
	unsigned int foamTex;
	float3 waterSurfaceColor;

	unsigned int waveHeight32;
	unsigned int waveTex1;
	unsigned int waveTex2;
	unsigned int waveTex3;
	unsigned int frameBuffer;
	unsigned int zeroTex;
	unsigned int fixedUpTex;

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

	unsigned int splashTex;
	unsigned int boatShape;
	unsigned int hoverShape;

	int lastWaveFrame;
	bool firstDraw;

	unsigned int waterFP;
	unsigned int waterVP;

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
