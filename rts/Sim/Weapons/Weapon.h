/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_H
#define WEAPON_H

#include <map>

#include "System/Object.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "System/float3.h"

class CUnit;
struct WeaponDef;

enum TargetType {
	Target_None,
	Target_Unit,
	Target_Pos,
	Target_Intercept
};

class CWeapon : public CObject
{
	CR_DECLARE(CWeapon);
public:
	CWeapon(CUnit* owner);
	virtual ~CWeapon();
	virtual void Init();

	void SetWeaponNum(int);

	void DependentDied(CObject* o);


	bool CheckTargetAngleConstraint(const float3& worldTargetDir, const float3& worldWeaponDir) const;
	bool SetTargetBorderPos(CUnit*, float3&, float3&, float3&);
	bool GetTargetBorderPos(const CUnit*, const float3&, float3&, float3&) const;

	/// test if the weapon is able to attack an enemy/mapspot just by its properties (no range check, no FreeLineOfFire check, ...)
	virtual bool TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const;
	/// test if the enemy/mapspot is in range/angle
	bool TestRange(const float3& pos, bool userTarget, const CUnit* unit) const;
	/// test if something is blocking our LineOfFire
	virtual bool HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const;

	bool TryTarget(const float3& pos, bool userTarget, const CUnit* unit) const;
	bool TryTarget(CUnit* unit, bool userTarget);
	bool TryTargetRotate(CUnit* unit, bool userTarget);
	bool TryTargetRotate(float3 pos, bool userTarget);
	bool TryTargetHeading(short heading, float3 pos, bool userTarget, CUnit* unit = 0);

	bool CobBlockShot(const CUnit* unit);
	float TargetWeight(const CUnit* unit) const;
	void SlowUpdate(bool noAutoTargetOverride);

	virtual void SlowUpdate();
	virtual void Update();
	virtual float GetRange2D(float yDiff) const;
	virtual void UpdateRange(float val) { range = val; }

	virtual bool AttackUnit(CUnit* newTargetUnit, bool isUserTarget);
	virtual bool AttackGround(float3 newTargetPos, bool isUserTarget);

	void AutoTarget();
	void AimReady(int value);
	void Fire();
	void HoldFire();
	
	float ExperienceScale() const;
	float AccuracyExperience() const { return accuracy*ExperienceScale(); }
	float SprayAngleExperience() const { return sprayAngle*ExperienceScale(); }
	float3 SalvoErrorExperience() const { return salvoError*ExperienceScale(); }
	float MoveErrorExperience() const;

	void StopAttackingAllyTeam(int ally);
	void UpdateInterceptTarget();

protected:
	virtual void FireImpl() {}

	void UpdateTargeting();
	void UpdateFire();
	bool UpdateStockpile();
	void UpdateSalvo();

	static bool TargetUnitOrPositionInUnderWater(const float3& targetPos, const CUnit* targetUnit);
	static bool TargetUnitOrPositionInWater(const float3& targetPos, const CUnit* targetUnit);

protected:
	ProjectileParams GetProjectileParams();

private:
	inline bool AllowWeaponTargetCheck();
	void UpdateRelWeaponPos();

public:
	CUnit* owner;

	const WeaponDef* weaponDef;

	int weaponNum;							// the weapons order among the owner weapons
	bool haveUserTarget;

	float craterAreaOfEffect;
	float damageAreaOfEffect;

	float muzzleFlareSize;					// size of muzzle flare if drawn
	int useWeaponPosForAim;					// sometimes weapon pos is better to use than aimpos
	bool hasCloseTarget;					// might need to update weapon pos more often when enemy is near

	int reloadTime;							// time between succesive fires in ticks
	int reloadStatus;						// next tick the weapon can fire again

	float range;
	float heightMod;						// how much extra range the weapon gain per height difference

	float projectileSpeed;
	float accuracy;							// inaccuracy of whole salvo
	float sprayAngle;						// inaccuracy of individual shots inside salvo

