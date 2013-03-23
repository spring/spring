/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURST_PROJECTILE_H
#define STARBURST_PROJECTILE_H

#include "WeaponProjectile.h"
#include <vector>


class CSmokeTrailProjectile;

class CStarburstProjectile : public CWeaponProjectile
{
	CR_DECLARE(CStarburstProjectile);
	CR_DECLARE_SUB(TracerPart);

public:
	CStarburstProjectile(const ProjectileParams& params);
	~CStarburstProjectile();

	virtual void Detach();
	void Collision(CUnit* unit);
	void Collision(CFeature* feature);
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
	float areaOfEffect;
	float distanceToTravel;

	int uptime;
	int age;

	float3 oldSmoke;
	float3 oldSmokeDir;
	float3 aimError;

	bool drawTrail;
	bool doturn;

	CSmokeTrailProjectile* curCallback;

	int numParts;
	int missileAge;
	size_t curTracerPart;

	static const int NUM_TRACER_PARTS = 5;

	struct TracerPart {
		CR_DECLARE_STRUCT(TracerPart);

		float3 pos;
		float3 dir;
		float speedf;
		std::vector<float> ageMods;
		size_t numAgeMods;
	};

	TracerPart tracerParts[NUM_TRACER_PARTS];
};

#endif /* STARBURST_PROJECTILE_H */
