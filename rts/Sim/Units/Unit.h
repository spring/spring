/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_H
#define UNIT_H

#include <vector>
#include <string>

#include "Sim/Objects/SolidObject.h"
#include "Sim/Misc/Resource.h"
#include "Sim/Weapons/WeaponTarget.h"
#include "System/Matrix44f.h"
#include "System/type2.h"


class CPlayer;
class CCommandAI;
class CGroup;
class CMissileProjectile;
class AMoveType;
class CWeapon;
class CUnitScript;
class DamageArray;
class DynDamageArray;
struct SolidObjectDef;
struct UnitDef;
struct UnitTrackStruct;
struct UnitLoadParams;
struct SLosInstance;


namespace icon {
	class CIconData;
}


// LOS state bits
#define LOS_INLOS      (1 << 0)  // the unit is currently in the los of the allyteam
#define LOS_INRADAR    (1 << 1)  // the unit is currently in radar from the allyteam
#define LOS_PREVLOS    (1 << 2)  // the unit has previously been in los from the allyteam
#define LOS_CONTRADAR  (1 << 3)  // the unit has continuously been in radar since it was last inlos by the allyteam

// LOS mask bits  (masked bits are not automatically updated)
#define LOS_INLOS_MASK     (1 << 8)   // do not update LOS_INLOS
#define LOS_INRADAR_MASK   (1 << 9)   // do not update LOS_INRADAR
#define LOS_PREVLOS_MASK   (1 << 10)  // do not update LOS_PREVLOS
#define LOS_CONTRADAR_MASK (1 << 11)  // do not update LOS_CONTRADAR

#define LOS_ALL_MASK_BITS \
	(LOS_INLOS_MASK | LOS_INRADAR_MASK | LOS_PREVLOS_MASK | LOS_CONTRADAR_MASK)

enum ScriptCloakBits { // FIXME -- not implemented
	// always set to 0 if not enabled
	SCRIPT_CLOAK_ENABLED          = (1 << 0),
	SCRIPT_CLOAK_IGNORE_ENERGY    = (1 << 1),
	SCRIPT_CLOAK_IGNORE_STUNNED   = (1 << 2),
	SCRIPT_CLOAK_IGNORE_PROXIMITY = (1 << 3),
	SCRIPT_CLOAK_IGNORE_BUILDING  = (1 << 4),
	SCRIPT_CLOAK_IGNORE_RECLAIM   = (1 << 5),
	SCRIPT_CLOAK_IGNORE_CAPTURING = (1 << 6),
	SCRIPT_CLOAK_IGNORE_TERRAFORM = (1 << 7)
};





class CUnit : public CSolidObject
{
public:
	CR_DECLARE(CUnit)

	CUnit();
	virtual ~CUnit();

	static void InitStatic();

	virtual void PreInit(const UnitLoadParams& params);
	virtual void PostInit(const CUnit* builder);

	virtual void SlowUpdate();
	virtual void SlowUpdateWeapons();
	virtual void Update();

	const SolidObjectDef* GetDef() const { return ((const SolidObjectDef*) unitDef); }

	virtual void DoDamage(const DamageArray& damages, const float3& impulse, CUnit* attacker, int weaponDefID, int projectileID);
	virtual void DoWaterDamage();
	virtual void FinishedBuilding(bool postInit);

	void ApplyImpulse(const float3& impulse);

	bool AttackUnit(CUnit* unit, bool isUserTarget, bool wantManualFire, bool fpsMode = false);
	bool AttackGround(const float3& pos, bool isUserTarget, bool wantManualFire, bool fpsMode = false);
	void DropCurrentAttackTarget();

	int GetBlockingMapID() const { return id; }

	void ChangeLos(int losRad, int airRad);
	/// negative amount=reclaim, return= true -> build power was successfully applied
	bool AddBuildPower(CUnit* builder, float amount);
	/// turn the unit on
	void Activate();
	/// turn the unit off
	void Deactivate();

	void ForcedMove(const float3& newPos);

	void DeleteScript();
	void EnableScriptMoveType();
	void DisableScriptMoveType();

	CMatrix44f GetTransformMatrix(const bool synced = false) const final;

	void DependentDied(CObject* o);

	bool AllowedReclaim(CUnit* builder) const;

	void SetMetalStorage(float newStorage);
	void SetEnergyStorage(float newStorage);

