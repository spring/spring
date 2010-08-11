/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GFX_PROJECTILE_H
#define GFX_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CGfxProjectile : public CProjectile
{
	CR_DECLARE(CGfxProjectile);

public:
	CGfxProjectile();
	CGfxProjectile(const float3& pos,const float3& speed,int lifeTime,const float3& color);
	virtual ~CGfxProjectile();

	void Update();
	void Draw();
	virtual void DrawOnMinimap(CVertexArray& lines, CVertexArray& points);

	int creationTime;
	int lifeTime;
	unsigned char color[4];
};

#endif /* GFX_PROJECTILE_H */
