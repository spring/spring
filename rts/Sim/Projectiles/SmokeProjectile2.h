#ifndef SMOKEPROJECTILE2_H
#define SMOKEPROJECTILE2_H
// SmokeProjectile.h: interface for the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Projectile.h"

class CSmokeProjectile2 : public CProjectile  
{
public:
	CR_DECLARE(CSmokeProjectile2);

	void Update();
	void Draw();
	void Init(const float3& pos, CUnit *owner);
	CSmokeProjectile2();
	CSmokeProjectile2(float3 pos,float3 wantedPos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner,float color=0.7f);
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
