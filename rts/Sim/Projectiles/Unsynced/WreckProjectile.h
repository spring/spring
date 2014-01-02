/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WRECK_PROJECTILE_H
#define WRECK_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWreckProjectile : public CProjectile
{
	CR_DECLARE(CWreckProjectile);
public:
	CWreckProjectile(CUnit* owner, float3 pos, float3 speed, float temperature);

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
};

#endif /* WRECK_PROJECTILE_H */
