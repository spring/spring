/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDef.h"
#include "IkChain.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "UnitDefHandler.h"
#include "UnitLoader.h"
#include "UnitMemPool.h"
#include "UnitToolTipMap.hpp"
#include "UnitTypes/Building.h"
#include "Scripts/NullUnitScript.h"
#include "Scripts/UnitScriptFactory.h"
#include "Scripts/CobInstance.h" // for TAANG2RAD

#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/MobileCAI.h"

#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/GroundFlash.h"

#include "Game/UI/Groups/Group.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/MoveTypeFactory.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/WeaponLoader.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/creg/DefTypes.h"
#include "System/creg/STL_List.h"
#include "System/Sound/ISoundChannels.h"
#include "System/Sync/SyncedPrimitive.h"
#include "System/Sync/SyncTracer.h"


// See end of source for member bindings
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool  CUnit::spawnFeature = true;

float CUnit::empDeclineRate = 0.0f;
float CUnit::expMultiplier  = 0.0f;
float CUnit::expPowerScale  = 0.0f;
float CUnit::expHealthScale = 0.0f;
float CUnit::expReloadScale = 0.0f;
float CUnit::expGrade       = 0.0f;


CUnit::CUnit()
: CSolidObject()
, unitDef(nullptr)

, shieldWeapon(nullptr)
, stockpileWeapon(nullptr)

, selfdExpDamages(nullptr)
, deathExpDamages(nullptr)

, soloBuilder(nullptr)
, lastAttacker(nullptr)
, transporter(nullptr)

, fpsControlPlayer(nullptr)

, moveType(nullptr)
, prevMoveType(nullptr)
, commandAI(nullptr)
, script(nullptr)

, los{nullptr}
, losStatus{0}

, incomingMissiles{nullptr}

, deathSpeed(ZeroVector)
, lastMuzzleFlameDir(UpVector)
, flankingBonusDir(RgtVector)
, posErrorVector(ZeroVector)
, posErrorDelta(ZeroVector)

, featureDefID(-1)
, power(100.0f)
, buildProgress(0.0f)
, paralyzeDamage(0.0f)
, captureProgress(0.0f)
, experience(0.0f)
, limExperience(0.0f)
, neutral(false)
, beingBuilt(true)
, upright(true)
, groundLevelled(true)
, terraformLeft(0.0f)
, repairAmount(0.0f)

, lastAttackFrame(-200)
, lastFireWeapon(0)
, lastNanoAdd(gs->frameNum)
, lastFlareDrop(0)

, loadingTransportId(-1)
, unloadingTransportId(-1)
, transportCapacityUsed(0)
, transportMassUsed(0)

, inBuildStance(false)
, useHighTrajectory(false)
, dontUseWeapons(false)
, dontFire(false)
, deathScriptFinished(false)

, delayedWreckLevel(-1)
, restTime(0)
, outOfMapTime(0)
, reloadSpeed(1.0f)
, maxRange(0.0f)
, lastMuzzleFlameSize(0.0f)
, armorType(0)
, category(0)
, mapSquare(-1)
, realLosRadius(0)
, realAirLosRadius(0)
, losRadius(0)
, airLosRadius(0)
, radarRadius(0)
, sonarRadius(0)
, jammerRadius(0)
, sonarJamRadius(0)
, seismicRadius(0)
, seismicSignature(0.0f)
, stealth(false)
, sonarStealth(false)
, cost(100.0f, 0.0f)
, energyTickMake(0.0f)
, metalExtract(0.0f)
, buildTime(100.0f)
, recentDamage(0.0f)
, fireState(FIRESTATE_FIREATWILL)
, moveState(MOVESTATE_MANEUVER)
, activated(false)
, isDead(false)
, fallSpeed(0.2f)
, travel(0.0f)
, travelPeriod(0.0f)
, flankingBonusMode(0)
, flankingBonusMobility(10.0f)
, flankingBonusMobilityAdd(0.01f)
, flankingBonusAvgDamage(1.4f)
, flankingBonusDifDamage(0.5f)
, armoredState(false)
, armoredMultiple(1.0f)
, curArmorMultiple(1.0f)
, nextPosErrorUpdate(1)

, isCloaked(false)
, wantCloak(false)
, decloakDistance(0.0f)

, lastTerrainType(-1)
, curTerrainType(0)

, selfDCountdown(0)
, cegDamage(1)
, ikIDPool(0)

, noMinimap(false)
, leaveTracks(false)
, isSelected(false)
, isIcon(false)
, iconRadius(0.0f)
, lastUnitUpdate(0)
, group(nullptr)

, stunned(false)
{
	assert(unitMemPool.alloced(this));
	static_assert((sizeof(los) / sizeof(los[0])) == ILosType::LOS_TYPE_COUNT, "");
	static_assert((sizeof(losStatus) / sizeof(losStatus[0])) == MAX_TEAMS, "");
}

CUnit::~CUnit()
{
	assert(unitMemPool.mapped(this));
	// clean up if we are still under MoveCtrl here
	DisableScriptMoveType();

	if (delayedWreckLevel >= 0) {
		// NOTE: could also do this in Update() or even in CUnitKilledCB()
		// where we wouldn't need deathSpeed, but not in KillUnit() since
		// we have to wait for deathScriptFinished (but we want the delay
		// in frames between CUnitKilledCB() and the CreateWreckage() call
		// to be as short as possible to prevent position jumps)
		FeatureLoadParams params = {featureDefHandler->GetFeatureDefByID(featureDefID), unitDef, pos, deathSpeed, -1, team, -1, heading, buildFacing, delayedWreckLevel - 1, 1};
		featureHandler.CreateWreckage(params);
	}
	if (deathExpDamages != nullptr)
		DynDamageArray::DecRef(deathExpDamages);
	if (selfdExpDamages != nullptr)
		DynDamageArray::DecRef(selfdExpDamages);

#ifdef TRACE_SYNC
	tracefile << "Unit died: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif

	if (fpsControlPlayer != nullptr) {
		fpsControlPlayer->StopControllingUnit();
		assert(fpsControlPlayer == nullptr);
	}

	if (activated && unitDef->targfac)
		losHandler->IncreaseAllyTeamRadarErrorSize(allyteam);

	SetMetalStorage(0);
	SetEnergyStorage(0);

	// not all unit deletions run through KillUnit(),
	// but we always want to call this for ourselves
	UnBlock();

	// Remove us from our group, if we were in one
	SetGroup(nullptr);

	// delete script first so any callouts still see valid ptrs
	DeleteScript();

	spring::SafeDestruct(commandAI);
	spring::SafeDestruct(moveType);
	spring::SafeDestruct(prevMoveType);
	//delete all existing IK-Chains
	for (IkChain* ik: IkChains){
		delete ik;
	}

	// ScriptCallback may reference weapons, so delete the script first
	CWeaponLoader::FreeWeapons(this);
	quadField.RemoveUnit(this);
}


void CUnit::InitStatic()
{
	spawnFeature = true;

	// numerator was 2*UNIT_SLOWUPDATE_RATE/GAME_SPEED which equals 1 since 99.0
	SetEmpDeclineRate(1.0f / modInfo.paralyzeDeclineRate);
	SetExpMultiplier(modInfo.unitExpMultiplier);
	SetExpPowerScale(modInfo.unitExpPowerScale);
	SetExpHealthScale(modInfo.unitExpHealthScale);
	SetExpReloadScale(modInfo.unitExpReloadScale);
	SetExpGrade(0.0f);

	CBuilderCAI::InitStatic();
	unitToolTipMap.Clear();
}


void CUnit::PreInit(const UnitLoadParams& params)
{
	// if this is < 0, UnitHandler will give us a random ID
	id = params.unitID;
	featureDefID = -1;

	unitDef = params.unitDef;

	{
		const FeatureDef* wreckFeatureDef = featureDefHandler->GetFeatureDef(unitDef->wreckName);

		if (wreckFeatureDef != nullptr) {
			featureDefID = wreckFeatureDef->id;

			while (wreckFeatureDef != nullptr) {
				wreckFeatureDef->PreloadModel();
				wreckFeatureDef = featureDefHandler->GetFeatureDefByID(wreckFeatureDef->deathFeatureDefID);
			}
		}
	}
	for (const auto it: unitDef->buildOptions) {
		const UnitDef* ud = unitDefHandler->GetUnitDefByName(it.second);
		if (ud == nullptr)
			continue;
		ud->PreloadModel();
	}

	team = params.teamID;
	allyteam = teamHandler.AllyTeam(team);

	buildFacing = std::abs(params.facing) % NUM_FACINGS;
	xsize = ((buildFacing & 1) == 0) ? unitDef->xsize : unitDef->zsize;
	zsize = ((buildFacing & 1) == 1) ? unitDef->xsize : unitDef->zsize;


	localModel.SetModel(model = unitDef->LoadModel());

	collisionVolume = unitDef->collisionVolume;
	selectionVolume = unitDef->selectionVolume;

	// specialize defaults if non-custom sphere or footprint-box
	collisionVolume.InitDefault(float4(model->radius, model->height,  xsize * SQUARE_SIZE, zsize * SQUARE_SIZE));
	selectionVolume.InitDefault(float4(model->radius, model->height,  xsize * SQUARE_SIZE, zsize * SQUARE_SIZE));


	mapSquare = CGround::GetSquare((params.pos).cClampInMap());

	heading  = GetHeadingFromFacing(buildFacing);
	upright  = unitDef->upright;

	SetVelocity(params.speed);
	Move((params.pos).cClampInMap(), false);
	UpdateDirVectors(!upright);
	SetMidAndAimPos(model->relMidPos, model->relMidPos, true);
	SetRadiusAndHeight(model);
	UpdateMidAndAimPos();

	unitHandler.AddUnit(this);
	quadField.MovedUnit(this);

	losStatus[allyteam] = LOS_ALL_MASK_BITS | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] id: " << id << ", name: " << unitDef->name << " ";
	tracefile << "pos: <" << pos.x << ", " << pos.y << ", " << pos.z << ">\n";
#endif

	ASSERT_SYNCED(pos);

	blockMap = (unitDef->GetYardMap()).data(); // null if empty
	footprint = int2(unitDef->xsize, unitDef->zsize);

	beingBuilt = params.beingBuilt;
	mass = (beingBuilt)? mass: unitDef->mass;
	crushResistance = unitDef->crushResistance;
	power = unitDef->power;
	maxHealth = unitDef->health;
	health = beingBuilt? 0.1f: unitDef->health;
	cost.metal = unitDef->metal;
	cost.energy = unitDef->energy;
	buildTime = unitDef->buildTime;
	armoredMultiple = std::max(0.0001f, unitDef->armoredMultiple); // armored multiple of 0 will crash spring
	armorType = unitDef->armorType;
	category = unitDef->category;
	leaveTracks = unitDef->decalDef.leaveTrackDecals;

	unitToolTipMap.Set(id, unitDef->humanName + " - " + unitDef->tooltip);


	// sensor parameters
	realLosRadius    = Clamp(int(unitDef->losRadius)    , 0, MAX_UNIT_SENSOR_RADIUS);
	realAirLosRadius = Clamp(int(unitDef->airLosRadius) , 0, MAX_UNIT_SENSOR_RADIUS);
	radarRadius      = Clamp(    unitDef->radarRadius   , 0, MAX_UNIT_SENSOR_RADIUS);
	sonarRadius      = Clamp(    unitDef->sonarRadius   , 0, MAX_UNIT_SENSOR_RADIUS);
	jammerRadius     = Clamp(    unitDef->jammerRadius  , 0, MAX_UNIT_SENSOR_RADIUS);
	sonarJamRadius   = Clamp(    unitDef->sonarJamRadius, 0, MAX_UNIT_SENSOR_RADIUS);
	seismicRadius    = Clamp(    unitDef->seismicRadius , 0, MAX_UNIT_SENSOR_RADIUS);
	seismicSignature = unitDef->seismicSignature;

	stealth = unitDef->stealth;
	sonarStealth = unitDef->sonarStealth;

	// can be overridden by cloak orders during construction
	wantCloak |= unitDef->startCloaked;
	decloakDistance = unitDef->decloakDistance;

	flankingBonusMode        = unitDef->flankingBonusMode;
	flankingBonusDir         = unitDef->flankingBonusDir;
	flankingBonusMobility    = unitDef->flankingBonusMobilityAdd * 1000;
	flankingBonusMobilityAdd = unitDef->flankingBonusMobilityAdd;
	flankingBonusAvgDamage   = (unitDef->flankingBonusMax + unitDef->flankingBonusMin) * 0.5f;
	flankingBonusDifDamage   = (unitDef->flankingBonusMax - unitDef->flankingBonusMin) * 0.5f;

	useHighTrajectory = (unitDef->highTrajectoryType == 1);

	energyTickMake = unitDef->energyMake + unitDef->tidalGenerator * mapInfo->map.tidalStrength;

	harvestStorage.metal  = unitDef->harvestMetalStorage;
	harvestStorage.energy = unitDef->harvestEnergyStorage;

	moveType = MoveTypeFactory::GetMoveType(this, unitDef);
	script = CUnitScriptFactory::CreateScript(this, unitDef);

	if (unitDef->selfdExpWeaponDef != nullptr)
		selfdExpDamages = DynDamageArray::IncRef(&unitDef->selfdExpWeaponDef->damages);
	if (unitDef->deathExpWeaponDef != nullptr)
		deathExpDamages = DynDamageArray::IncRef(&unitDef->deathExpWeaponDef->damages);

	commandAI = CUnitLoader::NewCommandAI(this, unitDef);
}


