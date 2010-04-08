/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAMLASERPROJECTILE_H
#define BEAMLASERPROJECTILE_H

#include "WeaponProjectile.h"

class CBeamLaserProjectile: public CWeaponProjectile
{
	CR_DECLARE(CBeamLaserProjectile);
public:
	CBeamLaserProjectile(const float3& startPos, const float3& endPos,
		float startAlpha, float endAlpha, const float3& color, const float3& color2,
		CUnit* owner, float thickness, float corethickness, float flaresize,
		const WeaponDef* weaponDef, int ttl, float decay);
	~CBeamLaserProjectile(void);

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

	void Update(void);
	void Draw(void);
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
};

#endif
