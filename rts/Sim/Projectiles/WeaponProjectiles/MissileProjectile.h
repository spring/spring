/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MISSILE_PROJECTILE_H
#define MISSILE_PROJECTILE_H

#include "WeaponProjectile.h"

class CUnit;

class CMissileProjectile : public CWeaponProjectile
{
	CR_DECLARE(CMissileProjectile);
protected:
	void UpdateGroundBounce();
public:
	CMissileProjectile(const float3& pos, const float3& speed, CUnit* owner,
			float areaOfEffect, float maxSpeed, int ttl, CUnit* target,
			const WeaponDef* weaponDef, float3 targetPos);
	~CMissileProjectile();
	void DependentDied(CObject* o);
	void Collision(CUnit* unit);
	void Collision();

	void Update();
	void Draw();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float maxSpeed;
	float curSpeed;
	float areaOfEffect;
	int age;
	float3 oldSmoke;
	float3 oldDir;
	CUnit* target;
public:
	CProjectile* decoyTarget;
private:
	bool drawTrail;
	int numParts;
	float3 targPos;

	bool isWobbling;
	float3 wobbleDir;
	int wobbleTime;
	float3 wobbleDif;
	
	bool isDancing;
	int danceTime;
	float3 danceMove;
	/**
	 * Vector that points towards the center of the dance
	 * to keep the movement "coherent"
	 */
	float3 danceCenter;

	float extraHeight;
	float extraHeightDecay;
	int extraHeightTime;

	/// the smokes life-time in frames
	static const float SMOKE_TIME;
};


#endif /* MISSILE_PROJECTILE_H */