void CUnit::PostInit(const CUnit* builder)
{
	CWeaponLoader::LoadWeapons(this);
	CWeaponLoader::InitWeapons(this);

	// does nothing for LUS, calls Create+SetMaxReloadTime for COB
	script->Create();

	// all units are blocking (ie. register on the blocking-map
	// when not flying) except mines, since their position would
	// be given away otherwise by the PF, etc.
	// NOTE: this does mean that mines can be stacked indefinitely
	// (an extra yardmap character would be needed to prevent this)
	immobile = unitDef->IsImmobileUnit();

	UpdateCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS, unitDef->collidable && (!immobile || !unitDef->canKamikaze));
	Block();

	if (unitDef->windGenerator > 0.0f)
		wind.AddUnit(this);

	UpdateTerrainType();
	UpdatePhysicalState(0.1f);
	UpdatePosErrorParams(true, true);

	if (FloatOnWater() && IsInWater())
		Move(UpVector * (std::max(CGround::GetHeightReal(pos.x, pos.z), -moveType->GetWaterline()) - pos.y), true);

	if (unitDef->canmove || unitDef->builder) {
		if (unitDef->moveState <= MOVESTATE_NONE) {
			// always inherit our builder's movestate
			// if none, set CUnit's default (maneuver)
			if (builder != nullptr)
				moveState = builder->moveState;

		} else {
			// use our predefined movestate
			moveState = unitDef->moveState;
		}

		commandAI->GiveCommand(Command(CMD_MOVE_STATE, 0, moveState));
	}

	if (commandAI->CanChangeFireState()) {
		if (unitDef->fireState <= FIRESTATE_NONE) {
			// inherit our builder's firestate (if it is a factory)
			// if no builder, CUnit's default (fire-at-will) is set
			if (builder != nullptr && dynamic_cast<CFactoryCAI*>(builder->commandAI) != nullptr)
				fireState = builder->fireState;

		} else {
			// use our predefined firestate
			fireState = unitDef->fireState;
		}

		commandAI->GiveCommand(Command(CMD_FIRE_STATE, 0, fireState));
	}

	// Lua might call SetUnitHealth within UnitCreated
	// and trigger FinishedBuilding before we get to it
	const bool preBeingBuilt = beingBuilt;

	// these must precede UnitFinished from FinishedBuilding
	eventHandler.UnitCreated(this, builder);
	eoh->UnitCreated(*this, builder);

	// skip past the gradual build-progression
	if (!preBeingBuilt && !beingBuilt)
		FinishedBuilding(true);

	eventHandler.RenderUnitCreated(this, isCloaked);
}


void CUnit::PostLoad()
{
	blockMap = (unitDef->GetYardMap()).data();

	if (unitDef->windGenerator > 0.0f)
		wind.AddUnit(this);

	eventHandler.RenderUnitCreated(this, isCloaked);
}



//////////////////////////////////////////////////////////////////////
//Returns true if the ikID is found in this Unit

bool CUnit::isValidIKChain(float ikID){
	if (IkChains.empty()) return false;
	
	for (auto ik = IkChains.cbegin(); ik != IkChains.cend(); ++ik) {

				if ((*ik)->IkChainID == ikID) {
					return true;
				}
			}
	return false;
}



bool CUnit::isValidIKChainPiece(float ikID, float pieceID){
	if (IkChains.empty()) return false;

	IkChain* ik = getIKChain(ikID);

	if (ik == NULL) return false;

	return (*ik).isValidIKPiece(pieceID);
}

//returns the IKChain if existing, else NULL
//in O(n)- which is okay for a small list
IkChain* CUnit::getIKChain( float ikID)
{
	if (IkChains.empty()) return NULL;

	for (auto ik = IkChains.cbegin(); ik != IkChains.cend(); ++ik) {		
				if ((*ik)->IkChainID == ikID)  {
					return (*ik);
				}
			}

	return NULL;
}

//Create the IKChain and return the ikID
float CUnit::CreateIKChain(LocalModelPiece* startPiece, unsigned int startPieceID, unsigned int endPieceID)
{
	//ikIds are unique per Unit
	ikIDPool+= 1;
	IkChain * kinematIkChain= new IkChain((int)ikIDPool, this, startPiece, startPieceID, endPieceID);
	IkChains.push_back(kinematIkChain);
	kinematIkChain->print();
	return kinematIkChain->IkChainID;	
}

//Activates or deactivates the IK-Chain
void CUnit::SetIKActive(float ikID, bool Active){
	IkChain* ik = getIKChain(ikID);
	
	if (ik){
		(*ik).SetActive(Active);
		(*ik).GoalChanged=true;
	}
};

//Defines the TargetsCoords in UnitSpace
void CUnit::SetIKGoal(float ikID, float goalX, float goalY, float goalZ, bool isWorldCoordinate){
	IkChain* ik = getIKChain(ikID);
	
	if (ik){
	(*ik).goalPoint[0]	= goalX;
	(*ik).goalPoint[1]	= goalY;
	(*ik).goalPoint[2]	= goalZ;	
	(*ik).GoalChanged 	= true;
	(*ik).isWorldCoordinate = isWorldCoordinate;
	(*ik).print();
	}
};

//TODO SetIKTime(float ikID, float time)

//Sets the Per Piece Velocity per Axis
void CUnit::SetIKPieceSpeed(float ikID, float ikPieceID, float velX, float velY, float velZ){
	IkChain* ik = getIKChain(ikID);
	
	if (ik){
	(*ik).segments[(int)ikPieceID].velocity[0]= velX;
	(*ik).segments[(int)ikPieceID].velocity[1]= velY;	
	(*ik).segments[(int)ikPieceID].velocity[2]= velZ;
	}
};
//Defines a Joint Rotation Limit for each axis 
void CUnit::SetIKPieceLimit(float ikID, 
							float ikPieceID, 
							float limX, 
							float limUpX,
							float limY, 
							float limUpY,	
							float limZ,
							float limUpZ){
	IkChain* ik = getIKChain(ikID);
	
	if (ik){
		(*ik).segments[(int)ikPieceID].setLimitJoint( 	limX, 
														limUpX,
														limY, 
														limUpY,	
														limZ,
														limUpZ);

	}
};
//////////////////////////////////////////////////////////////////////
//

void CUnit::FinishedBuilding(bool postInit)
{
	if (!beingBuilt && !postInit)
		return;

	beingBuilt = false;
	buildProgress = 1.0f;
	mass = unitDef->mass;

	if (soloBuilder != nullptr) {
		DeleteDeathDependence(soloBuilder, DEPENDENCE_BUILDER);
		soloBuilder = nullptr;
	}

	ChangeLos(realLosRadius, realAirLosRadius);

	if (unitDef->activateWhenBuilt)
		Activate();

	SetMetalStorage(unitDef->metalStorage);
	SetEnergyStorage(unitDef->energyStorage);


	// Sets the frontdir in sync with heading.
	frontdir = GetVectorFromHeading(heading) + (UpVector * frontdir.y);

	eventHandler.UnitFinished(this);
	eoh->UnitFinished(*this);

	if (unitDef->isFeature && CUnit::spawnFeature) {
		FeatureLoadParams p = {featureDefHandler->GetFeatureDefByID(featureDefID), nullptr, pos, ZeroVector, -1, team, allyteam, heading, buildFacing, 0, 0};
		CFeature* f = featureHandler.CreateWreckage(p);

		if (f != nullptr)
			f->blockHeightChanges = true;

		UnBlock();
		KillUnit(nullptr, false, true);
	}
}


void CUnit::KillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool showDeathSequence)
{
	if (IsCrashing() && !beingBuilt)
		return;

	ForcedKillUnit(attacker, selfDestruct, reclaimed, showDeathSequence);
}

void CUnit::ForcedKillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool showDeathSequence)
{
	if (isDead)
		return;

	isDead = true;

	// release attached units
	ReleaseTransportees(attacker, selfDestruct, reclaimed);

	// pre-destruction event; unit may be kept around for its death sequence
	eventHandler.UnitDestroyed(this, attacker);
	eoh->UnitDestroyed(*this, attacker);

	deathSpeed = speed;

	// Will be called in the destructor again, but this can not hurt
	SetGroup(nullptr);

	if (unitDef->windGenerator > 0.0f)
		wind.DelUnit(this);

	blockHeightChanges = false;
	deathScriptFinished = (!showDeathSequence || reclaimed || beingBuilt);

	if (deathScriptFinished)
		return;

	const WeaponDef* wd = selfDestruct? unitDef->selfdExpWeaponDef: unitDef->deathExpWeaponDef;
	const DynDamageArray* da = selfDestruct? selfdExpDamages: deathExpDamages;

	if (wd != nullptr) {
		assert(da != nullptr);
		CExplosionParams params = {
			pos,
			ZeroVector,
			*da,
			wd,
			this,                                    // owner
			nullptr,                                 // hitUnit
			nullptr,                                 // hitFeature
			da->craterAreaOfEffect,
			da->damageAreaOfEffect,
			da->edgeEffectiveness,
			da->explosionSpeed,
			(da->GetDefault() > 500.0f)? 1.0f: 2.0f, // gfxMod
			false,                                   // impactOnly
			false,                                   // ignoreOwner
			true,                                    // damageGround
			-1u                                      // projectileID
		};

		helper->Explosion(params);
	}

	recentDamage += (maxHealth * 2.0f * selfDestruct);

	// start running the unit's kill-script
	script->Killed();
}


void CUnit::ForcedMove(const float3& newPos)
{
	UnBlock();
	Move(newPos - pos, true);
	Block();

	eventHandler.UnitMoved(this);
	quadField.MovedUnit(this);
}



float3 CUnit::GetErrorVector(int argAllyTeam) const
{
	const int tstAllyTeam = argAllyTeam * (argAllyTeam >= 0) * (argAllyTeam < teamHandler.ActiveAllyTeams());

	const bool b0 = (1     ) && (tstAllyTeam != argAllyTeam); // LuaHandle without full read access
	const bool b1 = (1 - b0) && ((losStatus[tstAllyTeam] & LOS_INLOS  ) != 0 || teamHandler.Ally(tstAllyTeam, allyteam)); // in LOS or allied, no error
	const bool b2 = (1 - b0) && ((losStatus[tstAllyTeam] & LOS_PREVLOS) != 0 && gameSetup->ghostedBuildings && unitDef->IsImmobileUnit()); // seen ghosted building, no error
	const bool b3 = (1 - b0) && ((losStatus[tstAllyTeam] & LOS_INRADAR) != 0); // current radar contact

	switch ((b0 * 1) + (b1 * 2) + (b2 * 4) + (b3 * 8)) {
		case  0: { return (posErrorVector * losHandler->GetBaseRadarErrorSize() * 2.0f);         } break; // !b0 &&  !b1 && !b2  && !b3
		case  1: { return (posErrorVector * losHandler->GetBaseRadarErrorSize() * 2.0f);         } break; //  b0
		case  8: { return (posErrorVector * losHandler->GetAllyTeamRadarErrorSize(tstAllyTeam)); } break; // !b0 &&  !b1 && !b2  &&  b3
		default: {                                                                               } break; // !b0 && ( b1 ||  b2) && !b3
	}

	return ZeroVector;
}

