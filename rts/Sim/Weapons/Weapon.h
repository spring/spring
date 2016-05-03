/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_H
#define WEAPON_H

#include <map>

#include "System/Object.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Projectiles/ProjectileParams.h"
#include "Sim/Weapons/WeaponTarget.h"
#include "System/float3.h"

class CUnit;
class CWeaponProjectile;
struct WeaponDef;


class CWeapon : public CObject
{
	CR_DECLARE_DERIVED(CWeapon)

public:
	CWeapon(CUnit* owner, const WeaponDef* def);
	virtual ~CWeapon();
	virtual void Init();

	void SetWeaponNum(int);
	void DependentDied(CObject* o) override;
	virtual void SlowUpdate();
	virtual void Update();

public:
	bool Attack(const SWeaponTarget& newTarget);
	void SetAttackTarget(const SWeaponTarget& newTarget); //< does not any checks etc. !
	void DropCurrentTarget();

	bool HaveTarget() const { return (currentTarget.type != Target_None); }
	const SWeaponTarget& GetCurrentTarget() const { return currentTarget; }
	const float3&     GetCurrentTargetPos() const { return currentTargetPos; }

public:
	/// test if the weapon is able to attack an enemy/mapspot just by its properties (no range check, no FreeLineOfFire check, ...)
	virtual bool TestTarget(const float3 tgtPos, const SWeaponTarget& trg) const;
	/// test if the enemy/mapspot is in range/angle
	virtual bool TestRange(const float3 tgtPos, const SWeaponTarget& trg) const;
	/// test if something is blocking our LineOfFire
	virtual bool HaveFreeLineOfFire(const float3 tgtPos, const SWeaponTarget& trg, bool useMuzzle = false) const;

	virtual bool CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const;

	bool TryTarget(const SWeaponTarget& trg) const;
	bool TryTargetRotate(const CUnit* unit, bool userTarget, bool manualFire);
	bool TryTargetRotate(float3 tgtPos, bool userTarget, bool manualFire);
	bool TryTargetHeading(short heading, const SWeaponTarget& trg);

public:
	bool CheckTargetAngleConstraint(const float3 worldTargetDir, const float3 worldWeaponDir) const;
	float3 GetTargetBorderPos(const CUnit* targetUnit, const float3 rawTargetPos, const float3 rawTargetDir) const;

	void AdjustTargetPosToWater(float3& tgtPos, bool attackGround) const;
	float3 GetUnitPositionWithError(const CUnit* unit) const;
	float3 GetUnitLeadTargetPos(const CUnit* unit) const;
	float3 GetLeadTargetPos(const SWeaponTarget& target) const;

	float TargetWeight(const CUnit* unit) const;

	virtual float GetRange2D(const float yDiff) const;
	virtual void UpdateRange(const float val) { range = val; }

	bool AutoTarget();
	void AimReady(const int value);
	void Fire(const bool scriptCall);

	float ExperienceErrorScale() const;
	float MoveErrorExperience() const;
	float AccuracyExperience() const { return (accuracyError * ExperienceErrorScale()); }
	float SprayAngleExperience() const { return (sprayAngle * ExperienceErrorScale()); }
	float3 SalvoErrorExperience() const { return (salvoError * ExperienceErrorScale()); }

	void StopAttackingAllyTeam(const int ally);

protected:
	virtual void FireImpl(const bool scriptCall) {}
	virtual void UpdateWantedDir();
	virtual float GetPredictedImpactTime(float3 p) const; //< how long time we predict it take for a projectile to reach target

	ProjectileParams GetProjectileParams();
	static bool TargetUnderWater(const float3 tgtPos, const SWeaponTarget&);
	static bool TargetInWater(const float3 tgtPos, const SWeaponTarget&);

	void UpdateWeaponPieces(const bool updateAimFrom = true);
	void UpdateWeaponVectors();
	float3 GetLeadVec(const CUnit* unit) const;

private:
	void UpdateAim();
	void UpdateFire();
	bool UpdateStockpile();
	void UpdateSalvo();

