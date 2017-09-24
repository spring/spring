/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NANO_PROJECTILE_H
#define NANO_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "System/Color.h"

class CNanoProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CNanoProjectile)

public:
	CNanoProjectile();
	CNanoProjectile(float3 pos, float3 speed, int lifeTime, SColor color);
	~CNanoProjectile();

	void Update() override;
	void Draw(CVertexArray* va) override;
	void DrawOnMinimap(CVertexArray& lines, CVertexArray& points) override;

	int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	int deathFrame;
	SColor color;
};

#endif /* NANO_PROJECTILE_H */

