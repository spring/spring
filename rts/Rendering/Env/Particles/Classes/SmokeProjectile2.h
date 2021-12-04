/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOKE_PROJECTILE_2_H
#define SMOKE_PROJECTILE_2_H

#include "Sim/Projectiles/Projectile.h"
#include "System/float3.h"

class CUnit;

class CSmokeProjectile2 : public CProjectile
{
	CR_DECLARE_DERIVED(CSmokeProjectile2)

public:
	CSmokeProjectile2();
	CSmokeProjectile2(
		CUnit* owner,
		const float3& pos,
		const float3& wantedPos,
		const float3& speed,
		float ttl,
		float startSize,
		float sizeExpansion,
		float color = 0.7f
	);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;
	void Init(const CUnit* owner, const float3& offset) override;

	int GetProjectilesCount() const override { return 1; }

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float color;
	float age;
	float ageSpeed;
	float size;
	float startSize;
	float sizeExpansion;
	int textureNum;
	float3 wantedPos;
	float glowFalloff;
};

#endif /* SMOKE_PROJECTILE_2_H */