	bool UseMetal(float metal);
	void AddMetal(float metal, bool useIncomeMultiplier = true);
	bool UseEnergy(float energy);
	void AddEnergy(float energy, bool useIncomeMultiplier = true);
	bool AddHarvestedMetal(float metal);

	void SetStorage(const SResourcePack& newstorage);
	bool HaveResources(const SResourcePack& res) const;
	bool UseResources(const SResourcePack& res);
	void AddResources(const SResourcePack& res, bool useIncomeMultiplier = true);
	bool IssueResourceOrder(SResourceOrder* order);

	/// push the new wind to the script
	void UpdateWind(float x, float z, float strength);

	void AddExperience(float exp);

	void SetMass(float newMass);

	void DoSeismicPing(float pingSize);

	void CalculateTerrainType();
	void UpdateTerrainType();
	void UpdatePhysicalState(float eps);

	float3 GetErrorVector(int allyteam) const;
	float3 GetErrorPos(int allyteam, bool aiming = false) const { return (aiming? aimPos: midPos) + GetErrorVector(allyteam); }
	float3 GetObjDrawErrorPos(int allyteam) const { return (GetObjDrawMidPos() + GetErrorVector(allyteam)); }

	float3 GetLuaErrorVector(int allyteam, bool fullRead) const { return (fullRead? ZeroVector: GetErrorVector(allyteam)); }
	float3 GetLuaErrorPos(int allyteam, bool fullRead) const { return (midPos + GetLuaErrorVector(allyteam, fullRead)); }

	void UpdatePosErrorParams(bool updateError, bool updateDelta);

	bool UsingScriptMoveType() const { return (prevMoveType != NULL); }
	bool UnderFirstPersonControl() const { return (fpsControlPlayer != NULL); }

	bool IsNeutral() const { return neutral; }
	bool IsCloaked() const { return isCloaked; }
	bool IsIdle() const;

	bool CanUpdateWeapons() const;

	void SetStunned(bool stun);
	bool IsStunned() const { return stunned; }

	void SetLosStatus(int allyTeam, unsigned short newStatus);
	unsigned short CalcLosStatus(int allyTeam);
	void UpdateLosStatus(int allyTeam);

	void SlowUpdateCloak(bool);
	void ScriptDecloak(bool);
	bool GetNewCloakState(bool checkStun);

	enum ChangeType {
		ChangeGiven,
		ChangeCaptured
	};
	virtual bool ChangeTeam(int team, ChangeType type);
	virtual void StopAttackingAllyTeam(int ally);

	//Transporter stuff
	CR_DECLARE_SUB(TransportedUnit)

	struct TransportedUnit {
		CR_DECLARE_STRUCT(TransportedUnit)
		CUnit* unit;
		int piece;
	};

	void SetLastAttacker(CUnit* attacker);

	void SetTransporter(CUnit* trans) { transporter = trans; }
	inline CUnit* GetTransporter() const { return transporter; }

	bool AttachUnit(CUnit* unit, int piece, bool force = false);
	bool CanTransport(const CUnit* unit) const;

	bool DetachUnit(CUnit* unit);
	bool DetachUnitCore(CUnit* unit);
	bool DetachUnitFromAir(CUnit* unit, const float3& pos); ///< moves to position after

	bool CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit, float* wantedHeightPtr = NULL) const;
	float GetTransporteeWantedHeight(const float3& wantedPos, const CUnit* unit, bool* ok = NULL) const;
	short GetTransporteeWantedHeading(const CUnit* unit) const;

public:
	void ForcedKillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool showDeathSequence = true);
	virtual void KillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool showDeathSequence = true);
	virtual void IncomingMissile(CMissileProjectile* missile);

	void TempHoldFire(int cmdID);
	void ReleaseTempHoldFire() { dontFire = false; }
	bool HaveTarget() const;

	/// start this unit in free fall from parent unit
	void Drop(const float3& parentPos, const float3& parentDir, CUnit* parent);
	void PostLoad();

protected:
	void ChangeTeamReset();
	void UpdateResources();
	float GetFlankingDamageBonus(const float3& attackDir);

public: // unsynced methods
	bool SetGroup(CGroup* newGroup, bool fromFactory = false);

