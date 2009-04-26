#ifndef LASERPROJECTILE_H
#define LASERPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CLaserProjectile :
	public CWeaponProjectile
{
	CR_DECLARE(CLaserProjectile);
public:
	CLaserProjectile(const float3& pos, const float3& speed, CUnit* owner,float length,
		const float3& color, const float3& color2, float intensity, const WeaponDef *weaponDef,
		int ttl = 1000 GML_PARG_H);
	virtual ~CLaserProjectile();
	void Draw(void);
	void Update(void);
	void Collision(CUnit* unit);
	void Collision(CFeature* feature);
	void Collision();
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);

	float3 dir;
	float intensity;
	float3 color;
	float3 color2;
	float length;
	float curLength;
	float speedf;
	float intensityFalloff;
	float midtexx;
	/**
	 * Number of frames the laser had left to expand
	 * if it impacted before reaching full length.
	 */
	int stayTime;


};

#endif /* LASERPROJECTILE_H */