void CUnit::UpdatePosErrorParams(bool updateError, bool updateDelta)
{
	// every frame, magnitude of error increases
	// error-direction is fixed until next delta
	if (updateError)
		posErrorVector += posErrorDelta;
	if (!updateDelta)
		return;

	if ((--nextPosErrorUpdate) > 0)
		return;

	constexpr float errorScale = 1.0f / 256.0f;
	constexpr float errorMults[] = {1.0f, -1.0f};

	float3 newPosError = gsRNG.NextVector();

	newPosError.y *= 0.2f;
	newPosError *= errorMults[posErrorVector.dot(newPosError) < 0.0f];

	posErrorDelta = (newPosError - posErrorVector) * errorScale;
	nextPosErrorUpdate = UNIT_SLOWUPDATE_RATE;
}

void CUnit::Drop(const float3& parentPos, const float3& parentDir, CUnit* parent)
{
	// drop unit from position
	fallSpeed = mix(unitDef->unitFallSpeed, parent->unitDef->fallSpeed, unitDef->unitFallSpeed <= 0.0f);

	frontdir = parentDir * XZVector;

	SetVelocityAndSpeed(speed * XZVector);
	Move(UpVector * ((parentPos.y - height) - pos.y), true);
	UpdateMidAndAimPos();
	SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FALLING);

	// start parachute animation
	script->Falling();
}


void CUnit::DeleteScript()
{
	if (script != &CNullUnitScript::value)
		spring::SafeDestruct(script);

	script = &CNullUnitScript::value;
}

void CUnit::EnableScriptMoveType()
{
	if (UsingScriptMoveType())
		return;

	prevMoveType = moveType;
	moveType = MoveTypeFactory::GetScriptMoveType(this);
}

void CUnit::DisableScriptMoveType()
{
	if (!UsingScriptMoveType())
		return;

	spring::SafeDestruct(moveType);

	moveType = prevMoveType;
	prevMoveType = nullptr;

	// ensure unit does not try to move back to the
	// position it was at when MoveCtrl was enabled
	// FIXME: prevent the issuing of extra commands?
	if (moveType == nullptr)
		return;

	moveType->SetGoal(moveType->oldPos = pos);
	moveType->StopMoving();
}


void CUnit::Update()
{
	ASSERT_SYNCED(pos);

	UpdatePhysicalState(0.1f);
	UpdatePosErrorParams(true, false);
	UpdateTransportees(); // none if already dead

	if (beingBuilt)
		return;
	if (isDead)
		return;

	if (travelPeriod != 0.0f)
		travel = math::fmod(travel += speed.w, travelPeriod);

	recentDamage *= 0.9f;
	flankingBonusMobility += flankingBonusMobilityAdd;

	if (IsStunned()) {
		// paralyzed weapons shouldn't reload
		for (CWeapon* w: weapons) {
			++(w->reloadStatus);
		}

		return;
	}

	restTime += 1;
	outOfMapTime += 1;
	outOfMapTime *= (!pos.IsInBounds());
	//Solve aktive Kinmatik Chains
	if (IkChains.size()> 0){
			for (auto ik = IkChains.cbegin(); ik != IkChains.cend(); ++ik) {
				IkChain* ikChain =(*ik); 

				if (ikChain->IKActive && (ikChain->GoalChanged || ikChain->isWorldCoordinate)) {
						ikChain->solve(100);	//TODO replce fixed attemptnumber	
					}
			}
	}
}

void CUnit::UpdateTransportees()
{
	for (TransportedUnit& tu: transportedUnits) {
		CUnit* transportee = tu.unit;

		transportee->mapSquare = mapSquare;

		float3 relPiecePos = ZeroVector;
		float3 absPiecePos = pos;

		if (tu.piece >= 0) {
			relPiecePos = script->GetPiecePos(tu.piece);
			absPiecePos = this->GetObjectSpacePos(relPiecePos);
		}

		if (unitDef->holdSteady) {
			// slave transportee orientation to piece
			if (tu.piece >= 0) {
				const CMatrix44f& transMat = GetTransformMatrix(true);
				const CMatrix44f& pieceMat = script->GetPieceMatrix(tu.piece);

				transportee->SetDirVectors(transMat * pieceMat);
			}
		} else {
			// slave transportee orientation to body
			transportee->heading  = heading;
			transportee->updir    = updir;
			transportee->frontdir = frontdir;
			transportee->rightdir = rightdir;
		}

		transportee->Move(absPiecePos, false);
		transportee->UpdateMidAndAimPos();
		transportee->SetHeadingFromDirection();

		// see ::AttachUnit
		if (transportee->IsStunned()) {
			quadField.MovedUnit(transportee);
		}
	}
}

void CUnit::ReleaseTransportees(CUnit* attacker, bool selfDestruct, bool reclaimed)
{
	for (TransportedUnit& tu: transportedUnits) {
		CUnit* transportee = tu.unit;
		assert(transportee != this);

		if (transportee->isDead)
			continue;

		transportee->SetTransporter(nullptr);
		transportee->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
		transportee->UpdateVoidState(false);

		if (!unitDef->releaseHeld) {
			// we don't want transportees to leave a corpse
			if (!selfDestruct)
				transportee->DoDamage(DamageArray(1e6f), ZeroVector, nullptr, -DAMAGE_EXTSOURCE_KILLED, -1);

			transportee->KillUnit(attacker, selfDestruct, reclaimed);
		} else {
			// NOTE: game's responsibility to deal with edge-cases now
			transportee->Move(transportee->pos.cClampInBounds(), false);

			// if this transporter uses the piece-underneath-ground
			// method to "hide" transportees, place transportee near
			// the transporter's place of death
			if (transportee->pos.y < CGround::GetHeightReal(transportee->pos.x, transportee->pos.z)) {
				const float r1 = transportee->radius + radius;
				const float r2 = r1 * std::max(unitDef->unloadSpread, 1.0f);

				// try to unload in a presently unoccupied spot
				// (if no such spot, unload on transporter wreck)
				for (int i = 0; i < 10; ++i) {
					float3 pos = transportee->pos;
					pos.x += (gsRNG.NextFloat() * 2.0f * r2 - r2);
					pos.z += (gsRNG.NextFloat() * 2.0f * r2 - r2);
					pos.y = CGround::GetHeightReal(pos.x, pos.z);

					if (!pos.IsInBounds())
						continue;

					if (quadField.NoSolidsExact(pos, transportee->radius + 2.0f, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS)) {
						transportee->Move(pos, false);
						break;
					}
				}
			} else {
				if (transportee->unitDef->IsGroundUnit()) {
					transportee->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
					transportee->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
				}
			}

			transportee->moveType->SlowUpdate();
			transportee->moveType->LeaveTransport();

			// issue a move order so that units dropped from flying
			// transports won't try to return to their pick-up spot
			if (unitDef->canfly && transportee->unitDef->canmove) {
				transportee->commandAI->GiveCommand(Command(CMD_MOVE, transportee->pos));
			}

			transportee->SetStunned(transportee->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? transportee->maxHealth: transportee->health));
			transportee->SetVelocityAndSpeed(speed * (0.5f + 0.5f * gsRNG.NextFloat()));

			eventHandler.UnitUnloaded(transportee, this);
		}
	}

	transportedUnits.clear();
}

void CUnit::TransporteeKilled(const CObject* o)
{
	const auto pred = [&](const TransportedUnit& tu) { return (tu.unit == o); };
	const auto iter = std::find_if(transportedUnits.begin(), transportedUnits.end(), pred);

	if (iter == transportedUnits.end())
		return;

	const CUnit* unit = iter->unit;

	transportCapacityUsed -= (unit->xsize / SPRING_FOOTPRINT_SCALE);
	transportMassUsed -= unit->mass;

	SetMass(mass - unit->mass);

	*iter = transportedUnits.back();
	transportedUnits.pop_back();
}

void CUnit::UpdateResources()
{
	resourcesMake.metal  = resourcesMakeI.metal  + resourcesMakeOld.metal;
	resourcesUse.metal   = resourcesUseI.metal   + resourcesUseOld.metal;
	resourcesMake.energy = resourcesMakeI.energy + resourcesMakeOld.energy;
	resourcesUse.energy  = resourcesUseI.energy  + resourcesUseOld.energy;

	resourcesMakeOld.metal  = resourcesMakeI.metal;
	resourcesUseOld.metal   = resourcesUseI.metal;
	resourcesMakeOld.energy = resourcesMakeI.energy;
	resourcesUseOld.energy  = resourcesUseI.energy;

	resourcesMakeI.metal = resourcesUseI.metal = resourcesMakeI.energy = resourcesUseI.energy = 0.0f;
}

void CUnit::SetLosStatus(int at, unsigned short newStatus)
{
	const unsigned short currStatus = losStatus[at];
	const unsigned short diffBits = (currStatus ^ newStatus);

	// add to the state before running the callins
	//
	// note that is not symmetric: UnitEntered* and
	// UnitLeft* are after-the-fact events, yet the
	// Left* call-ins would still see the old state
	// without first clearing the IN{LOS, RADAR} bit
	losStatus[at] |= newStatus;

	if (diffBits) {
		if (diffBits & LOS_INLOS) {
			if (newStatus & LOS_INLOS) {
				eventHandler.UnitEnteredLos(this, at);
				eoh->UnitEnteredLos(*this, at);
			} else {
				// clear before sending the event
				losStatus[at] &= ~LOS_INLOS;

				eventHandler.UnitLeftLos(this, at);
				eoh->UnitLeftLos(*this, at);
			}
		}

		if (diffBits & LOS_INRADAR) {
			if (newStatus & LOS_INRADAR) {
				eventHandler.UnitEnteredRadar(this, at);
				eoh->UnitEnteredRadar(*this, at);
			} else {
				// clear before sending the event
				losStatus[at] &= ~LOS_INRADAR;

				eventHandler.UnitLeftRadar(this, at);
				eoh->UnitLeftRadar(*this, at);
			}
		}
	}

	// remove from the state after running the callins
	losStatus[at] &= newStatus;
}


unsigned short CUnit::CalcLosStatus(int at) const
{
	const unsigned short currStatus = losStatus[at];

	unsigned short newStatus = currStatus;
	unsigned short mask = ~(currStatus >> LOS_MASK_SHIFT);

	if (losHandler->InLos(this, at)) {
		newStatus |= (mask & (LOS_INLOS   | LOS_INRADAR |
		                      LOS_PREVLOS | LOS_CONTRADAR));
	}
	else if (losHandler->InRadar(this, at)) {
		newStatus |=  (mask & LOS_INRADAR);
		newStatus &= ~(mask & LOS_INLOS);
	}
	else {
		newStatus &= ~(mask & (LOS_INLOS | LOS_INRADAR | LOS_CONTRADAR));
	}

	return newStatus;
}


void CUnit::UpdateLosStatus(int at)
{
	const unsigned short currStatus = losStatus[at];
	if ((currStatus & LOS_ALL_MASK_BITS) == LOS_ALL_MASK_BITS) {
		return; // no need to update, all changes are masked
	}
	SetLosStatus(at, CalcLosStatus(at));
}


void CUnit::SetStunned(bool stun) {
	stunned = stun;

	if (moveType->progressState == AMoveType::Active) {
		if (stunned) {
			script->StopMoving();
		} else {
			script->StartMoving(moveType->IsReversing());
		}
	}

	eventHandler.UnitStunned(this, stun);
}


