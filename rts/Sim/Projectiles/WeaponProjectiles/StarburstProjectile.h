/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURST_PROJECTILE_H
#define STARBURST_PROJECTILE_H

#include "WeaponProjectile.h"
#include <vector>

#if defined(USE_GML) && GML_ENABLE_SIM
#define AGEMOD_VECTOR gmlCircularQueue<float, 64>
#define AGEMOD_VECTOR_IT gmlCircularQueue<float, 64>::iterator
#else
#define AGEMOD_VECTOR std::vector<float>
#define AGEMOD_VECTOR_IT std::vector<float>::const_iterator
#endif

class CSmokeTrailProjectile;

class CStarburstProjectile : public CWeaponProjectile
{
	CR_DECLARE(CStarburstProjectile);
	void creg_Serialize(creg::ISerializer& s);
public:
	CStarburstProjectile(const float3& pos, const float3& speed, CUnit* owner,
			float3 targetPos, float areaOfEffect, float maxSpeed,float tracking,
			int uptime, CUnit* target, const WeaponDef* weaponDef,
			CWeaponProjectile* interceptTarget, float maxdistance,
			float3 aimError);
	~CStarburstProjectile();
	void Collision(CUnit* unit);
	void Collision();
	void Update();
	void Draw();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	void DrawCallback();

	float tracking;
	float maxGoodDif;
	float maxSpeed;
	float curSpeed;
	float acceleration;
	int uptime;
	float areaOfEffect;
	int age;
	float3 oldSmoke;
	float3 oldSmokeDir;
	float3 aimError;
	bool drawTrail;
	int numParts;
	bool doturn;
	CSmokeTrailProjectile* curCallback;
	int* numCallback;
	int missileAge;
	float distanceToTravel;

	static const int NUM_TRACER_PARTS = 5;
	/// the smokes life-time in frames
	static const float SMOKE_TIME;

	struct TracerPart {
		float3 pos;
		float3 dir;
		float speedf;
		AGEMOD_VECTOR ageMods;
	};
	TracerPart *tracerParts[NUM_TRACER_PARTS];
	TracerPart tracerPartMem[NUM_TRACER_PARTS];
};

#endif /* STARBURST_PROJECTILE_H */
