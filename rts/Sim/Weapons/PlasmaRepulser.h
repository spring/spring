/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"
#include "Sim/Misc/CollisionVolume.h"

#include <vector>

class CPlasmaRepulser: public CWeapon
{
	CR_DECLARE_DERIVED(CPlasmaRepulser)

public:
	CPlasmaRepulser(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}
	~CPlasmaRepulser();

	void Init() override final;
	void DependentDied(CObject* o) override final;
	bool HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const override final { return true; }

	void Update() override final;
	void SlowUpdate() override final;


	void SetEnabled(bool b) { isEnabled = b; }
	void SetCurPower(float p) { curPower = p; }

	bool IsEnabled() const { return isEnabled; }
	bool IsActive() const;

	bool IsRepulsing(CWeaponProjectile* p) const;
	bool IgnoreInteriorHit(CWeaponProjectile* p) const;

	float GetDeltaDist() const { return (deltaMuzzlePos.Length()); }
	float GetCurPower() const { return curPower; }
	float GetRadius() const { return radius; }

	int GetHitFrames() const { return hitFrameCount; }
	bool CanIntercept(unsigned interceptedType, int allyTeam) const;

	bool IncomingBeam(const CWeapon* emitter, const float3& startPos, const float3& hitPos, float damageMultiplier);
	bool IncomingProjectile(CWeaponProjectile* p, const float3& hitPos);

	const std::vector<int>& GetQuads() const { return quads; }

	void SetQuads(std::vector<int>&& q) { quads = std::move(q); }
	void ClearQuads() { quads.clear(); }

	static void SerializeShieldSegmentCollectionPool(creg::ISerializer* s);

private:
	void FireImpl(const bool scriptCall) override final {}

private:
	// these are strictly unsynced
	std::vector<CWeaponProjectile*> repulsedProjectiles;

	// collisions
	std::vector<int> quads;

public:
	CollisionVolume collisionVolume;

	int tempNum = 0;
	int scIndex = 0;

private:
	int hitFrameCount = 0;
	int rechargeDelay = 0;

	float curPower = 0.0f;

	float radius = 0.0f;
	float sqRadius = 0.0f;

	float3 lastMuzzlePos;
	float3 deltaMuzzlePos;

	bool isEnabled = true;
};

#endif