void CUnit::SlowUpdate()
{
	UpdatePosErrorParams(false, true);
	DoWaterDamage();

	if (health < 0.0f) {
		KillUnit(nullptr, false, true);
		return;
	}

	repairAmount = 0.0f;

	if (paralyzeDamage > 0.0f) {
		// NOTE: the paralysis degradation-rate has to vary, because
		// when units are paralyzed based on their current health (in
		// DoDamage) we potentially start decaying from a lower damage
		// level and would otherwise be de-paralyzed more quickly than
		// specified by <paralyzeTime>
		paralyzeDamage -= ((modInfo.paralyzeOnMaxHealth? maxHealth: health) * (UNIT_SLOWUPDATE_RATE / float(GAME_SPEED)) * CUnit::empDeclineRate);
		paralyzeDamage = std::max(paralyzeDamage, 0.0f);
	}

	UpdateResources();

	if (IsStunned()) {
		// call this because we can be pushed into a different quad while stunned
		// which would make us invulnerable to most non-/small-AOE weapon impacts
		static_cast<AMoveType*>(moveType)->SlowUpdate();

		const bool notStunned = (paralyzeDamage <= (modInfo.paralyzeOnMaxHealth? maxHealth: health));
		const bool inFireBase = (transporter == nullptr || !transporter->unitDef->IsTransportUnit() || transporter->unitDef->isFirePlatform);

		// de-stun only if we are not (still) inside a non-firebase transport
		if (notStunned && inFireBase)
			SetStunned(false);

		SlowUpdateCloak(true);
		return;
	}

	if (selfDCountdown > 0) {
		if ((selfDCountdown -= 1) == 0) {
			// avoid unfinished buildings making an explosion
			KillUnit(nullptr, !beingBuilt, beingBuilt);
			return;
		}

		if ((selfDCountdown & 1) && (team == gu->myTeam) && !gu->spectating)
			LOG("%s: self-destruct in %is", unitDef->humanName.c_str(), (selfDCountdown >> 1) + 1);
	}

	if (beingBuilt) {
		if (modInfo.constructionDecay && (lastNanoAdd < (gs->frameNum - modInfo.constructionDecayTime))) {
			float buildDecay = buildTime * modInfo.constructionDecaySpeed;

			buildDecay = 1.0f / std::max(0.001f, buildDecay);
			buildDecay = std::min(buildProgress, buildDecay);

			health         = std::max(0.0f, health - maxHealth * buildDecay);
			buildProgress -= buildDecay;

			AddMetal(cost.metal * buildDecay, false);

			if (health <= 0.0f || buildProgress <= 0.0f)
				KillUnit(nullptr, false, true);
		}

		ScriptDecloak(nullptr, nullptr);
		return;
	}

	// below is stuff that should not be run while being built
	commandAI->SlowUpdate();
	moveType->SlowUpdate();


	// FIXME: scriptMakeMetal ...?
	AddMetal(resourcesUncondMake.metal);
	AddEnergy(resourcesUncondMake.energy);
	UseMetal(resourcesUncondUse.metal);
	UseEnergy(resourcesUncondUse.energy);

	if (activated) {
		if (UseMetal(resourcesCondUse.metal))
			AddEnergy(resourcesCondMake.energy);

		if (UseEnergy(resourcesCondUse.energy))
			AddMetal(resourcesCondMake.metal);

	}

	AddMetal(unitDef->metalMake * 0.5f);

	if (activated) {
		if (UseEnergy(unitDef->energyUpkeep * 0.5f)) {
			AddMetal(unitDef->makesMetal * 0.5f);

			if (unitDef->extractsMetal > 0.0f)
				AddMetal(metalExtract * 0.5f);
		}

		UseMetal(unitDef->metalUpkeep * 0.5f);

		if (unitDef->windGenerator > 0.0f) {
			if (wind.GetCurrentStrength() > unitDef->windGenerator) {
 				AddEnergy(unitDef->windGenerator * 0.5f);
			} else {
 				AddEnergy(wind.GetCurrentStrength() * 0.5f);
			}
		}
	}

	AddEnergy(energyTickMake * 0.5f);


	if (health < maxHealth) {
		health += (unitDef->idleAutoHeal * (restTime > unitDef->idleTime));
		health += unitDef->autoHeal;
		health = std::min(health, maxHealth);
	}

	SlowUpdateCloak(false);

	if (unitDef->canKamikaze) {
		if (fireState >= FIRESTATE_FIREATWILL) {
			std::vector<int> nearbyUnits;
			if (unitDef->kamikazeUseLOS) {
				CGameHelper::GetEnemyUnits(pos, unitDef->kamikazeDist, allyteam, nearbyUnits);
			} else {
				CGameHelper::GetEnemyUnitsNoLosTest(pos, unitDef->kamikazeDist, allyteam, nearbyUnits);
			}

			for (auto it = nearbyUnits.cbegin(); it != nearbyUnits.cend(); ++it) {
				const CUnit* victim = unitHandler.GetUnitUnsafe(*it);
				const float3 dif = pos - victim->pos;

				if (dif.SqLength() < Square(unitDef->kamikazeDist)) {
					if (victim->speed.dot(dif) <= 0.0f) {
						// self destruct when target starts moving away from us, should maximize damage
						KillUnit(nullptr, true, false);
						return;
					}
				}
			}
		}

		if (
			   (curTarget.type == Target_Unit && (curTarget.unit->pos.SqDistance(pos) < Square(unitDef->kamikazeDist)))
			|| (curTarget.type == Target_Pos  && (curTarget.groundPos.SqDistance(pos) < Square(unitDef->kamikazeDist)))
		) {
			KillUnit(nullptr, true, false);
			return;
		}
	}

	if (moveType->progressState == AMoveType::Active) {
		if (seismicSignature && !GetTransporter()) {
			DoSeismicPing((int)seismicSignature);
		}
	}

	CalculateTerrainType();
	UpdateTerrainType();
}


bool CUnit::CanUpdateWeapons() const
{
	return (!beingBuilt && !IsStunned() && !dontUseWeapons && !dontFire && !isDead);
}


void CUnit::SlowUpdateWeapons()
{
	if (!CanUpdateWeapons())
		return;

	for (CWeapon* w: weapons) {
		w->SlowUpdate();
	}
}


bool CUnit::HaveTarget() const
{
	return (curTarget.type != Target_None);
//	for (const CWeapon* w: weapons) {
//		if (w->HaveTarget())
//			return true;
//	}
//	return false;
}


float CUnit::GetFlankingDamageBonus(const float3& attackDir)
{
	float flankingBonus = 1.0f;

	if (flankingBonusMode <= 0)
		return flankingBonus;

	if (flankingBonusMode == 1) {
		// mode 1 = global coordinates, mobile
		flankingBonusDir += (attackDir * flankingBonusMobility);
		flankingBonusDir.Normalize();
		flankingBonusMobility = 0.0f;
		flankingBonus = (flankingBonusAvgDamage - attackDir.dot(flankingBonusDir) * flankingBonusDifDamage);
	} else {
		float3 adirRelative;
		adirRelative.x = attackDir.dot(rightdir);
		adirRelative.y = attackDir.dot(updir);
		adirRelative.z = attackDir.dot(frontdir);

		if (flankingBonusMode == 2) {
			// mode 2 = unit coordinates, mobile
			flankingBonusDir += (adirRelative * flankingBonusMobility);
			flankingBonusDir.Normalize();
			flankingBonusMobility = 0.0f;
		}

		// modes 2 and 3 both use this; 3 is unit coordinates, immobile
		flankingBonus = (flankingBonusAvgDamage - adirRelative.dot(flankingBonusDir) * flankingBonusDifDamage);
	}

	return flankingBonus;
}

void CUnit::DoWaterDamage()
{
	if (mapInfo->water.damage <= 0.0f)
		return;
	if (!pos.IsInBounds())
		return;
	// note: hovercraft could also use a negative waterline
	// ("hoverline"?) to avoid being damaged but that would
	// confuse GMTPathController --> damage must be removed
	// via UnitPreDamaged if not wanted
	if (!IsInWater())
		return;

	DoDamage(DamageArray(mapInfo->water.damage), ZeroVector, NULL, -DAMAGE_EXTSOURCE_WATER, -1);
}



static void AddUnitDamageStats(CUnit* unit, float damage, bool dealt)
{
	if (unit == nullptr)
		return;

	CTeam* team = teamHandler.Team(unit->team);
	TeamStatistics& stats = team->GetCurrentStats();

	if (dealt) {
		stats.damageDealt += damage;
	} else {
		stats.damageReceived += damage;
	}
}

void CUnit::ApplyDamage(CUnit* attacker, const DamageArray& damages, float& baseDamage, float& experienceMod)
{
	if (damages.paralyzeDamageTime == 0) {
		// real damage
		if (baseDamage > 0.0f) {
			// do not log overkill damage, so nukes etc do not inflate values
			AddUnitDamageStats(attacker, Clamp(maxHealth - health, 0.0f, baseDamage), true);
			AddUnitDamageStats(this, Clamp(maxHealth - health, 0.0f, baseDamage), false);

			health -= baseDamage;
		} else {
			// healing
			health -= baseDamage;
			health = std::min(health, maxHealth);

			if (health > paralyzeDamage && !modInfo.paralyzeOnMaxHealth) {
				SetStunned(false);
			}
		}
	} else {
		// paralyzation damage (adds reduced experience for the attacker)
		experienceMod *= 0.1f;

		// paralyzeDamage may not get higher than baseHealth * (paralyzeTime + 1),
		// which means the unit will be destunned after <paralyzeTime> seconds.
		// (maximum paralyzeTime of all paralyzer weapons which recently hit it ofc)
		//
		// rate of paralysis-damage reduction is lower if the unit has less than
		// maximum health to ensure stun-time is always equal to <paralyzeTime>
		const float baseHealth = (modInfo.paralyzeOnMaxHealth? maxHealth: health);
		const float paralysisDecayRate = baseHealth * CUnit::empDeclineRate;
		const float sumParalysisDamage = paralysisDecayRate * damages.paralyzeDamageTime;
		const float maxParalysisDamage = std::max(baseHealth + sumParalysisDamage - paralyzeDamage, 0.0f);

		if (baseDamage > 0.0f) {
			// clamp the dealt paralysis-damage to [0, maxParalysisDamage]
			baseDamage = Clamp(baseDamage, 0.0f, maxParalysisDamage);

			// no attacker gains experience from a stunned target
			experienceMod *= (1 - IsStunned());
			// increase the current level of paralysis-damage
			paralyzeDamage += baseDamage;

			if (paralyzeDamage >= baseHealth) {
				SetStunned(true);
			}
		} else {
			// no experience from healing a non-stunned target
			experienceMod *= (paralyzeDamage > 0.0f);
			// decrease ("heal") the current level of paralysis-damage
			paralyzeDamage += baseDamage;
			paralyzeDamage = std::max(paralyzeDamage, 0.0f);

			if (paralyzeDamage <= baseHealth) {
				SetStunned(false);
			}
		}
	}

	recentDamage += baseDamage;
}

