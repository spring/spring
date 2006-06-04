#ifndef SMOKEPROJECTILE_H
#define SMOKEPROJECTILE_H
// SmokeProjectile.h: interface for the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Projectile.h"

class CSmokeProjectile : public CProjectile  
{
public:
	CR_DECLARE(CSmokeProjectile)

	void Update();
	void Draw();
	void Init(const float3& pos, CUnit *owner);
	CSmokeProjectile();
	CSmokeProjectile(const float3& pos,const float3& speed,float ttl,float startSize,float sizeExpansion, CUnit* owner,float color=0.7f);
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
