/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHIELDPARTPROJECTILE_H
#define SHIELDPARTPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CShieldPartProjectile :
	public CProjectile
{
	CR_DECLARE(CShieldPartProjectile);
public:
	CShieldPartProjectile(const float3& centerPos,int xpart,int ypart,float size,float3 color,float alpha, AtlasedTexture *texture,CUnit* owner);
	~CShieldPartProjectile(void);
	void Draw(void);
	void Update(void);

	float3 centerPos;
	float3 vectors[25];
	float3 texCoords[25];

	float sphereSize;

	float baseAlpha;
	float3 color;
	bool usePerlin;
};

#endif // SHIELDPARTPROJECTILE_H
