#ifndef BEAMLASER_H
#define BEAMLASER_H

#include "Weapon.h"

class CBeamLaser :
	public CWeapon
{
	CR_DECLARE(CBeamLaser);
public:
	CBeamLaser(CUnit* owner);
	~CBeamLaser(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void FireInternal(float3 dir, bool sweepFire);

	float3 color;
	float3 oldDir;
	float damageMul;

	unsigned int lastFireFrame;

private:
	virtual void FireImpl(void);
};

#endif
