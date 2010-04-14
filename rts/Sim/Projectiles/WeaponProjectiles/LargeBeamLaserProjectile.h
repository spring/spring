/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LARGEBEAMLASERPROJECTILE_H
#define LARGEBEAMLASERPROJECTILE_H

#include "WeaponProjectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CLargeBeamLaserProjectile : public CWeaponProjectile
{
	CR_DECLARE(CLargeBeamLaserProjectile);
public:
	CLargeBeamLaserProjectile(const float3& startPos, const float3& endPos, const float3& color, const float3& color2,
		CUnit* owner, const WeaponDef* weaponDef);
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
	float decay;

	AtlasedTexture beamtex;
	AtlasedTexture side;

	void Update(void);
	void Draw(void);
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
};

#endif
