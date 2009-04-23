#ifndef UNIT_H
#define UNIT_H
// Unit.h: interface for the CUnit class.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <vector>
#include <string>

#include "Lua/LuaUnitMaterial.h"
#include "Sim/Objects/SolidObject.h"
#include "Matrix44f.h"
#include "Vec2.h"

class CCobInstance;
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
struct S3DModel;
struct LocalModel;
struct LocalModelPiece;
struct UnitDef;
struct UnitTrackStruct;
struct CollisionVolume;

#ifdef DIRECT_CONTROL_ALLOWED
	struct DirectControlStruct;
#endif

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

	bool AttackGround(const float3&pos,bool dgun);
	bool AttackUnit(CUnit* unit,bool dgun);

	virtual void DoDamage(const DamageArray& damages, CUnit* attacker,
	                      const float3& impulse, int weaponId = -1);
	virtual void Kill(float3& impulse);
	virtual void FinishedBuilding(void);

	int GetBlockingMapID() const { return id; }

	void ChangeLos(int l, int airlos);
	void ChangeSensorRadius(int* valuePtr, int newValue);
	bool AddBuildPower(float amount,CUnit* builder);		//negative amount=reclaim, return= true -> build power was succesfully applied
	void Activate();		//turn the unit on
	void Deactivate();		//turn the unit off

	void ForcedMove(const float3& newPos);
	void ForcedSpin(const float3& newDir);
	void SetFront(const SyncedFloat3& newDir);
	void SetUp(const SyncedFloat3& newDir);
	void SetRight(const SyncedFloat3& newDir);
	void EnableScriptMoveType();
	void DisableScriptMoveType();

	//void SetGoal(float3 pos);		//order unit to move to position

	virtual void SlowUpdate();
	virtual void Update();

	void SetDirectionFromHeading();

	void ApplyTransformMatrix() const;
	CMatrix44f GetTransformMatrix(const bool synced = false, const bool error = false) const;

	void SetLastAttacker(CUnit* attacker);
	void DependentDied(CObject* o);
	void SetUserTarget(CUnit* target);
	virtual void Init(const CUnit* builder);
	bool SetGroup(CGroup* group);

	bool AllowedReclaim(CUnit *builder);
	bool UseMetal(float metal);
	void AddMetal(float metal, bool handicap = true);
	bool UseEnergy(float energy);
	void AddEnergy(float energy, bool handicap = true);
	void UpdateWind(float x, float z, float strength);		//push the new wind to the script
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

	enum ChangeType{
		ChangeGiven,
		ChangeCaptured
	};
	virtual bool ChangeTeam(int team, ChangeType type);
	virtual void StopAttackingAllyTeam(int ally);

	// should not be here
	void DrawS3O();

	const UnitDef* unitDef;
	CollisionVolume* collisionVolume;
	std::string unitDefName;

	/**
	 * This is a set of parameters that is initialized
	 * in CreateUnitRulesParams() and may change during the game.
	 * Each parameter is uniquely identified only by its id
	 * (which is the index in the vector).
	 * Parameters may or may not have a name.
	 */
	std::vector<float>         modParams;    // mod controlled parameters
	std::map<std::string, int> modParamsMap; // name map for mod parameters

	int team;
	int allyteam;
	/**
	 * The unit's origin lies in this team.
	 *
	 * example:
	 * It was created by a factory that was created by a builder
	 * from a factory built by a commander of this team.
	 * It does not matter at all, to which team
	 * the commander/builder/factories were shared.
	 * Only capturing can break the chain.
	 */
	int lineage;
	int aihint;							//tells the unit main function to the ai

	SyncedFloat3 frontdir;				// the forward direction of the unit
	SyncedFloat3 rightdir;
	SyncedFloat3 updir;
	bool upright;						// if the updir is straight up or align to the ground vector
	SyncedFloat3 relMidPos;				// = (midPos - pos)

	float3 deathSpeed;

	float travel; // total distance the unit has moved
	float travelPeriod; // 0.0f disables travel accumulation

	float power;						//indicate the relative power of the unit, used for experience calulations etc

	float maxHealth;
	float health;
	float paralyzeDamage;		//if health-this is negative the unit is stunned
	float captureProgress;	//how close this unit is to being captured
	float experience;
	float limExperience;		//goes ->1 as experience go -> infinite

	bool neutral;             // neutral allegiance, will not be automatically
	                          // fired upon unless the fireState is set to >= 3

	CUnit* soloBuilder;
	bool beingBuilt;
	int lastNanoAdd;					//if we arent built on for a while start decaying
	float repairAmount;                 //How much reapir power has been added to this recently
	CTransportUnit *transporter;		//transport that the unit is currently in
	bool toBeTransported;			//unit is about to be picked up by a transport
	float buildProgress;			//0.0-1.0
	bool groundLevelled;            //whether the ground below this unit has been terraformed
	float terraformLeft;            //how much terraforming is left to do
	int realLosRadius;				//set los to this when finished building
	int realAirLosRadius;

	std::vector<unsigned short> losStatus;	//indicate the los/radar status the allyteam has on this unit

	bool inBuildStance;				//used by constructing units
	bool stunned;							//if we are stunned by a weapon or for other reason
	bool useHighTrajectory;		//tells weapons that support it to try to use a high trajectory
	bool dontUseWeapons;			//used by landed aircrafts for now

	bool deathScriptFinished;	//the script has finished exectuting the killed function and the unit can be deleted
	int deathCountdown;				//asserts a certain minimum time between death and deletion
	int delayedWreckLevel;		//the wreck level the unit will eventually create when it has died

	int restTime;							//how long the unit has been inactive

	std::vector<CWeapon*> weapons;
	CWeapon* shieldWeapon;		//if we have a shield weapon
	CWeapon* stockpileWeapon;	//if we have a weapon with stockpiled ammo
	float reloadSpeed;
	float maxRange;
	bool haveTarget;
	bool haveUserTarget;
	bool haveDGunRequest;
	float lastMuzzleFlameSize;				//used to determine muzzle flare size
	float3 lastMuzzleFlameDir;

	int armorType;
	unsigned int category;							//what categories the unit is part of (bitfield)

	std::vector<int> quads;			//quads the unit is part of
	LosInstance* los;						//which squares the unit can currently observe

	int tempNum;								//used to see if something has operated on the unit before
	int lastSlowUpdate;

	int mapSquare;

	int controlRadius;
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
	bool stealth;
	bool sonarStealth;

	AMoveType* moveType;
	AMoveType* prevMoveType;
	bool usingScriptMoveType;

	CCommandAI* commandAI;
	CGroup* group; // if the unit is part of an group (hotkey group)

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

	float metalUse;   // cost per 16 frames
	float energyUse;  // cost per 16 frames
	float metalMake;  // metal income generated by unit
	float energyMake; // energy income generated by unit

	//variables used for calculating unit resource usage
	float metalUseI;
	float energyUseI;
	float metalMakeI;
	float energyMakeI;
	float metalUseold;
	float energyUseold;
	float metalMakeold;
	float energyMakeold;
	float energyTickMake;  //energy added each halftick

	float metalExtract;			//how much metal the unit currently extracts from the ground

	float metalCost;
	float energyCost;
	float buildTime;

	float metalStorage;
	float energyStorage;

	CUnit* lastAttacker;                  // last attacker
	LocalModelPiece* lastAttackedPiece;   // piece that was last hit by a projectile
	int lastAttackedPieceFrame;           // frame in which lastAttackedPiece was hit
	int lastAttack;                       // last frame unit was attacked by other unit
	int lastDamage;                       // last frame the unit was damaged
	int lastFireWeapon;                   // last time this unit fired a weapon
	float recentDamage;                   // decaying value of how much damage the unit has taken recently (for severity of death)

	CUnit* userTarget;
	float3 userAttackPos;
	bool userAttackGround;
	int commandShotCount;				//number of shots due to the latest command

	int fireState;							//0=hold fire,1=return,2=fire at will
	bool dontFire;							//temp variable that can be set when building etc to stop units to turn away to fire
	int moveState;							//0=hold pos,1=maneuvre,2=roam

	bool activated;					//if the unit is in it's 'on'-state

	inline CTransportUnit *GetTransporter() const {
#if defined(USE_GML) && GML_ENABLE_SIM
		return *(CTransportUnit * volatile *)&transporter; // transporter may suddenly be changed to NULL by sim
#else
		return transporter;
#endif
	}

	void UpdateDrawPos();
	float3 drawPos;
	float3 drawMidPos;