void CUnit::DoDamage(
	const DamageArray& damages,
	const float3& impulse,
	CUnit* attacker,
	int weaponDefID,
	int projectileID
) {
	if (isDead)
		return;
	if (IsCrashing() || IsInVoid())
		return;

	float baseDamage = damages.Get(armorType);
	float experienceMod = expMultiplier;
	float impulseMult = 1.0f;

	const bool isCollision = (weaponDefID == -CSolidObject::DAMAGE_COLLISION_OBJECT || weaponDefID == -CSolidObject::DAMAGE_COLLISION_GROUND);
	const bool isParalyzer = (damages.paralyzeDamageTime != 0);

	if (!isCollision && baseDamage > 0.0f) {
		if (attacker != nullptr) {
			SetLastAttacker(attacker);

			// FIXME -- not the impulse direction?
			baseDamage *= GetFlankingDamageBonus((attacker->pos - pos).SafeNormalize());
		}

		baseDamage *= curArmorMultiple;
		restTime = 0; // bleeding != resting
	}

	if (eventHandler.UnitPreDamaged(this, attacker, baseDamage, weaponDefID, projectileID, isParalyzer, &baseDamage, &impulseMult))
		return;

	script->WorldHitByWeapon(-(impulse * impulseMult).SafeNormalize2D(), weaponDefID, /*inout*/ baseDamage);
	ApplyImpulse((impulse * impulseMult) / mass);
	ApplyDamage(attacker, damages, baseDamage, experienceMod);

	{
		eventHandler.UnitDamaged(this, attacker, baseDamage, weaponDefID, projectileID, isParalyzer);

		// unit might have been killed via Lua from within UnitDamaged (e.g.
		// through a recursive DoDamage call from AddUnitDamage or directly
		// via DestroyUnit); can skip the rest
		if (isDead)
			return;

		eoh->UnitDamaged(*this, attacker, baseDamage, weaponDefID, projectileID, isParalyzer);
	}

#ifdef TRACE_SYNC
	tracefile << "Damage: ";
	tracefile << id << " " << baseDamage << "\n";
#endif

	if (!isCollision && baseDamage > 0.0f) {
		if ((attacker != nullptr) && !teamHandler.Ally(allyteam, attacker->allyteam)) {
			const float scaledExpMod = 0.1f * experienceMod * (power / attacker->power);
			const float scaledDamage = std::max(0.0f, (baseDamage + std::min(0.0f, health))) / maxHealth;
			// alternative
			// scaledDamage = (max(healthPreDamage, 0) - max(health, 0)) / maxHealth

			attacker->AddExperience(scaledExpMod * scaledDamage);
		}
	}

	if (health > 0.0f)
		return;

	KillUnit(attacker, false, false);

	if (!isDead)
		return;
	if (beingBuilt)
		return;
	if (attacker == nullptr)
		return;

	if (teamHandler.Ally(allyteam, attacker->allyteam))
		return;

	CTeam* attackerTeam = teamHandler.Team(attacker->team);
	TeamStatistics& attackerStats = attackerTeam->GetCurrentStats();

	attackerStats.unitsKilled += (1 - isCollision);
}



void CUnit::ApplyImpulse(const float3& impulse) {
	if (GetTransporter() != nullptr) {
		// transfer impulse to unit transporting us, scaled by its mass
		// assume we came here straight from DoDamage, not LuaSyncedCtrl
		GetTransporter()->ApplyImpulse((impulse * mass) / (GetTransporter()->mass));
		return;
	}

	const float3& groundNormal = CGround::GetNormal(pos.x, pos.z);
	const float groundImpulseScale = std::min(0.0f, impulse.dot(groundNormal));
	const float3 modImpulse = impulse - (groundNormal * groundImpulseScale * IsOnGround());

	if (moveType->CanApplyImpulse(modImpulse)) {
		CSolidObject::ApplyImpulse(modImpulse);
	}
}



/******************************************************************************/
/******************************************************************************/

CMatrix44f CUnit::GetTransformMatrix(bool synced, bool fullread) const
{
	float3 interPos = synced ? pos : drawPos;

	if (!synced && !fullread && !gu->spectatingFullView)
		interPos += GetErrorVector(gu->myAllyTeam);

	return (ComposeMatrix(interPos));
}

/******************************************************************************/
/******************************************************************************/

void CUnit::AddExperience(float exp)
{
	if (exp == 0.0f)
		return;

	assert((experience + exp) >= 0.0f);

	const float oldExperience = experience;
	const float oldMaxHealth = maxHealth;

	experience += exp;
	limExperience = experience / (experience + 1.0f);

	if (expGrade != 0.0f) {
		const int oldGrade = (int)(oldExperience / expGrade);
		const int newGrade = (int)(   experience / expGrade);
		if (oldGrade != newGrade) {
			eventHandler.UnitExperience(this, oldExperience);
		}
	}

	if (expPowerScale > 0.0f)
		power = unitDef->power * (1.0f + (limExperience * expPowerScale));

	if (expReloadScale > 0.0f)
		reloadSpeed = (1.0f + (limExperience * expReloadScale));

	if (expHealthScale > 0.0f) {
		maxHealth = std::max(0.1f, unitDef->health * (1.0f + (limExperience * expHealthScale)));
		health *= (maxHealth / oldMaxHealth);
	}
}


void CUnit::SetMass(float newMass)
{
	if (transporter != nullptr)
		transporter->SetMass(transporter->mass + (newMass - mass));

	CSolidObject::SetMass(newMass);
}


void CUnit::DoSeismicPing(float pingSize)
{
	float rx = gsRNG.NextFloat();
	float rz = gsRNG.NextFloat();

	if (!(losStatus[gu->myAllyTeam] & LOS_INLOS) && losHandler->InSeismicDistance(this, gu->myAllyTeam)) {
		const float3 err = float3(0.5f - rx, 0.0f, 0.5f - rz) * losHandler->GetAllyTeamRadarErrorSize(gu->myAllyTeam);

		projMemPool.alloc<CSeismicGroundFlash>(pos + err, 30, 15, 0, pingSize, 1, float3(0.8f, 0.0f, 0.0f));
	}
	for (int a = 0; a < teamHandler.ActiveAllyTeams(); ++a) {
		if (losHandler->InSeismicDistance(this, a)) {
			const float3 err = float3(0.5f - rx, 0.0f, 0.5f - rz) * losHandler->GetAllyTeamRadarErrorSize(a);
			const float3 pingPos = (pos + err);
			eventHandler.UnitSeismicPing(this, a, pingPos, pingSize);
			eoh->SeismicPing(a, *this, pingPos, pingSize);
		}
	}
}


void CUnit::ChangeLos(int losRad, int airRad)
{
	losRadius = losRad;
	airLosRadius = airRad;
}


bool CUnit::ChangeTeam(int newteam, ChangeType type)
{
	if (isDead)
		return false;

	// do not allow unit count violations due to team swapping
	// (this includes unit captures)
	if (unitHandler.NumUnitsByTeamAndDef(newteam, unitDef->id) >= unitDef->maxThisUnit)
		return false;

	if (!eventHandler.AllowUnitTransfer(this, newteam, type == ChangeCaptured))
		return false;

	// do not allow old player to keep controlling the unit
	if (fpsControlPlayer != nullptr) {
		fpsControlPlayer->StopControllingUnit();
		assert(fpsControlPlayer == nullptr);
	}

	const int oldteam = team;

	selectedUnitsHandler.RemoveUnit(this);
	SetGroup(nullptr);

	eventHandler.UnitTaken(this, oldteam, newteam);
	eoh->UnitCaptured(*this, oldteam, newteam);

	// remove for old allyteam
	quadField.RemoveUnit(this);


	if (type == ChangeGiven) {
		teamHandler.Team(oldteam)->RemoveUnit(this, CTeam::RemoveGiven);
		teamHandler.Team(newteam)->AddUnit(this,    CTeam::AddGiven);
	} else {
		teamHandler.Team(oldteam)->RemoveUnit(this, CTeam::RemoveCaptured);
		teamHandler.Team(newteam)->AddUnit(this,    CTeam::AddCaptured);
	}

	if (!beingBuilt) {
		teamHandler.Team(oldteam)->resStorage.metal  -= storage.metal;
		teamHandler.Team(oldteam)->resStorage.energy -= storage.energy;

		teamHandler.Team(newteam)->resStorage.metal  += storage.metal;
		teamHandler.Team(newteam)->resStorage.energy += storage.energy;
	}


	team = newteam;
	allyteam = teamHandler.AllyTeam(newteam);
	neutral = false;

	unitHandler.ChangeUnitTeam(this, oldteam, newteam);

	for (int at = 0; at < teamHandler.ActiveAllyTeams(); ++at) {
		if (teamHandler.Ally(at, allyteam)) {
			SetLosStatus(at, LOS_ALL_MASK_BITS | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR);
		} else {
			// re-calc LOS status
			losStatus[at] = 0;
			UpdateLosStatus(at);
		}
	}

	// insert for new allyteam
	quadField.MovedUnit(this);

	eventHandler.UnitGiven(this, oldteam, newteam);
	eoh->UnitGiven(*this, oldteam, newteam);

	// reset states and clear the queues
	if (!teamHandler.AlliedTeams(oldteam, newteam))
		ChangeTeamReset();

	return true;
}


void CUnit::ChangeTeamReset()
{
	// stop friendly units shooting at us
	std::vector<CUnit*> alliedunits;

	for (const auto& objs: GetAllListeners()) {
		for (CObject* obj: objs) {
			CUnit* u = dynamic_cast<CUnit*>(obj);

			if (u == nullptr)
				continue;
			if (!teamHandler.AlliedTeams(team, u->team))
				continue;

			alliedunits.push_back(u);
		}
	}
	for (auto ui = alliedunits.cbegin(); ui != alliedunits.cend(); ++ui) {
		(*ui)->StopAttackingAllyTeam(allyteam);
	}
	// and stop shooting at friendly ally teams
	for (int t = 0; t < teamHandler.ActiveAllyTeams(); ++t) {
		if (teamHandler.Ally(t, allyteam))
			StopAttackingAllyTeam(t);
	}

	// clear the commands (newUnitCommands for factories)
	Command c(CMD_STOP);
	commandAI->GiveCommand(c);

	// clear the build commands for factories
	CFactoryCAI* facAI = dynamic_cast<CFactoryCAI*>(commandAI);
	if (facAI) {
		const unsigned char options = RIGHT_MOUSE_KEY; // clear option
		CCommandQueue& buildCommands = facAI->commandQue;
		std::vector<Command> clearCommands;
		clearCommands.reserve(buildCommands.size());
		for (auto& cmd: buildCommands) {
			clearCommands.emplace_back(cmd.GetID(), options);
		}
		for (auto& cmd: clearCommands) {
			facAI->GiveCommand(cmd);
		}
	}

	//FIXME reset to unitdef defaults

	// deactivate to prevent the old give metal maker trick
	// TODO remove, because it is *A specific
	c = Command(CMD_ONOFF, 0, 0); // always off
	commandAI->GiveCommand(c);

	// reset repeat state
	c = Command(CMD_REPEAT, 0, 0);
	commandAI->GiveCommand(c);

	// reset cloak state
	if (unitDef->canCloak) {
		c = Command(CMD_CLOAK, 0, 0); // always off
		commandAI->GiveCommand(c);
	}
	// reset move state
	if (unitDef->canmove || unitDef->builder) {
		c = Command(CMD_MOVE_STATE, 0, MOVESTATE_MANEUVER);
		commandAI->GiveCommand(c);
	}
	// reset fire state
	if (commandAI->CanChangeFireState()) {
		c = Command(CMD_FIRE_STATE, 0, FIRESTATE_FIREATWILL);
		commandAI->GiveCommand(c);
	}
	// reset trajectory state
	if (unitDef->highTrajectoryType > 1) {
		c = Command(CMD_TRAJECTORY, 0, 0);
		commandAI->GiveCommand(c);
	}
}

bool CUnit::FloatOnWater() const {
	if (moveDef != nullptr)
		return (moveDef->FloatOnWater());

	// aircraft or building
	return (unitDef->floatOnWater);
}

bool CUnit::IsIdle() const
{
	if (beingBuilt)
		return false;

	return (commandAI->commandQue.empty());
}


bool CUnit::AttackUnit(CUnit* targetUnit, bool isUserTarget, bool wantManualFire, bool fpsMode)
{
	if (targetUnit == this) {
		// don't target ourself
		return false;
	}

	if (targetUnit == nullptr) {
		DropCurrentAttackTarget();
		return false;
	}

	SWeaponTarget newTarget = SWeaponTarget(targetUnit, isUserTarget);
	newTarget.isManualFire = wantManualFire || fpsMode;

	if (curTarget != newTarget) {
		DropCurrentAttackTarget();
		curTarget = newTarget;
		AddDeathDependence(targetUnit, DEPENDENCE_TARGET);
	}

	bool ret = false;
	for (CWeapon* w: weapons) {
		ret |= w->Attack(curTarget);
	}
	return ret;
}

bool CUnit::AttackGround(const float3& pos, bool isUserTarget, bool wantManualFire, bool fpsMode)
{
	SWeaponTarget newTarget = SWeaponTarget(pos, isUserTarget);
	newTarget.isManualFire = wantManualFire || fpsMode;

	if (curTarget != newTarget) {
		DropCurrentAttackTarget();
		curTarget = newTarget;
	}

	bool ret = false;
	for (CWeapon* w: weapons) {
		ret |= w->Attack(curTarget);
	}
	return ret;
}


