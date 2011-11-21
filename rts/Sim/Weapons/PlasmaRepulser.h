/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"
#include <list>
#include <set>

class CProjectile;
class CRepulseGfx;
class CShieldPartProjectile;

class CPlasmaRepulser :
	public CWeapon
{
	CR_DECLARE(CPlasmaRepulser);
public:
	CPlasmaRepulser(CUnit* owner);
	~CPlasmaRepulser(void);
	void Init(void);
	void DependentDied(CObject* o);

	void Update(void);
	void SlowUpdate(void);

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
	bool wasDrawn;
	bool startShowingShield;

private:
	virtual void FireImpl() {};

private:
	std::set<CWeaponProjectile*> hasGfx;
	std::list<CShieldPartProjectile*> visibleShieldParts;
};

#endif