	int salvoDelay;							// delay between shots in a salvo
	int salvoSize;							// number of shots in a salvo
	int projectilesPerShot;					// number of projectiles per shot
	int nextSalvo;							// when the next shot in the current salvo will fire
	int salvoLeft;							// number of shots left in current salvo

	TargetType targetType;					// indicated if we have a target and what type
	CUnit* targetUnit;						// the targeted unit if targettype=unit

	float predict;							// how long time we predict it take for a projectile to reach target
	float predictSpeedMod;					// how the weapon predicts the speed of the units goes -> 1 when experience increases

	float metalFireCost;
	float energyFireCost;

	int fireSoundId;
	float fireSoundVolume;

	bool hasBlockShot;						// set when the script has a BlockShot() function for this weapon
	bool hasTargetWeight;					// set when there's a TargetWeight() function for this weapon
	bool angleGood;							// set when script indicated ready to fire
	bool avoidTarget;						// set when the script wants the weapon to pick a new target, reset once one has been chosen
	bool subClassReady;						// set to false if the subclassed weapon cant fire for some reason
	bool onlyForward;						// can only fire in the forward direction of the unit (for aircrafts mostly?)

	unsigned int badTargetCategory;			// targets in this category get a lot lower targetting priority
	unsigned int onlyTargetCategory;		// only targets in this category can be targeted (default 0xffffffff)

	// projectiles that are on the way to our interception zone
	// (eg. nuke toward a repulsor, or missile toward a shield)
	std::map<int, CWeaponProjectile*> incomingProjectiles;
	// projectile that we currently target for interception
	CWeaponProjectile* interceptTarget;

	int stockpileTime;            // how long it takes to stockpile 1 missile
	float buildPercent;           // how far we have come on building current missile if stockpiling
	int numStockpiled;            // how many missiles we have stockpiled
	int numStockpileQued;         // how many weapons the user have added to our que

	int lastRequest;              // when the last script call was done
	int lastTargetRetry;          // when we last recalculated target selection
	int lastErrorVectorUpdate;

	CWeapon* slavedTo;            // use this weapon to choose target

	float maxForwardAngleDif;     // for onlyForward/!turret weapons, max. angle between owner->frontdir and (targetPos - owner->pos) (derived from UnitDefWeapon::maxAngleDif)
	float maxMainDirAngleDif;     // for !onlyForward/turret weapons, max. angle from <mainDir> the weapon can aim (derived from WeaponDef::tolerance)

	float targetBorder;           // if nonzero, units will TryTarget wrt. edge of scaled collision volume instead of centre
	float cylinderTargeting;      // if greater than 0, range will be checked in a cylinder (height=range*cylinderTargeting) instead of a sphere
	float minIntensity;           // for beamlasers - always hit with some minimum intensity (a damage coeffcient normally dependent on distance). do not confuse with intensity tag, it's completely unrelated.
	float heightBoostFactor;      // controls cannon range height boost. default: -1 -- automatically calculate a more or less sane value

	unsigned int avoidFlags;
	unsigned int collisionFlags;

	float fuelUsage;

	float3 relWeaponPos;          // weaponpos relative to the unit
	float3 weaponPos;             // absolute weapon pos
	float3 relWeaponMuzzlePos;    // position of the firepoint
	float3 weaponMuzzlePos;
	float3 weaponDir;
	float3 mainDir;               // main aiming-direction of weapon
	float3 wantedDir;             // the angle we want to aim in, set by the weapon subclass
	float3 lastRequestedDir;      // last angle we called the script with
	float3 salvoError;            // error vector for the whole salvo
	float3 errorVector;
	float3 errorVectorAdd;

	float3 targetPos;             // the position of the target (even if targettype=unit)
	float3 targetBorderPos;       // <targetPos> adjusted for target-border factor
};

#endif /* WEAPON_H */