void CUnit::DropCurrentAttackTarget()
{
	if (curTarget.type == Target_Unit)
		DeleteDeathDependence(curTarget.unit, DEPENDENCE_TARGET);

	for (CWeapon* w: weapons) {
		if (w->GetCurrentTarget() == curTarget)
			w->DropCurrentTarget();
	}

	curTarget = SWeaponTarget();
}


bool CUnit::SetSoloBuilder(CUnit* builder, const UnitDef* buildeeDef)
{
	if (builder == nullptr)
		return false;
	if (buildeeDef->canBeAssisted)
		return false;

	AddDeathDependence(soloBuilder = builder, DEPENDENCE_BUILDER);
	return true;
}

void CUnit::SetLastAttacker(CUnit* attacker)
{
	assert(attacker != nullptr);

	if (teamHandler.AlliedTeams(team, attacker->team))
		return;

	if (lastAttacker != nullptr)
		DeleteDeathDependence(lastAttacker, DEPENDENCE_ATTACKER);

	lastAttackFrame = gs->frameNum;
	lastAttacker = attacker;

	AddDeathDependence(attacker, DEPENDENCE_ATTACKER);
}

void CUnit::DependentDied(CObject* o)
{
	TransporteeKilled(o);

	if (o == curTarget.unit)
		DropCurrentAttackTarget();
	if (o == soloBuilder)
		soloBuilder = nullptr;
	if (o == transporter)
		transporter  = nullptr;
	if (o == lastAttacker)
		lastAttacker = nullptr;

	const auto missileIter = std::find(incomingMissiles.begin(), incomingMissiles.end(), static_cast<CMissileProjectile*>(o));

	if (missileIter != incomingMissiles.end())
		*missileIter = nullptr;

	CSolidObject::DependentDied(o);
}



void CUnit::UpdatePhysicalState(float eps)
{
	const bool inAir   = IsInAir();
	const bool inWater = IsInWater();

	CSolidObject::UpdatePhysicalState(eps);

	if (IsInAir() != inAir) {
		if (IsInAir()) {
			eventHandler.UnitEnteredAir(this);
		} else {
			eventHandler.UnitLeftAir(this);
		}
	}
	if (IsInWater() != inWater) {
		if (IsInWater()) {
			eventHandler.UnitEnteredWater(this);
		} else {
			eventHandler.UnitLeftWater(this);
		}
	}
}

void CUnit::UpdateTerrainType()
{
	if (curTerrainType != lastTerrainType) {
		script->SetSFXOccupy(curTerrainType);
		lastTerrainType = curTerrainType;
	}
}

void CUnit::CalculateTerrainType()
{
	enum {
		SFX_TERRAINTYPE_NONE    = 0,
		SFX_TERRAINTYPE_WATER_A = 1,
		SFX_TERRAINTYPE_WATER_B = 2,
		SFX_TERRAINTYPE_LAND    = 4,
	};

	// optimization: there's only about one unit that actually needs this information
	// ==> why are we even bothering with it? the callin parameter barely makes sense
	if (!script->HasSetSFXOccupy())
		return;

	if (GetTransporter() != NULL) {
		curTerrainType = SFX_TERRAINTYPE_NONE;
		return;
	}

	const float height = CGround::GetApproximateHeight(pos.x, pos.z);

	// water
	if (height < -5.0f) {
		if (upright)
			curTerrainType = SFX_TERRAINTYPE_WATER_B;
		else
			curTerrainType = SFX_TERRAINTYPE_WATER_A;
	}
	// shore
	else if (height < 0.0f) {
		if (upright)
			curTerrainType = SFX_TERRAINTYPE_WATER_A;
	}
	// land (or air)
	else {
		curTerrainType = SFX_TERRAINTYPE_LAND;
	}
}


bool CUnit::SetGroup(CGroup* newGroup, bool fromFactory, bool autoSelect)
{
	// factory is not necessarily selected
	if (fromFactory && !selectedUnitsHandler.AutoAddBuiltUnitsToFactoryGroup())
		return false;

	if (group != nullptr)
		group->RemoveUnit(this);

	if ((group = newGroup) == nullptr)
		return true;

	if (!newGroup->AddUnit(this)) {
		// group did not accept us
		group = nullptr;
		return false;
	}

	if (!autoSelect)
		return true;

	// add unit to the set of selected units iff its new group is already selected
	// and (user wants the unit to be auto-selected or the unit is not newly built)
	if (selectedUnitsHandler.IsGroupSelected(newGroup->id) && (selectedUnitsHandler.AutoAddBuiltUnitsToSelectedGroup() || !fromFactory))
		selectedUnitsHandler.AddUnit(this);

	return true;
}


/******************************************************************************/
/******************************************************************************/

bool CUnit::AddBuildPower(CUnit* builder, float amount)
{
	if (isDead || IsCrashing())
		return false;

	// stop decaying on building AND reclaim
	lastNanoAdd = gs->frameNum;

	CTeam* builderTeam = teamHandler.Team(builder->team);

	if (amount >= 0.0f) {
		// build or repair
		if (!beingBuilt && (health >= maxHealth))
			return false;

		if (beingBuilt) {
			// build
			const float step = std::min(amount / buildTime, 1.0f - buildProgress);
			const float metalCostStep  = cost.metal  * step;
			const float energyCostStep = cost.energy * step;

			if (builderTeam->res.metal < metalCostStep || builderTeam->res.energy < energyCostStep) {
				// update the energy and metal required counts
				builderTeam->resPull.metal  += metalCostStep;
				builderTeam->resPull.energy += energyCostStep;
				return false;
			}

			if (!eventHandler.AllowUnitBuildStep(builder, this, step))
				return false;

			if (builder->UseMetal(metalCostStep)) {
				// FIXME eventHandler.AllowUnitBuildStep() may have changed the storages!!! so the checks can be invalid!
				// TODO add a builder->UseResources(SResources(cost.metalStep, cost.energyStep))
				if (builder->UseEnergy(energyCostStep)) {
					health += (maxHealth * step);
					health = std::min(health, maxHealth);
					buildProgress += step;

					if (buildProgress >= 1.0f) {
						FinishedBuilding(false);
					}
				} else {
					// refund already-deducted metal if *energy* cost cannot be
					builder->UseMetal(-metalCostStep);
				}
			}

			return true;
		}
		else if (health < maxHealth) {
			// repair
			const float step = std::min(amount / buildTime, 1.0f - (health / maxHealth));
			const float energyUse = (cost.energy * step);
			const float energyUseScaled = energyUse * modInfo.repairEnergyCostFactor;

			if ((builderTeam->res.energy < energyUseScaled)) {
				// update the energy and metal required counts
				builderTeam->resPull.energy += energyUseScaled;
				return false;
			}

			if (!eventHandler.AllowUnitBuildStep(builder, this, step))
				return false;

	  		if (!builder->UseEnergy(energyUseScaled)) {
				return false;
			}

			repairAmount += amount;
			health += (maxHealth * step);
			health = std::min(health, maxHealth);

			return true;
		}
	} else {
		// reclaim
		if (!AllowedReclaim(builder)) {
			builder->DependentDied(this);
			return false;
		}

		const float step = std::max(amount / buildTime, -buildProgress);
		const float energyRefundStep = cost.energy * step;
		const float metalRefundStep  =  cost.metal * step;
		const float metalRefundStepScaled  =  metalRefundStep * modInfo.reclaimUnitEfficiency;
		const float energyRefundStepScaled = energyRefundStep * modInfo.reclaimUnitEnergyCostFactor;
		const float healthStep        = maxHealth * step;
		const float buildProgressStep = int(modInfo.reclaimUnitMethod == 0) * step;
		const float postHealth        = health + healthStep;
		const float postBuildProgress = buildProgress + buildProgressStep;

		if (!eventHandler.AllowUnitBuildStep(builder, this, step))
			return false;

		restTime = 0;

		bool killMe = false;
		SResourceOrder order;
		order.quantum    = false;
		order.overflow   = true;
		order.use.energy = -energyRefundStepScaled;
		if (modInfo.reclaimUnitMethod == 0) {
			// gradual reclamation of invested metal
			order.add.metal = -metalRefundStepScaled;
		} else {
			// lump reclamation of invested metal
			if (postHealth <= 0.0f || postBuildProgress <= 0.0f) {
				order.add.metal = (cost.metal * buildProgress) * modInfo.reclaimUnitEfficiency;
				killMe = true; // to make 100% sure the unit gets killed, and so no resources are reclaimed twice!
			}
		}

		if (!builder->IssueResourceOrder(&order)) {
			return false;
		}

		// turn reclaimee into nanoframe (even living units)
		if ((modInfo.reclaimUnitMethod == 0) && !beingBuilt) {
			beingBuilt = true;
			SetMetalStorage(0);
			SetEnergyStorage(0);
			eventHandler.UnitReverseBuilt(this);
		}

		// reduce health & resources
		health = postHealth;
		buildProgress = postBuildProgress;

		// reclaim finished?
		if (killMe || buildProgress <= 0.0f || health <= 0.0f) {
			health = 0.0f;
			buildProgress = 0.0f;
			KillUnit(nullptr, false, true);
			return false;
		}

		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////
//

void CUnit::SetMetalStorage(float newStorage)
{
	teamHandler.Team(team)->resStorage.metal -= storage.metal;
	storage.metal = newStorage;
	teamHandler.Team(team)->resStorage.metal += storage.metal;
}


void CUnit::SetEnergyStorage(float newStorage)
{
	teamHandler.Team(team)->resStorage.energy -= storage.energy;
	storage.energy = newStorage;
	teamHandler.Team(team)->resStorage.energy += storage.energy;
}


bool CUnit::AllowedReclaim(CUnit* builder) const
{
	// Don't allow the reclaim if the unit is finished and we arent allowed to reclaim it
	if (!beingBuilt) {
		if (allyteam == builder->allyteam) {
			if ((team != builder->team) && (!modInfo.reclaimAllowAllies)) return false;
		} else {
			if (!modInfo.reclaimAllowEnemies) return false;
		}
	}

	return true;
}


bool CUnit::UseMetal(float metal)
{
	if (metal < 0.0f) {
		AddMetal(-metal);
		return true;
	}

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull.metal += metal;

	if (myTeam->UseMetal(metal)) {
		resourcesUseI.metal += metal;
		return true;
	}

	return false;
}

void CUnit::AddMetal(float metal, bool useIncomeMultiplier)
{
	if (metal < 0.0f) {
		UseMetal(-metal);
		return;
	}

	resourcesMakeI.metal += metal;
	teamHandler.Team(team)->AddMetal(metal, useIncomeMultiplier);
}


bool CUnit::UseEnergy(float energy)
{
	if (energy < 0.0f) {
		AddEnergy(-energy);
		return true;
	}

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull.energy += energy;

	if (myTeam->UseEnergy(energy)) {
		resourcesUseI.energy += energy;
		return true;
	}

	return false;
}

void CUnit::AddEnergy(float energy, bool useIncomeMultiplier)
{
	if (energy < 0.0f) {
		UseEnergy(-energy);
		return;
	}
	resourcesMakeI.energy += energy;
	teamHandler.Team(team)->AddEnergy(energy, useIncomeMultiplier);
}


bool CUnit::AddHarvestedMetal(float metal)
{
	if (harvestStorage.metal <= 0.0f) {
		AddMetal(metal, false);
		return true;
	}

	if (harvested.metal >= harvestStorage.metal) {
		eventHandler.UnitHarvestStorageFull(this);
		return false;
	}

	//FIXME what do with exceeding metal?
	harvested.metal = std::min(harvested.metal + metal, harvestStorage.metal);
	if (harvested.metal >= harvestStorage.metal) {
		eventHandler.UnitHarvestStorageFull(this);
	}
	return true;
}


void CUnit::SetStorage(const SResourcePack& newStorage)
{
	teamHandler.Team(team)->resStorage -= storage;
	storage = newStorage;
	teamHandler.Team(team)->resStorage += storage;
}


bool CUnit::HaveResources(const SResourcePack& pack) const
{
	return teamHandler.Team(team)->HaveResources(pack);
}


bool CUnit::UseResources(const SResourcePack& pack)
{
	//FIXME
	/*if (energy < 0.0f) {
		AddEnergy(-energy);
		return true;
	}*/

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull += pack;

	if (myTeam->UseResources(pack)) {
		resourcesUseI += pack;
		return true;
	}
	return false;
}


void CUnit::AddResources(const SResourcePack& pack, bool useIncomeMultiplier)
{
	//FIXME
	/*if (energy < 0.0f) {
		UseEnergy(-energy);
		return true;
	}*/
	resourcesMakeI += pack;
	teamHandler.Team(team)->AddResources(pack, useIncomeMultiplier);
}


static bool CanDispatch(const CUnit* u, const CTeam* team, const SResourceOrder& order)
{
	const bool haveEnoughResources = (team->res >= order.use);
	bool canDispatch = haveEnoughResources;

	if (order.overflow)
		return canDispatch;

	if (u->harvestStorage.empty()) {
		const bool haveEnoughStorageFree = ((order.add + team->res) <= team->resStorage);
		canDispatch = canDispatch && haveEnoughStorageFree;
	} else {
		const bool haveEnoughHarvestStorageFree = ((order.add + u->harvested) <= u->harvestStorage);
		canDispatch = canDispatch && haveEnoughHarvestStorageFree;
	}

	return canDispatch;
}


static void GetScale(const float x1, const float x2, float* scale)
{
	const float v = std::min(x1, x2);
	*scale = (x1 == 0.0f) ? *scale : std::min(*scale, v / x1);
}


static bool LimitToFullStorage(const CUnit* u, const CTeam* team, SResourceOrder* order)
{
	float scales[SResourcePack::MAX_RESOURCES];

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		scales[i] = 1.0f;
		float& scale = order->separate ? scales[i] : scales[0];

		GetScale(order->use[i], team->res[i], &scale);

		if (u->harvestStorage.empty()) {
			GetScale(order->add[i], team->resStorage.res[i] - team->res[i], &scale);
		} else {
			GetScale(order->add[i], u->harvestStorage[i] - u->harvested[i], &scale);
		}
	}

	if (order->separate) {
		bool nonempty = false;
		for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
			if ((order->use[i] != 0.0f || order->add[i] != 0.0f) && scales[i] != 0.0f) nonempty = true;
			order->use[i] *= scales[i];
			order->add[i] *= scales[i];
		}
		return nonempty;
	}

	order->use *= scales[0];
	order->add *= scales[0];
	return (scales[0] != 0.0f);
}


