#ifndef GFXPROJECTILE_H
#define GFXPROJECTILE_H
// GfxProjectile.h: interface for the CGfxProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Projectiles/Projectile.h"

class CGfxProjectile : public CProjectile
{
public:
	CR_DECLARE(CGfxProjectile);

	void Update();
	void Draw();
	CGfxProjectile();
	CGfxProjectile(const float3& pos,const float3& speed,int lifeTime,const float3& color GML_PARG_H);
	virtual ~CGfxProjectile();

	int creationTime;
	int lifeTime;
	unsigned char color[4];
};

#endif /* GFXPROJECTILE_H */
