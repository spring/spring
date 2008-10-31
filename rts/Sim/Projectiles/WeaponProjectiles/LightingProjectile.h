#ifndef LIGHTINGPROJECTILE_H
#define LIGHTINGPROJECTILE_H

#include "WeaponProjectile.h"

class CWeapon;

class CLightingProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CLightingProjectile);
public:
	CLightingProjectile(const float3& pos, const float3& end, CUnit* owner, const float3& color,
		const WeaponDef *weaponDef, int ttl = 10, CWeapon* weap = 0 GML_PARG_H);
	~CLightingProjectile(void);

	float3 color;
	float3 endPos;
	CWeapon* weapon;

	float displacements[10];
	float displacements2[10];

	void Update(void);
	void Draw(void);
	void DependentDied(CObject* o);
};


#endif /* LIGHTINGPROJECTILE_H */
