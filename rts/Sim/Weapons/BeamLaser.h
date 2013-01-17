/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAMLASER_H
#define BEAMLASER_H

#include "Weapon.h"

class CBeamLaser :
	public CWeapon
{
	CR_DECLARE(CBeamLaser);
public:
	CBeamLaser(CUnit* owner);

	void Update();
	void Init();
	void FireInternal(float3 dir, bool sweepFire);

	float3 color;
	float3 oldDir;
	float damageMul;

	unsigned int lastFireFrame;

private:
	virtual void FireImpl();
};

#endif
