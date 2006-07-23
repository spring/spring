#ifndef LARGEBEAMLASERPROJECTILE_H
#define LARGEBEAMLASERPROJECTILE_H

#include "Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CLargeBeamLaserProjectile : public CProjectile
{
public:
	CLargeBeamLaserProjectile(const float3& startPos,const float3& endPos,const float3& color, const float3& color2,CUnit* owner,float thickness, float corethickness,
							float flaresize, float tilelength, float scrollspeed, float pulseSpeed, AtlasedTexture *beamtex, AtlasedTexture *side);
	~CLargeBeamLaserProjectile(void);

	float3 startPos;
	float3 endPos;
	unsigned char corecolstart[4];
	//unsigned char corecolend[4];
	unsigned char kocolstart[4];
	//unsigned char kocolend[4];

	//float startAlpha;
	//float endAlpha;
	float thickness;
	float corethickness;
	float flaresize;
	//float midtexx;
	float tilelength;
	float scrollspeed;
	float pulseSpeed;

	AtlasedTexture beamtex;
	AtlasedTexture side;

	void Update(void);
	void Draw(void);
};

#endif