	void UpdateInterceptTarget();
	bool AllowWeaponAutoTarget() const;
	bool CobBlockShot() const;
	void ReAimWeapon();
	void HoldIfTargetInvalid();

	bool TryTarget(const float3 tgtPos, const SWeaponTarget& trg, bool preFire = false) const;
public:
	CUnit* owner;

	const WeaponDef* weaponDef;

	int weaponNum;							// the weapons order among the owner weapons

	int aimFromPiece;
	int muzzlePiece;

	int reloadTime;							// time between succesive fires in ticks
	int reloadStatus;						// next tick the weapon can fire again

	float range;
	const DynDamageArray* damages;

	float projectileSpeed;
	float accuracyError;					// inaccuracy of whole salvo
	float sprayAngle;						// inaccuracy of individual shots inside salvo

	int salvoDelay;							// delay between shots in a salvo
	int salvoSize;							// number of shots in a salvo
	int projectilesPerShot;					// number of projectiles per shot
	int nextSalvo;							// when the next shot in the current salvo will fire
	int salvoLeft;							// number of shots left in current salvo

protected:
	SWeaponTarget currentTarget;
	float3 currentTargetPos;

public:
	float predictSpeedMod;					// how the weapon predicts the speed of the units goes -> 1 when experience increases

	bool hasBlockShot;						// set when the script has a BlockShot() function for this weapon
	bool hasTargetWeight;					// set when there's a TargetWeight() function for this weapon
	bool angleGood;							// set when script indicated ready to fire
	bool avoidTarget;						// set when the script wants the weapon to pick a new target, reset once one has been chosen
	bool onlyForward;						// can only fire in the forward direction of the unit (for aircrafts mostly?)
	bool doTargetGroundPos;    // (used for bombers) target the ground pos under the unit instead of the center aimPos
	bool noAutoTarget;
	bool alreadyWarnedAboutMissingPieces;

	unsigned int badTargetCategory;			// targets in this category get a lot lower targetting priority
	unsigned int onlyTargetCategory;		// only targets in this category can be targeted (default 0xffffffff)

	// projectiles that are on the way to our interception zone
	// (eg. nuke toward a repulsor, or missile toward a shield)
	std::map<int, CWeaponProjectile*> incomingProjectiles;

	float buildPercent;           // how far we have come on building current missile if stockpiling
	int numStockpiled;            // how many missiles we have stockpiled
	int numStockpileQued;         // how many weapons the user have added to our que

	int lastRequest;              // when the last script call was done
	int lastTargetRetry;          // when we last recalculated target selection

	CWeapon* slavedTo;            // use this weapon to choose target

	float maxForwardAngleDif;     // for onlyForward/!turret weapons, max. angle between owner->frontdir and (targetPos - owner->pos) (derived from UnitDefWeapon::maxAngleDif)
	float maxMainDirAngleDif;     // for !onlyForward/turret weapons, max. angle from <mainDir> the weapon can aim (derived from WeaponDef::tolerance)

	float heightBoostFactor;      // controls cannon range height boost. default: -1 -- automatically calculate a more or less sane value

	unsigned int avoidFlags;
	unsigned int collisionFlags;

	float3 relAimFromPos;         // aimFromPos relative to the unit
	float3 aimFromPos;            // absolute weapon pos
	float3 relWeaponMuzzlePos;    // position of the firepoint
	float3 weaponMuzzlePos;
	float3 weaponDir;
	float3 mainDir;               // main aiming-direction of weapon
	float3 wantedDir;             // norm(currentTargetPos - weaponMuzzlePos)
	float3 lastRequestedDir;      // last angle we called the script with

	float3 salvoError;            // error vector for the whole salvo
	float3 errorVector;
	float3 errorVectorAdd;

	float muzzleFlareSize;        // size of muzzle flare if drawn
	int fireSoundId;
	float fireSoundVolume;
};

#endif /* WEAPON_H */
