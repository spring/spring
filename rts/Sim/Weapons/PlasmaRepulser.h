/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"
#include <list>
#include <set>

class ShieldProjectile;
class CRepulseGfx;

class CPlasmaRepulser :
	public CWeapon
{
	CR_DECLARE(CPlasmaRepulser);
public:
	CPlasmaRepulser(CUnit* owner);
	~CPlasmaRepulser();
	void Init();
	void DependentDied(CObject* o);
	bool HaveFreeLineOfFire(const float3& pos, bool userTarget, CUnit* unit) const;

	void Update();
	void SlowUpdate();

	void NewProjectile(CWeaponProjectile* p);
	float NewBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir);
	bool BeamIntercepted(CWeapon* emitter, float damageMultiplier = 1.0f); // returns true if we are a repulsing shield

public:
	float curPower;

	float radius;
	float sqRadius;

	int hitFrames;
	int rechargeDelay;

	bool isEnabled;

private:
	void FireImpl() {}

private:
	// these are strictly unsynced
	ShieldProjectile* shieldProjectile;
	std::set<CWeaponProjectile*> hasGfx;
};

#endif
