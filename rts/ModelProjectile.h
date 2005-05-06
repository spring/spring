#pragma once
#include "WeaponProjectile.h"

struct S3DOModel;

class CModelProjectile :
	public CWeaponProjectile
{
public:
	CModelProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef);
	~CModelProjectile(void);

	//void Draw();
	void Update();
	void DrawUnitPart();

protected:
	S3DOModel *model;
};

