#pragma once
#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CShieldPartProjectile :
	public CProjectile
{
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
};