#if defined(USE_GML) && GML_ENABLE_SIM
	unsigned lastUnitUpdate;
#endif
	S3DModel *model;
	LocalModel *localmodel;
	CCobInstance *cob;
	CUnitScript* script;

	std::string tooltip;

	bool crashing;
	bool isDead;    // prevent damage from hitting an already dead unit (causing multi wreck etc)
	bool	falling;  // for units being dropped from transports (parachute drops)
	float	fallSpeed;

	bool inAir;
	bool inWater;

	int flankingBonusMode;  // 0 = no flanking bonus
	                        // 1 = global coords, mobile
	                        // 2 = unit coords, mobile
	                        // 3 = unit coords, locked
	float3 flankingBonusDir;         // units takes less damage when attacked from this dir (encourage flanking fire)
	float  flankingBonusMobility;    // how much the lowest damage direction of the flanking bonus can turn upon an attack (zeroed when attacked, slowly increases)
	float  flankingBonusMobilityAdd; // how much ability of the flanking bonus direction to move builds up each frame
	float  flankingBonusAvgDamage;   // average factor to multiply damage by
	float  flankingBonusDifDamage;   // (max damage - min damage) / 2

	bool armoredState;
	float armoredMultiple;
	float curArmorMultiple;			//multiply all damage the unit take with this

	std::string wreckName;

	float3 posErrorVector;			//used for innacuracy with radars etc
	float3 posErrorDelta;
	int	nextPosErrorUpdate;

	bool hasUWWeapons;					//true if the unit has weapons that can fire at underwater targets

	bool wantCloak;							//true if the unit currently wants to be cloaked
	int scriptCloak;						//true if a script currently wants the unit to be cloaked
	int cloakTimeout;						//the minimum time between decloaking and cloaking again
	int curCloakTimeout;				//the earliest frame the unit can cloak again
	bool isCloaked;							//true if the unit is currently cloaked (has enough energy etc)
	float decloakDistance;

	int lastTerrainType;
	int curTerrainType;					// Used for calling setSFXoccupy which TA scripts want

	int selfDCountdown;
