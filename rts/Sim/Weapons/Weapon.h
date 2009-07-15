#ifndef WEAPON_H
#define WEAPON_H
// Weapon.h: interface for the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include <list>

#include "Object.h"
#include "Sim/Misc/DamageArray.h"
#include "float3.h"

class CUnit;
class CWeaponProjectile;
struct WeaponDef;
class UnitDefWeapon;

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
	virtual void Init(void);

	void SetWeaponNum(int);

	void DependentDied(CObject* o);

	virtual bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
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

	void HoldFire();
	virtual bool AttackUnit(CUnit* unit, bool userTarget);
	virtual bool AttackGround(float3 pos, bool userTarget);

	void AimReady(int value);

	bool ShouldCheckForNewTarget() const;

	void Fire();

	void StopAttackingAllyTeam(int ally);

	CUnit* owner;

	const WeaponDef *weaponDef;
	std::string modelDispList;

	int weaponNum;							// the weapons order among the owner weapons
	bool haveUserTarget;

	float areaOfEffect;

	float3 relWeaponPos;					// weaponpos relative to the unit
	float3 weaponPos;						// absolute weapon pos

	float3 relWeaponMuzzlePos;				// position of the firepoint
	float3 weaponMuzzlePos;
	float3 weaponDir;

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
	float3 salvoError;						// error vector for the whole salvo

	TargetType targetType;					// indicated if we have a target and what type
	CUnit* targetUnit;						// the targeted unit if targettype=unit
	float3 targetPos;						// the position of the target (even if targettype=unit)
	int lastTargetRetry;					// when we last recalculated target selection

	float predict;							// how long time we predict it take for a projectile to reach target
	float predictSpeedMod;					// how the weapon predicts the speed of the units goes -> 1 when experience increases

	float metalFireCost;
	float energyFireCost;					// part of unit supply used to fire a salvo (transformed by unitloader)

	int fireSoundId;
	float fireSoundVolume;

	bool cobHasBlockShot;					// set when the script has a BlockShot() function for this weapon
	bool hasTargetWeight;					// set when there's a TargetWeight() function for this weapon
	bool angleGood;							// set when script indicated ready to fire
	bool avoidTarget;						// set when the script wants the weapon to pick a new target, reset once one has been chosen
	bool subClassReady;						// set to false if the subclassed weapon cant fire for some reason
	bool onlyForward;						// can only fire in the forward direction of the unit (for aircrafts mostly?)

	float maxAngleDif;						// max dotproduct between wanted and actual angle to fire
	float3 wantedDir;						// the angle we want to aim in,set by the weapon subclass
	float3 lastRequestedDir;				// last angle we called the script with
	int lastRequest;						// when the last script call was done

	unsigned int badTargetCategory;			// targets in this category get a lot lower targetting priority
	unsigned int onlyTargetCategory;		// only targets in this category can be targeted (default 0xffffffff)

	std::list<CWeaponProjectile*> incoming;	// nukes that are on the way to our area
	CWeaponProjectile* interceptTarget;		// nuke that we currently targets

	int stockpileTime;						// how long it takes to stockpile 1 missile
	float buildPercent;						// how far we have come on building current missile if stockpiling
	int numStockpiled;						// how many missiles we have stockpiled
	int numStockpileQued;					// how many weapons the user have added to our que
	void CheckIntercept(void);

	float3 errorVector;
	float3 errorVectorAdd;
	int lastErrorVectorUpdate;

	CWeapon* slavedTo;						// use this weapon to choose target

	float3 mainDir;							// main aim dir of weapon
	float maxMainDirAngleDif;				// how far away from main aim dir the weapon can aim at something (as an acos value)

	bool avoidFriendly;						// if true, try to avoid friendly units while aiming
	bool avoidFeature;      				// if true, try to avoid features while aiming
	bool avoidNeutral;						// if true, try to avoid neutral units while aiming

	float targetBorder;						// if nonzero, targetting units will TryTarget at the edge of collision sphere (radius*tag value, [-1;1]) instead of its centre
	float cylinderTargetting;				// if greater than 0, range will be checked in a cylinder (height=range*cylinderTargetting) instead of a sphere
	float minIntensity;						// for beamlasers - always hit with some minimum intensity (a damage coeffcient normally dependent on distance). do not confuse with intensity tag, it's completely unrelated.
	float heightBoostFactor;				// controls cannon range height boost. default: -1 -- automatically calculate a more or less sane value

	unsigned int collisionFlags;

	float fuelUsage;

private:
	virtual void FireImpl() {};
};

#endif /* WEAPON_H */
