/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WRECKPROJECTILE_H
#define WRECKPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWreckProjectile : public CProjectile
{
public:
	CR_DECLARE(CWreckProjectile);
	void Update();
	CWreckProjectile(float3 pos,float3 speed,float temperature,CUnit* owner GML_PARG_H);
	virtual ~CWreckProjectile();

	void Draw(void);
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
};

#endif /* WRECKPROJECTILE_H */