#ifdef DIRECT_CONTROL_ALLOWED
	DirectControlStruct* directControl;
#endif

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

	float maxSpeed;  //max speed of the unit

	float weaponHitMod; //percentage of weapondamage to use when hit by weapon (set by script callbak

	// unsynced calls
	void SetLODCount(unsigned int count);
	unsigned int CalcLOD(unsigned int lastLOD) const;
	unsigned int CalcShadowLOD(unsigned int lastLOD) const;
	// unsynced data
	unsigned int lodCount;
	unsigned int currentLOD;
	vector<float> lodLengths; // length-per-pixel
	LuaUnitMaterial luaMats[LUAMAT_TYPE_COUNT];

	float alphaThreshold;	// minimum alpha value for a texel to be drawn
	int cegDamage;			// the damage value passed to CEGs spawned by this unit's script

protected:
	void ChangeTeamReset();
	void UpdateResources();
	void UpdateLosStatus(int allyTeam);

public:
	virtual void KillUnit(bool SelfDestruct,bool reclaimed, CUnit *attacker, bool showDeathSequence = true);
	virtual void LoadSave(CLoadSaveInterface* file, bool loading);
	virtual void IncomingMissile(CMissileProjectile* missile);
	void TempHoldFire(void);
	void ReleaseTempHoldFire(void);
	void Drop(float3 parentPos,float3 parentDir,CUnit* parent); //start this unit in freefall from parent unit
	void PostLoad();
	static void hitByWeaponIdCallback(int retCode, void *p1, void *p2);

public:
	static void SetLODFactor(float);

	static void  SetExpMultiplier(float value) { expMultiplier = value; }
	static float GetExpMultiplier()     { return expMultiplier; }
	static void  SetExpPowerScale(float value) { expPowerScale = value; }
	static float GetExpPowerScale()    { return expPowerScale; }
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

public:
	void LogMessage(const char*, ...);
};

#endif /* UNIT_H */
