#ifndef WEAPON_H
#define WEAPON_H
// Weapon.h: interface for the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WEAPON_H__57851722_F7FA_4271_9479_800DB93A2180__INCLUDED_)
#define AFX_WEAPON_H__57851722_F7FA_4271_9479_800DB93A2180__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include "Object.h"
#include "DamageArray.h"
#include <list>

class CUnit;
class CWeaponProjectile;
struct WeaponDef;

enum TargetType {
	Target_None,
	Target_Unit,
	Target_Pos,
	Target_Intercept
};

class CWeapon : public CObject
{
public:
	CWeapon(CUnit* owner);
	virtual ~CWeapon();
	virtual void Init(void);

	void DependentDied(CObject* o);

	virtual bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
	virtual void SlowUpdate();
	virtual void Update();

	void HoldFire();
	bool AttackUnit(CUnit* unit,bool userTarget);
	bool AttackGround(float3 pos,bool userTarget);

	void AimReady(int value);

	virtual void Fire(){};								//should be implemented by subclasses
	void ScriptReady(void);

	CUnit* owner;

	WeaponDef *weaponDef;

	int weaponNum;							//the weapons order among the owner weapons
	bool haveUserTarget;

	DamageArray damages;
	float areaOfEffect;

	float3 relWeaponPos;				//weaponpos relative to the unit
	float3 weaponPos;						//absolute weapon pos
	float muzzleFlareSize;			//size of muzzle flare if drawn
	int useWeaponPosForAim;			//sometimes weapon pos is better to use than aimpos

	int reloadTime;							//time between succesive fires in ticks
	int reloadStatus;						//next tick the weapon can fire again

	float range;
	float heightMod;							//how much extra range the weapon gain per height difference

	float projectileSpeed;
	float accuracy;								//inaccuracy of whole salvo
	float sprayangle;							//inaccuracy of individual shots inside salvo

	int salvoDelay;								//delay between shots in a salvo
	int salvoSize;								//number of shots in a salvo
	int nextSalvo;								//when the next shot in the current salvo will fire
	int salvoLeft;								//number of shots left in current salvo
	float3 salvoError;						//error vector for the whole salvo

	TargetType targetType;				//indicated if we have a target and what type
	CUnit* targetUnit;						//the targeted unit if targettype=unit
	float3 targetPos;							//the position of the target (even if targettype=unit)
	int lastTargetRetry;					//when we last recalculated target selection

	float predict;								//how long time we predict it take for a projectile to reach target
	float predictSpeedMod;				//how the weapon predicts the speed of the units goes -> 1 when experience increases

	float metalFireCost;
	float energyFireCost;					//part of unit supply used to fire a salvo (transformed by unitloader)

	int fireSoundId;
	float fireSoundVolume;

	bool angleGood;										//set when script indicated ready to fire
	bool subClassReady;								//set to false if the subclassed weapon cant fire for some reason
	bool onlyForward;									//can only fire in the forward direction of the unit (for aircrafts mostly?)

	float maxAngleDif;								//max dotproduct between wanted and actual angle to fire
	float3 wantedDir;									//the angle we want to aim in,set by the weapon subclass
	float3 lastRequestedDir;					//last angle we called the script with
	int lastRequest;									//when the last script call was done

	unsigned int badTargetCategory;		//targets in this category get a lot lower targetting priority
	unsigned int onlyTargetCategory;	//only targets in this category can be targeted (default 0xffffffff)

	std::list<CWeaponProjectile*> incoming;	//nukes that are on the way to our area
	CWeaponProjectile* interceptTarget;				//nuke that we currently targets

	float buildPercent;								//how far we have come on building current missile if stockpiling
	int numStockpiled;								//how many missiles we have stockpiled
	int numStockpileQued;							//how many weapons the user have added to our que
	void CheckIntercept(void);

	float3 errorVector;
	float3 errorVectorAdd;
	int lastErrorVectorUpdate;

	CWeapon* slavedTo;
};

#endif // !defined(AFX_WEAPON_H__57851722_F7FA_4271_9479_800DB93A2180__INCLUDED_)


#endif /* WEAPON_H */
