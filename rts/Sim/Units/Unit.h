/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_H
#define UNIT_H

#include <map>
#include <vector>
#include <string>

#include "Lua/LuaRulesParams.h"
#include "Lua/LuaUnitMaterial.h"
#include "Sim/Objects/SolidObject.h"
#include "Matrix44f.h"
#include "Vec2.h"

class CCommandAI;
class CGroup;
class CLoadSaveInterface;
class CMissileProjectile;
class AMoveType;
class CUnitAI;
class CWeapon;
class CUnitScript;
struct DamageArray;
struct LosInstance;
struct LocalModel;
struct LocalModelPiece;
struct UnitDef;
struct UnitTrackStruct;
struct CollisionVolume;
struct DirectControlStruct;

class CTransportUnit;


// LOS state bits
#define LOS_INLOS      (1 << 0)  // the unit is currently in the los of the allyteam
#define LOS_INRADAR    (1 << 1)  // the unit is currently in radar from the allyteam
#define LOS_PREVLOS    (1 << 2)  // the unit has previously been in los from the allyteam
#define LOS_CONTRADAR  (1 << 3)  // the unit has continously been in radar since it was last inlos by the allyteam

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

	virtual void UnitInit(const UnitDef* def, int team, const float3& position);

	bool AttackGround(const float3& pos, bool dgun);
	bool AttackUnit(CUnit* unit, bool dgun);

	virtual void DoDamage(const DamageArray& damages, CUnit* attacker,
	                      const float3& impulse, int weaponId = -1);
	virtual void DoWaterDamage();
	virtual void Kill(const float3& impulse);
	virtual void FinishedBuilding();

	int GetBlockingMapID() const { return id; }

	void ChangeLos(int l, int airlos);
	void ChangeSensorRadius(int* valuePtr, int newValue);
	/// negative amount=reclaim, return= true -> build power was successfully applied
	bool AddBuildPower(float amount, CUnit* builder);
	/// turn the unit on
	void Activate();
	/// turn the unit off
	void Deactivate();

	void ForcedMove(const float3& newPos);
	void ForcedSpin(const float3& newDir);
	void SetFront(const SyncedFloat3& newDir);
	void SetUp(const SyncedFloat3& newDir);
	void SetRight(const SyncedFloat3& newDir);
	void EnableScriptMoveType();
	void DisableScriptMoveType();

	///// order unit to move to position
	//void SetGoal(float3 pos);

	virtual void SlowUpdate();
	virtual void SlowUpdateWeapons();
	virtual void Update();

	void SetDirectionFromHeading();

	void ApplyTransformMatrix() const;
	CMatrix44f GetTransformMatrix(const bool synced = false, const bool error = false) const;

	void SetLastAttacker(CUnit* attacker);
	void SetLastAttackedPiece(LocalModelPiece* p, int f) {
		lastAttackedPiece      = p;
		lastAttackedPieceFrame = f;
	}

	void DependentDied(CObject* o);
	enum DependenceType { DEPENDENCE_ATTACKER, DEPENDENCE_BUILD, DEPENDENCE_BUILDER, DEPENDENCE_CAPTURE, DEPENDENCE_RECLAIM, 
						DEPENDENCE_RESURRECT, DEPENDENCE_TARGET, DEPENDENCE_TERRAFORM, DEPENDENCE_TRANSPORTEE, DEPENDENCE_TRANSPORTER };
	virtual void DeleteDeathDependence(CObject* o, DependenceType dep);

	void SetUserTarget(CUnit* target);
	virtual void Init(const CUnit* builder);
	bool SetGroup(CGroup* group);

	bool AllowedReclaim(CUnit* builder) const;
	bool UseMetal(float metal);
	void AddMetal(float metal, bool useIncomeMultiplier = true);
	bool UseEnergy(float energy);
	void AddEnergy(float energy, bool useIncomeMultiplier = true);
	/// push the new wind to the script
	void UpdateWind(float x, float z, float strength);
	void SetMetalStorage(float newStorage);
	void SetEnergyStorage(float newStorage);

	void AddExperience(float exp);

	void DoSeismicPing(float pingSize);

	void CalculateTerrainType();
	void UpdateTerrainType();

	void UpdateMidPos();

	bool IsNeutral() const {
		return neutral;
	}

	void SetLosStatus(int allyTeam, unsigned short newStatus);
	unsigned short CalcLosStatus(int allyTeam);

	void SlowUpdateCloak(bool);
	void ScriptDecloak(bool);

	enum ChangeType{
		ChangeGiven,
		ChangeCaptured
	};
	virtual bool ChangeTeam(int team, ChangeType type);
	virtual void StopAttackingAllyTeam(int ally);

	const UnitDef* unitDef;
	CollisionVolume* collisionVolume;
	std::string unitDefName;

	/**
	 * @brief mod controlled parameters
	 * This is a set of parameters that is initialized
	 * in CreateUnitRulesParams() and may change during the game.
	 * Each parameter is uniquely identified only by its id
	 * (which is the index in the vector).
	 * Parameters may or may not have a name.
	 */
	LuaRulesParams::Params  modParams;
	LuaRulesParams::HashMap modParamsMap; ///< name map for mod parameters

	/// tells the units main function to the ai, eg "builder"
	int aihint;

	/// the forward direction of the unit
	SyncedFloat3 frontdir;
	SyncedFloat3 rightdir;
	SyncedFloat3 updir;
	/// if the updir is straight up or align to the ground vector
	bool upright;
	/// = (midPos - pos)
	SyncedFloat3 relMidPos;

	float3 deathSpeed;

	/// total distance the unit has moved
	float travel;
	/// 0.0f disables travel accumulation
	float travelPeriod;

	/// indicate the relative power of the unit, used for experience calulations etc
	float power;

	float maxHealth;
	float health;
	/// if health-this is negative the unit is stunned
	float paralyzeDamage;
	/// how close this unit is to being captured
	float captureProgress;
	float experience;
	/// goes ->1 as experience go -> infinite
	float limExperience;

	/**
	 * neutral allegiance, will not be automatically
	 * fired upon unless the fireState is set to >= 3
	 */
	bool neutral;

	CUnit* soloBuilder;
	bool beingBuilt;
	/// if we arent built on for a while start decaying
	int lastNanoAdd;
	/// How much reapir power has been added to this recently
	float repairAmount;
	/// transport that the unit is currently in
	CTransportUnit* transporter;
	/// unit is about to be picked up by a transport
	bool toBeTransported;
	/// 0.0-1.0
	float buildProgress;
	/// whether the ground below this unit has been terraformed
	bool groundLevelled;
	/// how much terraforming is left to do
	float terraformLeft;
	/// set los to this when finished building
	int realLosRadius;
	int realAirLosRadius;

	/// indicate the los/radar status the allyteam has on this unit
	std::vector<unsigned short> losStatus;

	/// used by constructing units
	bool inBuildStance;
	/// if we are stunned by a weapon or for other reason
	bool stunned;
	/// tells weapons that support it to try to use a high trajectory
	bool useHighTrajectory;
	/// used by landed aircrafts for now
	bool dontUseWeapons;

	/// the script has finished exectuting the killed function and the unit can be deleted
	bool deathScriptFinished;
	/// the wreck level the unit will eventually create when it has died
	int delayedWreckLevel;

	/// how long the unit has been inactive
	int restTime;

	std::vector<CWeapon*> weapons;
	/// Our shield weapon, or NULL, if we have none
	CWeapon* shieldWeapon;
	/// Our weapon with stockpiled ammo, or NULL, if we have none
	CWeapon* stockpileWeapon;
	float reloadSpeed;
	float maxRange;
	bool haveTarget;
	bool haveUserTarget;
	bool haveDGunRequest;
	/// used to determine muzzle flare size
	float lastMuzzleFlameSize;
	float3 lastMuzzleFlameDir;

	int armorType;
	/// what categories the unit is part of (bitfield)
	unsigned int category;

	/// quads the unit is part of
	std::vector<int> quads;
	/// which squares the unit can currently observe
	LosInstance* los;

	/// used to see if something has operated on the unit before
	int tempNum;
	int lastSlowUpdate;

	int mapSquare;

	int losRadius;
	int airLosRadius;
	float losHeight;
	int lastLosUpdate;

	int radarRadius;
	int sonarRadius;
	int jammerRadius;
	int sonarJamRadius;
	int seismicRadius;
	float seismicSignature;
	bool hasRadarCapacity;
	std::vector<int> radarSquares;
	int2 oldRadarPos;
	bool hasRadarPos;
	bool stealth;
	bool sonarStealth;

	AMoveType* moveType;
	AMoveType* prevMoveType;
	bool usingScriptMoveType;

	CCommandAI* commandAI;
	/// if the unit is part of an group (hotkey group)
	CGroup* group;

	// only when the unit is active
	float condUseMetal;
	float condUseEnergy;
	float condMakeMetal;
	float condMakeEnergy;
	// always applied
	float uncondUseMetal;
	float uncondUseEnergy;
	float uncondMakeMetal;
	float uncondMakeEnergy;

	/// cost per 16 frames
	float metalUse;
	/// cost per 16 frames
	float energyUse;
	/// metal income generated by unit
	float metalMake;
	/// energy income generated by unit
	float energyMake;

	// variables used for calculating unit resource usage
	float metalUseI;
	float energyUseI;
	float metalMakeI;
	float energyMakeI;
	float metalUseold;
	float energyUseold;
	float metalMakeold;
	float energyMakeold;
	/// energy added each halftick
	float energyTickMake;

	/// how much metal the unit currently extracts from the ground
	float metalExtract;

	float metalCost;
	float energyCost;
	float buildTime;

	float metalStorage;
	float energyStorage;

	/// last attacker
	CUnit* lastAttacker;
	/// piece that was last hit by a projectile
	LocalModelPiece* lastAttackedPiece;
	/// frame in which lastAttackedPiece was hit
	int lastAttackedPieceFrame;
	/// last frame unit was attacked by other unit
	int lastAttack;
	/// last time this unit fired a weapon
	int lastFireWeapon;
	/// decaying value of how much damage the unit has taken recently (for severity of death)
	float recentDamage;

	CUnit* userTarget;
	float3 userAttackPos;
	bool userAttackGround;
	/// number of shots due to the latest command
	int commandShotCount;

	/// 0=hold fire,1=return,2=fire at will
	int fireState;
	/// temp variable that can be set when building etc to stop units to turn away to fire
	bool dontFire;
	/// 0=hold pos,1=maneuvre,2=roam
	int moveState;

	/// if the unit is in it's 'on'-state
	bool activated;

	inline CTransportUnit* GetTransporter() const {
#if defined(USE_GML) && GML_ENABLE_SIM
		// transporter may suddenly be changed to NULL by sim
		return *(CTransportUnit * volatile *)&transporter;
#else
		return transporter;
#endif
	}

