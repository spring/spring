/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAM_LASER_PROJECTILE_H
#define BEAM_LASER_PROJECTILE_H

#include "WeaponProjectile.h"

class CBeamLaserProjectile: public CWeaponProjectile
{
	CR_DECLARE(CBeamLaserProjectile);
public:
	CBeamLaserProjectile(const float3& startPos, const float3& endPos,
		float startAlpha, float endAlpha, const float3& color,
		CUnit* owner, const WeaponDef* weaponDef);
	~CBeamLaserProjectile() {}

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);

private:
	float3 startPos;
	float3 endPos;
	unsigned char corecolstart[4];
	unsigned char corecolend[4];
	unsigned char kocolstart[4];
	unsigned char kocolend[4];

	float thickness;
	float corethickness;
	float flaresize;
	float midtexx;
	
	float decay;
};

#endif // BEAM_LASER_PROJECTILE_H
