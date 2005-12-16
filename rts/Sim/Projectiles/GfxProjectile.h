#ifndef GFXPROJECTILE_H
#define GFXPROJECTILE_H
// GfxProjectile.h: interface for the CGfxProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Projectile.h"

class CGfxProjectile : public CProjectile  
{
public:
	void Update();
	void Draw();
	CGfxProjectile(const float3& pos,const float3& speed,int lifeTime,const float3& color);
	virtual ~CGfxProjectile();

	int creationTime;
	int lifeTime;
	unsigned char color[4];
};

#endif /* GFXPROJECTILE_H */
