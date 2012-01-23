/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"
#include <list>
#include <set>

class CProjectile;
class CRepulseGfx;
struct ShieldSegment;

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

	void AddShieldSegment(ShieldSegment* ss) { shieldSegments.push_back(ss); }
	const std::list<ShieldSegment*>& GetShieldSegments() const { return shieldSegments; }
	      std::list<ShieldSegment*>& GetShieldSegments()       { return shieldSegments; }

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
	std::set<CWeaponProjectile*> hasGfx;
	std::list<ShieldSegment*> shieldSegments;
};

#endif
