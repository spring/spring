/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURST_PROJECTILE_H
#define STARBURST_PROJECTILE_H

#include "WeaponProjectile.h"
#include <vector>


class CSmokeTrailProjectile;

class CStarburstProjectile : public CWeaponProjectile
{
	CR_DECLARE(CStarburstProjectile)
	CR_DECLARE_SUB(TracerPart)

public:
	CStarburstProjectile(const ProjectileParams& params);
	~CStarburstProjectile();

	void Collision(CUnit* unit) override;
	void Collision(CFeature* feature) override;
	void Collision() override;
	void Update() override;
	void Draw() override;

	virtual int GetProjectilesCount() const override;

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

	void SetIgnoreError(bool b) { ignoreError = b; }
private:
	void DrawCallback() override;

	void UpdateTrajectory();

private:
	float tracking;
	bool ignoreError;
	float maxGoodDif;
	float maxSpeed;
	float acceleration;
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
	unsigned int curTracerPart;

	struct TracerPart {
		CR_DECLARE_STRUCT(TracerPart)

		float3 pos;
		float3 dir;
		float speedf;
		std::vector<float> ageMods;
		unsigned int numAgeMods;
	};

	static const unsigned int NUM_TRACER_PARTS = 5;
	TracerPart tracerParts[NUM_TRACER_PARTS];
};

#endif /* STARBURST_PROJECTILE_H */
