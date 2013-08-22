/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BEAMLASER_H
#define BEAMLASER_H

#include "Weapon.h"

class CBeamLaser: public CWeapon
{
	CR_DECLARE(CBeamLaser);
public:
	CBeamLaser(CUnit* owner, const WeaponDef* def);

	void Update();
	void Init();

private:
	float3 GetFireDir(bool sweepFire);

	void FireInternal(bool sweepFire);
	void FireImpl();

private:
	float3 color;
	float3 oldDir;
	float3 lastSweepFirePos;
	float3 lastSweepFireDir;

	float salvoDamageMult;

	bool sweepFiring;
};

#endif
