/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"
#include "Sim/Misc/CollisionVolume.h"

#include <vector>

class ShieldSegmentCollection;
class CRepulseGfx;

class CPlasmaRepulser: public CWeapon
{
	CR_DECLARE_DERIVED(CPlasmaRepulser)
public:
	CPlasmaRepulser(CUnit* owner, const WeaponDef* def);
	~CPlasmaRepulser();

	void Init() override final;
	void DependentDied(CObject* o) override final;
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle = false) const override final;

	void Update() override final;
	void SlowUpdate() override final;


	void SetEnabled(bool b) { isEnabled = b; }
	void SetCurPower(float p) { curPower = p; }

	bool IsEnabled() const { return isEnabled; }
	bool IsActive() const;
	bool IsRepulsing(CWeaponProjectile* p) const;
	float GetCurPower() const { return curPower; }
	float GetRadius() const { return radius; }
	int GetHitFrames() const { return hitFrames; }
	bool CanIntercept(unsigned interceptedType, int allyTeam) const;

	bool IncomingBeam(const CWeapon* emitter, const float3& startPos, const float3& hitPos, float damageMultiplier);
	bool IncomingProjectile(CWeaponProjectile* p, const float3& hitPos);

	//collisions
	std::vector<int> quads;
	CollisionVolume collisionVolume;
	int tempNum;

private:
	void FireImpl(const bool scriptCall) override final {}

	// these are strictly unsynced
	ShieldSegmentCollection* segmentCollection;
	std::vector<CWeaponProjectile*> repulsedProjectiles;


	float3 lastPos;
	float curPower;

	float radius;
	float sqRadius;

	int hitFrames;
	int rechargeDelay;

	bool isEnabled;
};

#endif
