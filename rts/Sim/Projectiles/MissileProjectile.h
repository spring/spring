#ifndef MISSILEPROJECTILE_H
#define MISSILEPROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CUnit;

class CMissileProjectile :
	public CWeaponProjectile
{
public:
	CMissileProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,float areaOfEffect,float maxSpeed, int ttl,CUnit* target, WeaponDef *weaponDef,float3 targetPos);
	~CMissileProjectile(void);
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();

	void Update(void);
	void Draw(void);
	void DrawUnitPart(void);

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
	float3 targPos;

	bool isWobbling;
	float3 wobbleDir;
	int wobbleTime;
	float3 wobbleDif;

	float extraHeight;
	float extraHeightDecay;
	int extraHeightTime;
};


#endif /* MISSILEPROJECTILE_H */
