#ifndef MISSILEPROJECTILE_H
#define MISSILEPROJECTILE_H
/*pragma once removed*/
#include "WeaponProjectile.h"
#include "DamageArray.h"

class CUnit;

class CMissileProjectile :
	public CWeaponProjectile
{
public:
	CMissileProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,float areaOfEffect,float maxSpeed,float tracking, int ttl,CUnit* target, WeaponDef *weaponDef);
	~CMissileProjectile(void);
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();

	void Update(void);
	void Draw(void);
	void DrawUnitPart(void);

	float tracking;
	float3 dir;
	float maxSpeed;
	float curSpeed;
	int ttl;
	DamageArray damages;
	float areaOfEffect;
	int age;
	float3 oldSmoke,oldDir;
	CUnit* target;
	CProjectile* decoyTarget;
	bool drawTrail;
	int numParts;

	unsigned int modelDispList;
};


#endif /* MISSILEPROJECTILE_H */
