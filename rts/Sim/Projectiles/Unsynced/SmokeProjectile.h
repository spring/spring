#ifndef SMOKEPROJECTILE_H
#define SMOKEPROJECTILE_H
// SmokeProjectile.h: interface for the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Sim/Projectiles/Projectile.h"

class CSmokeProjectile : public CProjectile
{
public:
	CR_DECLARE(CSmokeProjectile)

	void Update();
	void Draw();
	void Init(const float3& pos, CUnit *owner GML_PARG_H);
	CSmokeProjectile();
	CSmokeProjectile(const float3& pos,const float3& speed,float ttl,float startSize,float sizeExpansion, CUnit* owner,float color=0.7f GML_PARG_H);
	virtual ~CSmokeProjectile();

	float color;
	float age;
	float ageSpeed;
	float size;
	float startSize;
	float sizeExpansion;
	int textureNum;
};

#endif /* SMOKEPROJECTILE_H */
