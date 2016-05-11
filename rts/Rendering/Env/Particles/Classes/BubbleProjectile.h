/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUBBLE_PROJECTILE_H
#define _BUBBLE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CBubbleProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CBubbleProjectile)
public:
	CBubbleProjectile();
	CBubbleProjectile(
		CUnit* owner,
		float3 pos,
		float3 speed,
		int ttl,
		float startSize,
		float sizeExpansion,
		float alpha
	);
	virtual ~CBubbleProjectile();

	void Update() override;
	void Draw() override;

	virtual int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	int ttl;
	float alpha;
	float size;
	float startSize;
	float sizeExpansion;
};

#endif // _BUBBLE_PROJECTILE_H
