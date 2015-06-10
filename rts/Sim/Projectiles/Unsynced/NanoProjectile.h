/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NANO_PROJECTILE_H
#define NANO_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CNanoProjectile : public CProjectile
{
	CR_DECLARE(CNanoProjectile)

public:
	CNanoProjectile();
	CNanoProjectile(const float3& pos, const float3& speed, int lifeTime, const float3& color);
	virtual ~CNanoProjectile();

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);

private:
	int creationTime;
	int lifeTime;
	unsigned char color[4];
};

#endif /* NANO_PROJECTILE_H */

