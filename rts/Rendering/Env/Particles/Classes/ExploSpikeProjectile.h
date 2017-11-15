/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXPLO_SPIKE_PROJECTILE_H
#define EXPLO_SPIKE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CExploSpikeProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CExploSpikeProjectile)

public:
	CExploSpikeProjectile();

	CExploSpikeProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& spd,
		float length,
		float width,
		float alpha,
		float alphaDecay
	);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;

	void Init(const CUnit* owner, const float3& offset) override;
	// override this so the projectile does not instantly disappear
	// after creation if it also happened to collide with something
	void Collision() override {}

	int GetProjectilesCount() const override { return 1; }

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float length;
	float width;
	float alpha;
	float alphaDecay;
	float lengthGrowth;
	float3 color;
};

#endif // EXPLO_SPIKE_PROJECTILE_H