#if defined(USE_GML) && GML_ENABLE_SIM
	unsigned lastUnitUpdate;
#endif

	LocalModel* localmodel;
	CUnitScript* script;

	std::string tooltip;

	bool crashing;
	/// prevent damage from hitting an already dead unit (causing multi wreck etc)
	bool isDead;
	/// for units being dropped from transports (parachute drops)
	bool	falling;
	float	fallSpeed;

	bool inAir;
	bool inWater;

	/**
	 * 0 = no flanking bonus
	 * 1 = global coords, mobile
	 * 2 = unit coords, mobile
	 * 3 = unit coords, locked
	 */
	int flankingBonusMode;
	/// units takes less damage when attacked from this dir (encourage flanking fire)
	float3 flankingBonusDir;
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

	std::string wreckName;

	/// used for innacuracy with radars etc
	float3 posErrorVector;
	float3 posErrorDelta;
	int	nextPosErrorUpdate;

	/// true if the unit has weapons that can fire at underwater targets
	bool hasUWWeapons;

	/// true if the unit currently wants to be cloaked
	bool wantCloak;
	/// true if a script currently wants the unit to be cloaked
	int scriptCloak;
	/// the minimum time between decloaking and cloaking again
	int cloakTimeout;
	/// the earliest frame the unit can cloak again
	int curCloakTimeout;
	///true if the unit is currently cloaked (has enough energy etc)
	bool isCloaked;
	float decloakDistance;

	int lastTerrainType;
	/// Used for calling setSFXoccupy which TA scripts want
	int curTerrainType;

	int selfDCountdown;
	DirectControlStruct* directControl;

	UnitTrackStruct* myTrack;

	std::list<CMissileProjectile*> incomingMissiles;
	int lastFlareDrop;

	float currentFuel;

	// 4 unsynced vars
	bool luaDraw;
	bool noDraw;
	bool noSelect;
	bool noMinimap;

	bool isIcon;
	float iconRadius;

	/// max speed of the unit
	float maxSpeed;
	/// max reverse speed (used only by ground units for now)
	float maxReverseSpeed;

	// unsynced calls
	void SetLODCount(unsigned int count);
	unsigned int CalcLOD(unsigned int lastLOD) const;
	unsigned int CalcShadowLOD(unsigned int lastLOD) const;
	// unsynced data
	unsigned int lodCount;
	unsigned int currentLOD;
	/// length-per-pixel
	std::vector<float> lodLengths;
	LuaUnitMaterial luaMats[LUAMAT_TYPE_COUNT];

	/// minimum alpha value for a texel to be drawn
	float alphaThreshold;
	/// the damage value passed to CEGs spawned by this unit's script
	int cegDamage;
