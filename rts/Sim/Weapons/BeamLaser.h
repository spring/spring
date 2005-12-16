#ifndef BEAMLASER_H
#define BEAMLASER_H

#include "Weapon.h"

class CBeamLaser :
	public CWeapon
{
public:
	CBeamLaser(CUnit* owner);
	~CBeamLaser(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void Fire(void);

	float3 color;
	float3 oldDir;
};

#endif