bool CUnit::IssueResourceOrder(SResourceOrder* order)
{
	//FIXME assert(order.use.energy >= 0.0f && order.use.metal >= 0.0f);
	//FIXME assert(order.add.energy >= 0.0f && order.add.metal >= 0.0f);

	CTeam* myTeam = teamHandler.Team(team);
	myTeam->resPull += order->use;

	// check
	if (!CanDispatch(this, myTeam, *order)) {
		if (order->quantum)
			return false;

		if (!LimitToFullStorage(this, myTeam, order))
			return false;
	}

	// use
	if (!order->use.empty()) {
		UseResources(order->use);
	}

	// add
	if (!order->add.empty()) {
		if (harvestStorage.empty()) {
			AddResources(order->add);
		} else {
			bool isFull = false;
			for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
				if (order->add[i] > 0.0f) {
					harvested[i] += order->add[i];
					harvested[i]  = std::min(harvested[i], harvestStorage[i]);
					isFull |= (harvested[i] >= harvestStorage[i]);
				}
			}

			if (isFull) {
				eventHandler.UnitHarvestStorageFull(this);
			}
		}
	}

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CUnit::Activate()
{
	if (activated)
		return;

	activated = true;
	script->Activate();

	if (unitDef->targfac)
		losHandler->DecreaseAllyTeamRadarErrorSize(allyteam);

	if (IsInLosForAllyTeam(gu->myAllyTeam))
		Channels::General->PlayRandomSample(unitDef->sounds.activate, this);
}


void CUnit::Deactivate()
{
	if (!activated)
		return;

	activated = false;
	script->Deactivate();

	if (unitDef->targfac)
		losHandler->IncreaseAllyTeamRadarErrorSize(allyteam);

	if (IsInLosForAllyTeam(gu->myAllyTeam))
		Channels::General->PlayRandomSample(unitDef->sounds.deactivate, this);
}



void CUnit::UpdateWind(float x, float z, float strength)
{
	const float windHeading = ClampRad(GetHeadingFromVectorF(-x, -z) - heading * TAANG2RAD);
	const float windStrength = std::min(strength, unitDef->windGenerator);

	script->WindChanged(windHeading, windStrength);
}


void CUnit::IncomingMissile(CMissileProjectile* missile)
{
	if (!unitDef->canDropFlare)
		return;

	// always drop a flare for dramatic effect; only the
	// first <N> missiles have a chance to go after this
	if (lastFlareDrop < (gs->frameNum - unitDef->flareReloadTime * GAME_SPEED))
		projMemPool.alloc<CFlareProjectile>(pos, speed, this, int((lastFlareDrop = gs->frameNum) + unitDef->flareDelay * (1 + gsRNG.NextFloat()) * 15));

	const auto missileIter = std::find(incomingMissiles.begin(), incomingMissiles.end(), nullptr);

	if (missileIter == incomingMissiles.end())
		return;

	// no risk of duplicates; only caller is MissileProjectile ctor
	AddDeathDependence(*missileIter = missile, DEPENDENCE_INCOMING);
}



void CUnit::TempHoldFire(int cmdID)
{
	if (weapons.empty())
		return;
	if (!eventHandler.AllowBuilderHoldFire(this, cmdID))
		return;

	// block the SlowUpdateWeapons cycle
	dontFire = true;

	// clear current target (if any)
	AttackUnit(nullptr, false, false);
}


void CUnit::StopAttackingAllyTeam(int ally)
{
	if (lastAttacker != nullptr && lastAttacker->allyteam == ally) {
		DeleteDeathDependence(lastAttacker, DEPENDENCE_ATTACKER);
		lastAttacker = nullptr;
	}
	if (curTarget.type == Target_Unit && curTarget.unit->allyteam == ally)
		DropCurrentAttackTarget();

	commandAI->StopAttackingAllyTeam(ally);

	for (CWeapon* w: weapons) {
		w->StopAttackingAllyTeam(ally);
	}
}


bool CUnit::GetNewCloakState(bool stunCheck) {
	assert(wantCloak);

	// grab nearest enemy wrt our default decloak-distance
	// Lua code can do more elaborate scans if it wants to
	// and has access to cloakCost{Moving}/decloakDistance
	//
	// NB: for stun checks, set enemy to <this> instead of
	// a nullptr s.t. Lua can deduce the context
	const CUnit* closestEnemy = this;

	if (!stunCheck)
		closestEnemy = CGameHelper::GetClosestEnemyUnitNoLosTest(nullptr, midPos, decloakDistance, allyteam, unitDef->decloakSpherical, false);

	return (eventHandler.AllowUnitCloak(this, closestEnemy));
}


void CUnit::SlowUpdateCloak(bool stunCheck)
{
	const bool oldCloak = isCloaked;
	const bool newCloak = wantCloak && GetNewCloakState(stunCheck);

	if (oldCloak != newCloak) {
		if (newCloak) {
			eventHandler.UnitCloaked(this);
		} else {
			eventHandler.UnitDecloaked(this);
		}
	}

	isCloaked = newCloak;
}


#if 0
// no use for this currently
bool CUnit::ScriptCloak()
{
	if (isCloaked)
		return true;

	if (!eventHandler.AllowUnitCloak(this, nullptr, nullptr, nullptr))
		return false;

	wantCloak = true;
	isCloaked = true;

	eventHandler.UnitCloaked(this);
	return true;
}
#endif

bool CUnit::ScriptDecloak(const CSolidObject* object, const CWeapon* weapon)
{
	// horrific ScriptCloak asymmetry for Lua's sake
	// maintaining internal consistency requires the
	// decloak event to only fire if isCloaked
	if (!isCloaked)
		return (!wantCloak || eventHandler.AllowUnitDecloak(this, object, weapon));

	if (!eventHandler.AllowUnitDecloak(this, object, weapon))
		return false;

	// wantCloak = false;
	isCloaked = false;

	eventHandler.UnitDecloaked(this);
	return true;
}


/******************************************************************************/
/******************************************************************************/

bool CUnit::CanTransport(const CUnit* unit) const
{
	if (!unitDef->IsTransportUnit())
		return false;
	if (unit->GetTransporter() != nullptr)
		return false;

	if (!eventHandler.AllowUnitTransport(this, unit))
		return false;

	if (!unit->unitDef->transportByEnemy && !teamHandler.AlliedTeams(unit->team, team))
		return false;
	if (transportCapacityUsed >= unitDef->transportCapacity)
		return false;
	if (unit->unitDef->cantBeTransported)
		return false;

	// don't transport cloaked enemies
	if (unit->isCloaked && !teamHandler.AlliedTeams(unit->team, team))
		return false;

	if (unit->xsize > (unitDef->transportSize * SPRING_FOOTPRINT_SCALE))
		return false;
	if (unit->xsize < (unitDef->minTransportSize * SPRING_FOOTPRINT_SCALE))
		return false;

	if (unit->mass >= CSolidObject::DEFAULT_MASS || unit->beingBuilt)
		return false;
	if (unit->mass < unitDef->minTransportMass)
		return false;
	if ((unit->mass + transportMassUsed) > unitDef->transportMass)
		return false;

	if (!CanLoadUnloadAtPos(unit->pos, unit))
		return false;

	// check if <unit> is already (in)directly transporting <this>
	const CUnit* u = this;

	while (u != nullptr) {
		if (u == unit)
			return false;

		u = u->GetTransporter();
	}

	return true;
}


bool CUnit::AttachUnit(CUnit* unit, int piece, bool force)
{
	assert(unit != this);

	if (unit->GetTransporter() == this) {
		// assume we are already transporting this unit,
		// and just want to move it to a different piece
		// with script logic (this means the UnitLoaded
		// event is only sent once)
		//
		for (TransportedUnit& tu: transportedUnits) {
			if (tu.unit == unit) {
				tu.piece = piece;
				break;
			}
		}

		unit->UpdateVoidState(piece < 0);
		return false;
	}

	// handle transfers from another transport to us
	// (can still fail depending on CanTransport())
	if (unit->GetTransporter() != nullptr)
		unit->GetTransporter()->DetachUnit(unit);

	// covers the case where unit->transporter != NULL
	if (!force && !CanTransport(unit))
		return false;

	AddDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
	unit->AddDeathDependence(this, DEPENDENCE_TRANSPORTER);

	unit->SetTransporter(this);
	unit->loadingTransportId = -1;
	unit->SetStunned(!unitDef->isFirePlatform && unitDef->IsTransportUnit());
	unit->UpdateVoidState(piece < 0);

	// make sure unit does not fire etc in transport
	if (unit->IsStunned())
		selectedUnitsHandler.RemoveUnit(unit);

	unit->UnBlock();

	// do not remove unit from QF, otherwise projectiles
	// will not be able to connect with (ie. damage) it
	//
	// for NON-stunned transportees, QF position is kept
	// up-to-date by MoveType::SlowUpdate, otherwise by
	// ::Update
	//
	// quadField.RemoveUnit(unit);

	if (dynamic_cast<CBuilding*>(unit) != nullptr)
		unitLoader->RestoreGround(unit);

	if (dynamic_cast<CHoverAirMoveType*>(moveType) != nullptr)
		unit->moveType->useHeading = false;

	TransportedUnit tu;
		tu.unit = unit;
		tu.piece = piece;

	transportCapacityUsed += unit->xsize / SPRING_FOOTPRINT_SCALE;
	transportMassUsed += unit->mass;
	SetMass(mass + unit->mass);

	transportedUnits.push_back(tu);

	unit->moveType->StopMoving(true, true);
	unit->CalculateTerrainType();
	unit->UpdateTerrainType();

	eventHandler.UnitLoaded(unit, this);
	commandAI->BuggerOff(pos, -1.0f);
	return true;
}


