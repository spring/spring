/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURST_PROJECTILE_H
#define STARBURST_PROJECTILE_H

#include "WeaponProjectile.h"


class CSmokeTrailProjectile;

class CStarburstProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CStarburstProjectile)
	CR_DECLARE_SUB(TracerPart)

public:
	// creg only
	CStarburstProjectile() { }

	CStarburstProjectile(const ProjectileParams& params);

	void Collision(CUnit* unit) override;
	void Collision(CFeature* feature) override;
	void Collision() override;
	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;

	int GetProjectilesCount() const override { return (numParts + 1); }

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

	void SetIgnoreError(bool b) { ignoreError = b; }
private:
	void UpdateTargeting();
	void UpdateTrajectory();

	void InitTracerParts();
	void UpdateTracerPart();
	void UpdateSmokeTrail();

private:
	float3 aimError;
	float3 oldSmoke;
	float3 oldSmokeDir;

	float tracking = 0.0f;
	float maxGoodDif = 0.0f;
	float maxSpeed = 0.0f;
	float acceleration = 0.0f;
	float distanceToTravel = 0.0f;

	int uptime = 0;
	int trailAge = 0;
	int numParts = 0;
	int missileAge = 0;
	unsigned int curTracerPart = 0;

	bool ignoreError = false;
	bool turnToTarget = true;
	bool leaveSmokeTrail = false;

	static constexpr unsigned int NUM_TRACER_PARTS = 3;
	static constexpr unsigned int MAX_NUM_AGEMODS = 20;
	static constexpr unsigned int SMOKE_INTERVAL = 8;

	struct TracerPart {
		CR_DECLARE_STRUCT(TracerPart)

		float3 pos;
		float3 dir;
		float speedf = 0.0f;
		float ageMods[MAX_NUM_AGEMODS] = {1.0f};
		unsigned int numAgeMods = 0;
	};

	TracerPart tracerParts[NUM_TRACER_PARTS];
	CSmokeTrailProjectile* smokeTrail = nullptr;
};

#endif /* STARBURST_PROJECTILE_H */
