/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WRECK_PROJECTILE_H
#define WRECK_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWreckProjectile : public CProjectile
{
	CR_DECLARE(CWreckProjectile);
public:
	CWreckProjectile(float3 pos, float3 speed, float temperature, CUnit* owner);
	virtual ~CWreckProjectile();

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
};

#endif /* WRECK_PROJECTILE_H */
