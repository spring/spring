/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPHERE_PART_PROJECTILE_H
#define SPHERE_PART_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CSpherePartProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CSpherePartProjectile)

public:
	CSpherePartProjectile() { }
	CSpherePartProjectile(
		const CUnit* owner,
		const float3& centerPos,
		int xpart,
		int ypart,
		float expansionSpeed,
		float alpha,
		int ttl,
		const float3& color
	);

	void Draw() override;
	void Update() override;

	virtual int GetProjectilesCount() const override;

	static void CreateSphere(
		const CUnit* owner,
		int ttl,
		float alpha,
		float expansionSpeed,
		float3 pos,
		float3 color = float3(0.8, 0.8, 0.6)
	);

private:
	float3 centerPos;
	float3 vectors[25];
	float3 color;

	float sphereSize;
	float expansionSpeed;

	int xbase;
	int ybase;

	float baseAlpha;
	int age;
	int ttl;
	float texx;
	float texy;
};

/// This class makes a sphere-part-projectile via the explosion-generator
class CSpherePartSpawner : public CProjectile
{
	CR_DECLARE_DERIVED(CSpherePartSpawner)

public:
	CSpherePartSpawner();

	void Init(const CUnit* owner, const float3& offset) override;

	virtual int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float alpha;
	int ttl;
	float expansionSpeed;
	float3 color;
};

#endif /* SPHERE_PART_PROJECTILE_H */
