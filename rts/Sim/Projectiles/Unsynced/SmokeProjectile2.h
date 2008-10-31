#ifndef SMOKEPROJECTILE2_H
#define SMOKEPROJECTILE2_H
// SmokeProjectile.h: interface for the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Projectiles/Projectile.h"

class CSmokeProjectile2 : public CProjectile
{
public:
	CR_DECLARE(CSmokeProjectile2);

	void Update();
	void Draw();
	void Init(const float3& pos, CUnit *owner GML_PARG_H);
	CSmokeProjectile2();
	CSmokeProjectile2(float3 pos,float3 wantedPos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner,float color=0.7f GML_PARG_H);
	virtual ~CSmokeProjectile2();

	float color;
	float age;
	float ageSpeed;
	float size;
	float startSize;
	float sizeExpansion;
	int textureNum;
	float3 wantedPos;
	float glowFalloff;
};

#endif /* SMOKEPROJECTILE2_H */