bool CUnit::DetachUnitCore(CUnit* unit)
{
	if (unit->GetTransporter() != this)
		return false;

	for (TransportedUnit& tu: transportedUnits) {
		if (tu.unit != unit)
			continue;

		this->DeleteDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
		unit->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
		unit->SetTransporter(nullptr);
		unit->unloadingTransportId = id;

		if (dynamic_cast<CHoverAirMoveType*>(moveType) != nullptr)
			unit->moveType->useHeading = true;

		// de-stun detaching units in case we are not a fire-platform
		unit->SetStunned(unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health));

		unit->moveType->SlowUpdate();
		unit->moveType->LeaveTransport();

		if (CBuilding* building = dynamic_cast<CBuilding*>(unit))
			building->ForcedMove(building->pos);

		transportCapacityUsed -= unit->xsize / SPRING_FOOTPRINT_SCALE;
		transportMassUsed -= unit->mass;
		mass = Clamp(mass - unit->mass, CSolidObject::MINIMUM_MASS, CSolidObject::MAXIMUM_MASS);

		tu = transportedUnits.back();
		transportedUnits.pop_back();

		unit->UpdateVoidState(false);
		unit->CalculateTerrainType();
		unit->UpdateTerrainType();

		eventHandler.UnitUnloaded(unit, this);
		return true;
	}

	return false;
}


bool CUnit::DetachUnit(CUnit* unit)
{
	if (DetachUnitCore(unit)) {
		unit->Block();

		// erase command queue unless it's a wait command
		const CCommandQueue& queue = unit->commandAI->commandQue;

		if (unitDef->IsTransportUnit() && (queue.empty() || (queue.front().GetID() != CMD_WAIT)))
			unit->commandAI->GiveCommand(Command(CMD_STOP));

		return true;
	}

	return false;
}


bool CUnit::DetachUnitFromAir(CUnit* unit, const float3& pos)
{
	if (DetachUnitCore(unit)) {
		unit->Drop(this->pos, this->frontdir, this);

		// add an additional move command for after we land
		if (unitDef->IsTransportUnit() && unit->unitDef->canmove)
			unit->commandAI->GiveCommand(Command(CMD_MOVE, pos));

		return true;
	}

	return false;
}

bool CUnit::CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit, float* wantedHeightPtr) const {
	bool canLoadUnload = false;
	float wantedHeight = GetTransporteeWantedHeight(wantedPos, unit, &canLoadUnload);

	if (wantedHeightPtr != nullptr)
		*wantedHeightPtr = wantedHeight;

	return canLoadUnload;
}

float CUnit::GetTransporteeWantedHeight(const float3& wantedPos, const CUnit* unit, bool* allowedPos) const {
	bool isAllowedTerrain = true;

	float wantedHeight = unit->pos.y;
	float wantedSlope = 90.0f;
	float clampedHeight = wantedHeight;

	const UnitDef* transporteeUnitDef = unit->unitDef;
	const MoveDef* transporteeMoveDef = unit->moveDef;

	if (unit->GetTransporter() != nullptr) {
		// if unit is being transported, set <clampedHeight>
		// to the altitude at which to UNload the transportee
		wantedHeight = CGround::GetHeightReal(wantedPos.x, wantedPos.z);
		wantedSlope = CGround::GetSlope(wantedPos.x, wantedPos.z);

		if ((isAllowedTerrain = CGameHelper::CheckTerrainConstraints(transporteeUnitDef, transporteeMoveDef, wantedHeight, wantedHeight, wantedSlope, &clampedHeight))) {
			if (transporteeMoveDef != nullptr) {
				// transportee is a mobile ground unit
				switch (transporteeMoveDef->speedModClass) {
					case MoveDef::Ship: {
						wantedHeight = std::max(-unit->moveType->GetWaterline(), wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					case MoveDef::Hover: {
						wantedHeight = std::max(0.0f, wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					default: {
					} break;
				}
			} else {
				// transportee is a building or an airplane
				wantedHeight *= (1 - transporteeUnitDef->floatOnWater);
				clampedHeight = wantedHeight;
			}
		}

		if (dynamic_cast<const CBuilding*>(unit) != nullptr) {
			// for transported structures, <wantedPos> must be free/buildable
			// (note: TestUnitBuildSquare calls CheckTerrainConstraints again)
			BuildInfo bi(transporteeUnitDef, wantedPos, unit->buildFacing);
			bi.pos = CGameHelper::Pos2BuildPos(bi, true);
			CFeature* f = nullptr;

			if (isAllowedTerrain && (!CGameHelper::TestUnitBuildSquare(bi, f, -1, true) || f != nullptr))
				isAllowedTerrain = false;
		}
	}


	float rawContactHeight = clampedHeight + unit->height;
	float modContactHeight = rawContactHeight;

	// *we* must be capable of reaching the point-of-contact height
	// however this check fails for eg. ships that want to (un)load
	// land units on shore --> would require too many special cases
	// therefore restrict its use to transport aircraft
	if (this->moveDef == nullptr)
		isAllowedTerrain &= CGameHelper::CheckTerrainConstraints(unitDef, nullptr, rawContactHeight, rawContactHeight, 90.0f, &modContactHeight);

	if (allowedPos != nullptr)
		*allowedPos = isAllowedTerrain;

	return modContactHeight;
}

short CUnit::GetTransporteeWantedHeading(const CUnit* unit) const {
	if (unit->GetTransporter() == nullptr)
		return unit->heading;
	if (dynamic_cast<CHoverAirMoveType*>(moveType) == nullptr)
		return unit->heading;
	if (dynamic_cast<const CBuilding*>(unit) == nullptr)
		return unit->heading;

	// transported structures want to face a cardinal direction
	return (GetHeadingFromFacing(unit->buildFacing));
}

/******************************************************************************/
/******************************************************************************/


CR_BIND_DERIVED_POOL(CUnit, CSolidObject, , unitMemPool.allocMem, unitMemPool.freeMem)
CR_REG_METADATA(CUnit, (
	CR_MEMBER(unitDef),
	CR_MEMBER(shieldWeapon),
	CR_MEMBER(stockpileWeapon),
	CR_MEMBER(selfdExpDamages),
	CR_MEMBER(deathExpDamages),

	CR_MEMBER(featureDefID),

	CR_MEMBER(travel),
	CR_MEMBER(travelPeriod),

	CR_MEMBER(power),

	CR_MEMBER(paralyzeDamage),
	CR_MEMBER(captureProgress),
	CR_MEMBER(experience),
	CR_MEMBER(limExperience),

	CR_MEMBER(neutral),
	CR_MEMBER(beingBuilt),
	CR_MEMBER(upright),

	CR_MEMBER(lastAttackFrame),
	CR_MEMBER(lastFireWeapon),
	CR_MEMBER(lastFlareDrop),
	CR_MEMBER(lastNanoAdd),

	CR_MEMBER(soloBuilder),
	CR_MEMBER(lastAttacker),
	CR_MEMBER(transporter),

	CR_MEMBER(fpsControlPlayer),

	CR_MEMBER(moveType),
	CR_MEMBER(prevMoveType),

	CR_MEMBER(commandAI),
	CR_MEMBER(group),
	CR_MEMBER(script),

	CR_IGNORED( usMemBuffer),
	CR_IGNORED(amtMemBuffer),
	CR_IGNORED(smtMemBuffer),
	CR_IGNORED(caiMemBuffer),

	CR_MEMBER(weapons),
	CR_IGNORED(los),
	CR_MEMBER(losStatus),
	CR_MEMBER(quads),


	CR_MEMBER(loadingTransportId),
	CR_MEMBER(unloadingTransportId),
	CR_MEMBER(transportCapacityUsed),
	CR_MEMBER(transportMassUsed),

	CR_MEMBER(buildProgress),
	CR_MEMBER(groundLevelled),
	CR_MEMBER(terraformLeft),
	CR_MEMBER(repairAmount),

	CR_MEMBER(realLosRadius),
	CR_MEMBER(realAirLosRadius),

	CR_MEMBER(inBuildStance),
	CR_MEMBER(useHighTrajectory),

	CR_MEMBER(dontUseWeapons),
	CR_MEMBER(dontFire),

	CR_MEMBER(deathScriptFinished),
	CR_MEMBER(delayedWreckLevel),

	CR_MEMBER(restTime),
	CR_MEMBER(outOfMapTime),

	CR_MEMBER(reloadSpeed),
	CR_MEMBER(maxRange),
	CR_MEMBER(lastMuzzleFlameSize),

	CR_MEMBER(deathSpeed),
	CR_MEMBER(lastMuzzleFlameDir),
	CR_MEMBER(flankingBonusDir),

	CR_MEMBER(armorType),
	CR_MEMBER(category),

	CR_MEMBER(mapSquare),

	CR_MEMBER(losRadius),
	CR_MEMBER(airLosRadius),

	CR_MEMBER(radarRadius),
	CR_MEMBER(sonarRadius),
	CR_MEMBER(jammerRadius),
	CR_MEMBER(sonarJamRadius),
	CR_MEMBER(seismicRadius),
	CR_MEMBER(seismicSignature),
	CR_MEMBER(stealth),
	CR_MEMBER(sonarStealth),

	CR_MEMBER(curTarget),

	CR_MEMBER(resourcesCondUse),
	CR_MEMBER(resourcesCondMake),
	CR_MEMBER(resourcesUncondUse),
	CR_MEMBER(resourcesUncondMake),

	CR_MEMBER(resourcesUse),
	CR_MEMBER(resourcesMake),

	CR_MEMBER(resourcesUseI),
	CR_MEMBER(resourcesMakeI),
	CR_MEMBER(resourcesUseOld),
	CR_MEMBER(resourcesMakeOld),

	CR_MEMBER(storage),

	CR_MEMBER(harvestStorage),
	CR_MEMBER(harvested),

	CR_MEMBER(energyTickMake),
	CR_MEMBER(metalExtract),

	CR_MEMBER(cost),
	CR_MEMBER(buildTime),

	CR_MEMBER(recentDamage),

	CR_MEMBER(fireState),
	CR_MEMBER(moveState),

	CR_MEMBER(activated),

	CR_MEMBER(isDead),
	CR_MEMBER(fallSpeed),

	CR_MEMBER(flankingBonusMode),
	CR_MEMBER(flankingBonusMobility),
	CR_MEMBER(flankingBonusMobilityAdd),
	CR_MEMBER(flankingBonusAvgDamage),
	CR_MEMBER(flankingBonusDifDamage),

	CR_MEMBER(armoredState),
	CR_MEMBER(armoredMultiple),
	CR_MEMBER(curArmorMultiple),

	CR_MEMBER(posErrorVector),
	CR_MEMBER(posErrorDelta),
	CR_MEMBER(nextPosErrorUpdate),

	CR_MEMBER(wantCloak),
	CR_MEMBER(isCloaked),
	CR_MEMBER(decloakDistance),

	CR_MEMBER(lastTerrainType),
	CR_MEMBER(curTerrainType),

	CR_MEMBER(selfDCountdown),

	CR_MEMBER(transportedUnits),
	CR_MEMBER(incomingMissiles),

	CR_MEMBER(cegDamage),

	CR_MEMBER_UN(noMinimap),
	CR_MEMBER_UN(leaveTracks),

	CR_MEMBER_UN(isSelected),
	CR_MEMBER_UN(isIcon),
	CR_MEMBER(iconRadius),

	CR_MEMBER(lastUnitUpdate),

	CR_MEMBER(stunned),

//	CR_MEMBER(expMultiplier),
//	CR_MEMBER(expPowerScale),
//	CR_MEMBER(expHealthScale),
//	CR_MEMBER(expReloadScale),
//	CR_MEMBER(expGrade),

//	CR_MEMBER(empDecline),
//	CR_MEMBER(spawnFeature),

//	CR_MEMBER(model),

	CR_POSTLOAD(PostLoad)
))

CR_BIND(CUnit::TransportedUnit,)

CR_REG_METADATA_SUB(CUnit, TransportedUnit, (
	CR_MEMBER(unit),
	CR_MEMBER(piece)
))