#ifdef USE_GML
	/// last draw frame
	int lastDrawFrame;
#endif
protected:
	void ChangeTeamReset();
	void UpdateResources();
	void UpdateLosStatus(int allyTeam);

public:
	virtual void KillUnit(bool SelfDestruct, bool reclaimed, CUnit* attacker, bool showDeathSequence = true);
	virtual void LoadSave(CLoadSaveInterface* file, bool loading);
	virtual void IncomingMissile(CMissileProjectile* missile);
	void TempHoldFire();
	void ReleaseTempHoldFire();
	/// start this unit in free fall from parent unit
	void Drop(const float3& parentPos, const float3& parentDir, CUnit* parent);
	void PostLoad();

public:
	static void SetLODFactor(float value);

	static void  SetExpMultiplier(float value) { expMultiplier = value; }
	static float GetExpMultiplier()     { return expMultiplier; }
	static void  SetExpPowerScale(float value) { expPowerScale = value; }
	static float GetExpPowerScale()     { return expPowerScale; }
	static void  SetExpHealthScale(float value) { expHealthScale = value; }
	static float GetExpHealthScale()     { return expHealthScale; }
	static void  SetExpReloadScale(float value) { expReloadScale = value; }
	static float GetExpReloadScale()     { return expReloadScale; }
	static void  SetExpGrade(float value) { expGrade = value; }
	static float GetExpGrade()     { return expGrade; }

private:
	static float lodFactor; // unsynced

	static float expMultiplier;
	static float expPowerScale;
	static float expHealthScale;
	static float expReloadScale;
	static float expGrade;

	static float empDecline;
};

#endif // UNIT_H
