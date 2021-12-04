/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MISSILE_PROJECTILE_H
#define MISSILE_PROJECTILE_H

#include "WeaponProjectile.h"

class CUnit;
class CSmokeTrailProjectile;


class CMissileProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CMissileProjectile)
protected:
	void UpdateGroundBounce() override;
public:
	// creg only
	CMissileProjectile() { }

	CMissileProjectile(const ProjectileParams& params);

	void Collision(CUnit* unit) override;
	void Collision(CFeature* feature) override;
	void Collision() override;

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;

	int GetProjectilesCount() const override { return 1; }

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

	void SetIgnoreError(bool b) { ignoreError = b; }

private:
	float3 UpdateTargeting();
	void UpdateWobble();
	void UpdateDance();

	bool ignoreError;

	float maxSpeed;
	float extraHeight;
	float extraHeightDecay;

	int age;
	int numParts;
	int extraHeightTime;

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
	CSmokeTrailProjectile* smokeTrail;

	/// the smokes life-time in frames
	static const float SMOKE_TIME;
};


#endif /* MISSILE_PROJECTILE_H */
