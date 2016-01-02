/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLASMAREPULSER_H
#define PLASMAREPULSER_H

#include "Weapon.h"
#include "Sim/Misc/CollisionVolume.h"

#include <vector>

class ShieldProjectile;
class CRepulseGfx;

class CPlasmaRepulser: public CWeapon
{
	CR_DECLARE(CPlasmaRepulser)
public:
	CPlasmaRepulser(CUnit* owner, const WeaponDef* def);
	~CPlasmaRepulser();

	void Init() override final;
	void DependentDied(CObject* o) override final;
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg) const override final;

	void Update() override final;
	void SlowUpdate() override final;

	float NewBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir);
	bool BeamIntercepted(CWeapon* emitter, float3 start, float damageMultiplier = 1.0f); // returns true if we are a repulsing shield

	void SetEnabled(bool b) { isEnabled = b; }
	void SetCurPower(float p) { curPower = p; }

	bool IsEnabled() const { return isEnabled; }
	bool IsActive() const;
	bool IsRepulsing(CWeaponProjectile* p) const;
	float GetCurPower() const { return curPower; }
	float GetRadius() const { return radius; }
	int GetHitFrames() const { return hitFrames; }

	bool IncomingProjectile(CWeaponProjectile* p);

	//collisions
	std::vector<int> quads;
	CollisionVolume collisionVolume;
	int tempNum;

private:
	void FireImpl(const bool scriptCall) override final {}

	// these are strictly unsynced
	ShieldProjectile* shieldProjectile;
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
