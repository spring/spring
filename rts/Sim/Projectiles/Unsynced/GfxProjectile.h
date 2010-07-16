/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GFXPROJECTILE_H
#define GFXPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CGfxProjectile : public CProjectile
{
public:
	CR_DECLARE(CGfxProjectile);

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);
	CGfxProjectile();
	CGfxProjectile(const float3& pos,const float3& speed,int lifeTime,const float3& color);
	virtual ~CGfxProjectile();

	int creationTime;
	int lifeTime;
	unsigned char color[4];
};

#endif /* GFXPROJECTILE_H */
