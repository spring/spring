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
	CMissileProjectile(const ProjectileParams& params);

	void Collision(CUnit* unit);
	void Collision(CFeature* feature);
	void Collision();

	void Update();
	void Draw();

	int ShieldRepulse(
		CPlasmaRepulser* shield,
		float3 shieldPos,
		float shieldForce,
		float shieldMaxSpeed
	);

private:
	void UpdateWobble();
	void UpdateDance();

	float maxSpeed;
	float areaOfEffect;
	float extraHeight;
	float extraHeightDecay;

	int age;
	int numParts;
	int extraHeightTime;

	bool drawTrail;
	bool isDancing;
	bool isWobbling;

	int danceTime;
	int wobbleTime;

	float3 danceMove; // points towards center of the dance to keep the movement "coherent"
	float3 danceCenter;
	float3 wobbleDir;
	float3 wobbleDif;

	float3 oldSmoke;
	float3 oldDir;
	
	/// the smokes life-time in frames
	static const float SMOKE_TIME;
};


#endif /* MISSILE_PROJECTILE_H */
