/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIGHTNINGPROJECTILE_H
#define LIGHTNINGPROJECTILE_H

#include "WeaponProjectile.h"

class CWeapon;

class CLightningProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CLightningProjectile);
public:
	CLightningProjectile(const float3& pos, const float3& end, CUnit* owner, const float3& color,
		const WeaponDef *weaponDef, int ttl = 10, CWeapon* weap = 0);
	~CLightningProjectile(void);

	float3 color;
	float3 endPos;
	CWeapon* weapon;

	float displacements[10];
	float displacements2[10];

	void Update(void);
	void Draw(void);
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	void DependentDied(CObject* o);
};


#endif /* LIGHTNINGPROJECTILE_H */