public:
	static void  SetExpMultiplier(float value) { expMultiplier = value; }
	static void  SetExpPowerScale(float value) { expPowerScale = value; }
	static void  SetExpHealthScale(float value) { expHealthScale = value; }
	static void  SetExpReloadScale(float value) { expReloadScale = value; }
	static void  SetExpGrade(float value) { expGrade = value; }

	static float GetExpMultiplier() { return expMultiplier; }
	static float GetExpPowerScale() { return expPowerScale; }
	static float GetExpHealthScale() { return expHealthScale; }
	static float GetExpReloadScale() { return expReloadScale; }
	static float GetExpGrade() { return expGrade; }

	static float ExperienceScale(const float limExperience, const float experienceWeight) {
		// limExperience ranges from 0.0 to 0.9999..., experienceWeight
		// should be in [0, 1] and have no effect on accuracy when zero
		return (1.0f - (limExperience * experienceWeight));
	}

	static void SetSpawnFeature(bool b) { spawnFeature = b; }

public:
	const UnitDef* unitDef;

	std::vector<CWeapon*> weapons;

	/// Our shield weapon, NULL if we have none
	CWeapon* shieldWeapon;
	/// Our weapon with stockpiled ammo, NULL if we have none
	CWeapon* stockpileWeapon;

	const DynDamageArray* selfdExpDamages;
	const DynDamageArray* deathExpDamages;

	CUnit* soloBuilder;
	/// last attacker
	CUnit* lastAttacker;

	/// last frame unit was attacked by other unit
	int lastAttackFrame;
	/// last time this unit fired a weapon
	int lastFireWeapon;

	/// current attackee
	SWeaponTarget curTarget;

	/// transport that the unit is currently in
	CUnit* transporter;

	//Transporter stuff
	int transportCapacityUsed;
	float transportMassUsed;
	std::vector<TransportedUnit> transportedUnits;

	AMoveType* moveType;
	AMoveType* prevMoveType;

	CCommandAI* commandAI;
	CUnitScript* script;

	/// which squares the unit can currently observe
	std::vector<SLosInstance*> los;

	/// indicate the los/radar status the allyteam has on this unit
	std::vector<unsigned short> losStatus;

	/// player who is currently FPS'ing this unit
	CPlayer* fpsControlPlayer;

	/// quads the unit is part of
	std::vector<int> quads;

	std::vector<CMissileProjectile*> incomingMissiles; //FIXME make std::set?

	float3 deathSpeed;
	float3 lastMuzzleFlameDir;

	/// units takes less damage when attacked from this dir (encourage flanking fire)
	float3 flankingBonusDir;

	/// used for innacuracy with radars etc
	float3 posErrorVector;
	float3 posErrorDelta;

	int featureDefID; // FeatureDef id of the wreck we spawn on death

	/// indicate the relative power of the unit, used for experience calulations etc
	float power;

	/// 0.0-1.0
	float buildProgress;

	/// if health-this is negative the unit is stunned
	float paralyzeDamage;
	/// how close this unit is to being captured
	float captureProgress;
	float experience;
	/// goes ->1 as experience go -> infinite
	float limExperience;

	/**
	 * neutral allegiance, will not be automatically
	 * fired upon unless the fireState is set to >
	 * FIRESTATE_FIREATWILL
	 */
	bool neutral;
	/// is in built (:= nanoframe)
	bool beingBuilt;
	/// if the updir is straight up or align to the ground vector
	bool upright;
	/// whether the ground below this unit has been terraformed
	bool groundLevelled;
	/// how much terraforming is left to do
	float terraformLeft;

	/// if we arent built on for a while start decaying
	int lastNanoAdd;
	int lastFlareDrop;

	/// How much reapir power has been added to this recently
	float repairAmount;

	/// id of transport that the unit is about to be picked up by
	int loadingTransportId;
	int unloadingTransportId;


	/// used by constructing units
	bool inBuildStance;
	/// tells weapons that support it to try to use a high trajectory
	bool useHighTrajectory;

	/// used by landed gunships to block weapon Update()'s
	bool dontUseWeapons;
	/// used by builders to prevent weapon SlowUpdate()'s and Attack{Unit,Ground}()'s
	bool dontFire;

	/// the script has finished exectuting the killed function and the unit can be deleted
	bool deathScriptFinished;
	/// the wreck level the unit will eventually create when it has died
	int delayedWreckLevel;

	/// how long the unit has been inactive
	unsigned int restTime;
	unsigned int outOfMapTime;

	float reloadSpeed;
	float maxRange;

	/// used to determine muzzle flare size
	float lastMuzzleFlameSize;

	int armorType;
	/// what categories the unit is part of (bitfield)
	unsigned int category;

	int mapSquare;

	/// set los to this when finished building
	int realLosRadius;
	int realAirLosRadius;

	int losRadius;
	int airLosRadius;

	int radarRadius;
	int sonarRadius;
	int jammerRadius;
	int sonarJamRadius;
	int seismicRadius;
	float seismicSignature;
	bool stealth;
	bool sonarStealth;

	/// only when the unit is active
	SResourcePack resourcesCondUse;
	SResourcePack resourcesCondMake;

	/// always applied
	SResourcePack resourcesUncondUse;
	SResourcePack resourcesUncondMake;

	/// costs per UNIT_SLOWUPDATE_RATE frames
	SResourcePack resourcesUse;

	/// incomes per UNIT_SLOWUPDATE_RATE frames
	SResourcePack resourcesMake;

	/// variables used for calculating unit resource usage
	SResourcePack resourcesUseI;
	SResourcePack resourcesMakeI;
	SResourcePack resourcesUseOld;
	SResourcePack resourcesMakeOld;

	/// energy added each halftick
	float energyTickMake; //FIXME???

	/// how much metal the unit currently extracts from the ground
	float metalExtract;

	/// the amount of storage the unit contributes to the team
	SResourcePack storage;

	/// per unit metal storage (gets filled on reclaim and needs then to be unloaded at some storage building -> 2nd part is lua's job)
	SResourcePack harvestStorage;
	SResourcePack harvested;

	SResourcePack cost;
	float buildTime;

	/// decaying value of how much damage the unit has taken recently (for severity of death)
	float recentDamage;

	int fireState;
	int moveState;

	/// if the unit is in it's 'on'-state
	bool activated;
	/// prevent damage from hitting an already dead unit (causing multi wreck etc)
	bool isDead;

	/// for units being dropped from transports (parachute drops)
	float fallSpeed;

	/// total distance the unit has moved
	float travel;
	/// 0.0f disables travel accumulation
	float travelPeriod;

	/**
	 * 0 = no flanking bonus
	 * 1 = global coords, mobile
	 * 2 = unit coords, mobile
	 * 3 = unit coords, locked
	 */
	int flankingBonusMode;
	/// how much the lowest damage direction of the flanking bonus can turn upon an attack (zeroed when attacked, slowly increases)
	float  flankingBonusMobility;
	/// how much ability of the flanking bonus direction to move builds up each frame
	float  flankingBonusMobilityAdd;
	/// average factor to multiply damage by
	float  flankingBonusAvgDamage;
	/// (max damage - min damage) / 2
	float  flankingBonusDifDamage;

	bool armoredState;
	float armoredMultiple;
	/// multiply all damage the unit take with this
	float curArmorMultiple;

	int nextPosErrorUpdate;

	///true if the unit is currently cloaked (has enough energy etc)
	bool isCloaked;
	/// true if the unit currently wants to be cloaked
	bool wantCloak;
	/// true if a script currently wants the unit to be cloaked
	int scriptCloak;
	/// the minimum time between decloaking and cloaking again
	int cloakTimeout;
	/// the earliest frame the unit can cloak again
	int curCloakTimeout;
	float decloakDistance;

	int lastTerrainType;
	/// Used for calling setSFXoccupy which TA scripts want
	int curTerrainType;

	int selfDCountdown;

	/// the damage value passed to CEGs spawned by this unit's script
	int cegDamage;


	// unsynced vars
	bool noMinimap;
	bool leaveTracks;

	bool isSelected;
	bool isIcon;
	float iconRadius;

	unsigned int lastUnitUpdate;

	std::string tooltip;

	CGroup* group;

	UnitTrackStruct* myTrack;
	icon::CIconData* myIcon;

private:
	/// if we are stunned by a weapon or for other reason, access via IsStunned/SetStunned(bool)
	bool stunned;

	static float empDeclineRate;
	static float expMultiplier;
	static float expPowerScale;
	static float expHealthScale;
	static float expReloadScale;
	static float expGrade;

	static bool spawnFeature;
};

#endif // UNIT_H
