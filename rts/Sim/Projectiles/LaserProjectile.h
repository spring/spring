#ifndef LASERPROJECTILE_H
#define LASERPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CLaserProjectile :
	public CWeaponProjectile
{
public:
	CLaserProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,float length,const float3& color,const float3& color2, float intensity, WeaponDef *weaponDef, int ttl=1000);
	virtual ~CLaserProjectile();
	void Draw(void);
	void Update(void);
	void Collision(CUnit* unit);
	void Collision(CFeature* feature);
	void Collision();
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);

	float3 dir;
	int ttl;
	float intensity;
	float3 color;
	float3 color2;
	float length;
	float curLength;
	float speedf;
	float intensityFalloff;
	float midtexx;

};


#endif /* LASERPROJECTILE_H */
