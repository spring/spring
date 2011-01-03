/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "UnitDef.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "UnitDefHandler.h"
#include "UnitLoader.h"
#include "UnitTypes/Building.h"
#include "UnitTypes/TransportUnit.h"
#include "COB/NullUnitScript.h"
#include "COB/UnitScriptFactory.h"
#include "COB/CobInstance.h" // for TAANG2RAD

#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/MobileCAI.h"
#include "CommandAI/TransportCAI.h"

#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/Models/IModelParser.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GroundFlash.h"

#include "Sim/Units/Groups/Group.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/MoveTypeFactory.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/creg/STL_List.h"
#include "System/LoadSave/LoadSaveInterface.h"
#include "System/Sound/IEffectChannel.h"
#include "System/Sync/SyncedPrimitive.h"
#include "System/Sync/SyncTracer.h"

#define PLAY_SOUNDS 1

CLogSubsystem LOG_UNIT("unit");

CR_BIND_DERIVED(CUnit, CSolidObject, );

// See end of source for member bindings
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//! info: SlowUpdate runs each 16th GameFrame (:= twice per 32GameFrames) (a second has GAME_SPEED=30 gameframes!)
float CUnit::empDecline     = 2.0f * (float)UNIT_SLOWUPDATE_RATE / (float)GAME_SPEED / 40.0f;
float CUnit::expMultiplier  = 1.0f;
float CUnit::expPowerScale  = 1.0f;
float CUnit::expHealthScale = 0.7f;
float CUnit::expReloadScale = 0.4f;
float CUnit::expGrade       = 0.0f;


CUnit::CUnit() : CSolidObject(),
	unitDef(NULL),
	frontdir(0.0f, 0.0f, 1.0f),
	rightdir(-1.0f, 0.0f, 0.0f),
	updir(0.0f, 1.0f, 0.0f),
	upright(true),
	relMidPos(0.0f, 0.0f, 0.0f),
	travel(0.0f),
	travelPeriod(0.0f),
	power(100.0f),
	maxHealth(100.0f),
	health(100.0f),
	paralyzeDamage(0.0f),
	captureProgress(0.0f),
	experience(0.0f),
	limExperience(0.0f),
	neutral(false),
	soloBuilder(NULL),
	beingBuilt(true),
	lastNanoAdd(gs->frameNum),
	repairAmount(0.0f),
	transporter(NULL),
	toBeTransported(false),
	buildProgress(0.0f),
	groundLevelled(true),
	terraformLeft(0.0f),
	realLosRadius(0),
	realAirLosRadius(0),
	losStatus(teamHandler->ActiveAllyTeams(), 0),
	inBuildStance(false),
	stunned(false),
	useHighTrajectory(false),
	dontUseWeapons(false),
	deathScriptFinished(false),
	delayedWreckLevel(-1),
	restTime(0),
	shieldWeapon(NULL),
	stockpileWeapon(NULL),
	reloadSpeed(1.0f),
	maxRange(0.0f),
	haveTarget(false),
	haveUserTarget(false),
	haveDGunRequest(false),
	lastMuzzleFlameSize(0.0f),
	lastMuzzleFlameDir(0.0f, 1.0f, 0.0f),
	armorType(0),
	category(0),
	los(NULL),
	tempNum(0),
	lastSlowUpdate(0),
	losRadius(0),
	airLosRadius(0),
	losHeight(0.0f),
	lastLosUpdate(0),
	radarRadius(0),
	sonarRadius(0),
	jammerRadius(0),
	sonarJamRadius(0),
	seismicRadius(0),
	seismicSignature(0.0f),
	hasRadarCapacity(false),
	oldRadarPos(0, 0),
	moveType(NULL),
	prevMoveType(NULL),
	usingScriptMoveType(false),
	commandAI(NULL),
	group(NULL),
	condUseMetal(0.0f),
	condUseEnergy(0.0f),
	condMakeMetal(0.0f),
	condMakeEnergy(0.0f),
	uncondUseMetal(0.0f),
	uncondUseEnergy(0.0f),
	uncondMakeMetal(0.0f),
	uncondMakeEnergy(0.0f),
	metalUse(0.0f),
	energyUse(0.0f),
	metalMake(0.0f),
	energyMake(0.0f),
	metalUseI(0.0f),
	energyUseI(0.0f),
	metalMakeI(0.0f),
	energyMakeI(0.0f),
	metalUseold(0.0f),
	energyUseold(0.0f),
	metalMakeold(0.0f),
	energyMakeold(0.0f),
	energyTickMake(0.0f),
	metalExtract(0.0f),
	metalCost(100.0f),
	energyCost(0.0f),
	buildTime(100.0f),
	metalStorage(0.0f),
	energyStorage(0.0f),
	lastAttacker(NULL),
	lastAttackedPiece(NULL),
	lastAttackedPieceFrame(-1),
	lastAttack(-200),
	lastFireWeapon(0),
	recentDamage(0.0f),
	userTarget(NULL),
	userAttackPos(ZeroVector),
	userAttackGround(false),
	commandShotCount(-1),
	fireState(FIRESTATE_FIREATWILL),
	moveState(MOVESTATE_MANEUVER),
	dontFire(false),
	activated(false),
	localmodel(NULL),
	script(NULL),
	crashing(false),
	isDead(false),
	falling(false),
	fallSpeed(0.2f),
	inAir(false),
	inWater(false),
	flankingBonusMode(0),
	flankingBonusDir(1.0f, 0.0f, 0.0f),
	flankingBonusMobility(10.0f),
	flankingBonusMobilityAdd(0.01f),
	flankingBonusAvgDamage(1.4f),
	flankingBonusDifDamage(0.5f),
	armoredState(false),
	armoredMultiple(1.0f),
	curArmorMultiple(1.0f),
	posErrorVector(ZeroVector),
	posErrorDelta(ZeroVector),
	nextPosErrorUpdate(1),
	hasUWWeapons(false),
	wantCloak(false),
	scriptCloak(0),
	cloakTimeout(128),
	curCloakTimeout(gs->frameNum),
	isCloaked(false),
	decloakDistance(0.0f),
	lastTerrainType(-1),
	curTerrainType(0),
	selfDCountdown(0),
	directControl(NULL),
	myTrack(NULL),
	lastFlareDrop(0),
	currentFuel(0.0f),
	luaDraw(false),
	noDraw(false),
	noSelect(false),
	noMinimap(false),
	isIcon(false),
	iconRadius(0.0f),
	maxSpeed(0.0f),
	maxReverseSpeed(0.0f),
	lodCount(0),
	currentLOD(0),
	alphaThreshold(0.1f),
	cegDamage(1)
#ifdef USE_GML
	, lastDrawFrame(-30)
#endif
{
	GML_GET_TICKS(lastUnitUpdate);
}

CUnit::~CUnit()
{
	// clean up if we are still under movectrl here
	DisableScriptMoveType();

	if (delayedWreckLevel >= 0) {
		// note: could also do this in Update() or even in CUnitKilledCB()
		// where we wouldn't need deathSpeed, but not in KillUnit() since
		// we have to wait for deathScriptFinished (but we want the delay
		// in frames between CUnitKilledCB() and the CreateWreckage() call
		// to be as short as possible to prevent position jumps)
		featureHandler->CreateWreckage(pos, wreckName, heading, buildFacing,
		                               delayedWreckLevel, team, -1, true,
		                               unitDefName, deathSpeed);
	}

	if (unitDef->isAirBase) {
		airBaseHandler->DeregisterAirBase(this);
	}

#ifdef TRACE_SYNC
	tracefile << "Unit died: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif

	if (directControl) {
		directControl->myController->StopControllingUnit();
		directControl = NULL;
	}

	if (activated && unitDef->targfac) {
		radarhandler->radarErrorSize[allyteam] *= radarhandler->targFacEffect;
	}

	SetMetalStorage(0);
	SetEnergyStorage(0);

	delete commandAI;       commandAI       = NULL;
	delete moveType;        moveType        = NULL;
	delete prevMoveType;    prevMoveType    = NULL;

	// not all unit deletions run through KillUnit(),
	// but we always want to call this for ourselves
	UnBlock();

	// Remove us from our group, if we were in one
	SetGroup(NULL);

	std::vector<CWeapon*>::iterator wi;
	for (wi = weapons.begin(); wi != weapons.end(); ++wi) {
		delete *wi;
	}

	qf->RemoveUnit(this);
	loshandler->DelayedFreeInstance(los);
	los = NULL;
	radarhandler->RemoveUnit(this);

	if (script != &CNullUnitScript::value) {
		delete script;
		script = NULL;
	}

	modelParser->DeleteLocalModel(this);
}


void CUnit::SetMetalStorage(float newStorage)
{
	teamHandler->Team(team)->metalStorage -= metalStorage;
	metalStorage = newStorage;
	teamHandler->Team(team)->metalStorage += metalStorage;
}


void CUnit::SetEnergyStorage(float newStorage)
{
	teamHandler->Team(team)->energyStorage -= energyStorage;
	energyStorage = newStorage;
	teamHandler->Team(team)->energyStorage += energyStorage;
}



void CUnit::PreInit(const UnitDef* uDef, int uTeam, int facing, const float3& position, bool build)
{
	team = uTeam;
	allyteam = teamHandler->AllyTeam(uTeam);

	unitDef = uDef;
	unitDefName = unitDef->name;

	pos = position;
	pos.CheckInBounds();
	mapSquare = ground->GetSquare(pos);

	// temporary radius
	SetRadius(1.2f);
	uh->AddUnit(this);
	qf->MovedUnit(this);
	hasRadarPos = false;

	losStatus[allyteam] =
		LOS_ALL_MASK_BITS |
		LOS_INLOS         |
		LOS_INRADAR       |
		LOS_PREVLOS       |
		LOS_CONTRADAR;

	posErrorVector = gs->randVector();
	posErrorVector.y *= 0.2f;

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] new unit: " << unitDefName << " ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif

	ASSERT_SYNCED_FLOAT3(pos);

	heading  = GetHeadingFromFacing(facing);
	frontdir = GetVectorFromHeading(heading);
	updir    = UpVector;
	rightdir = frontdir.cross(updir);
	upright  = unitDef->upright;

	buildFacing = std::abs(facing) % NUM_FACINGS;
	curYardMap = (unitDef->GetYardMap(buildFacing).empty())? NULL: &unitDef->GetYardMap(buildFacing)[0];
	xsize = ((buildFacing & 1) == 0) ? unitDef->xsize : unitDef->zsize;
	zsize = ((buildFacing & 1) == 1) ? unitDef->xsize : unitDef->zsize;

	beingBuilt = build;
	mass = (beingBuilt)? mass: unitDef->mass;
	power = unitDef->power;
	maxHealth = unitDef->health;
	health = unitDef->health;
	losHeight = unitDef->losHeight;
	metalCost = unitDef->metalCost;
	energyCost = unitDef->energyCost;
	buildTime = unitDef->buildTime;
	currentFuel = unitDef->maxFuel;
	armoredMultiple = std::max(0.0001f, unitDef->armoredMultiple); // armored multiple of 0 will crash spring
	armorType = unitDef->armorType;
	category = unitDef->category;

	tooltip = unitDef->humanName + " - " + unitDef->tooltip;
	wreckName = unitDef->wreckName;


	// sensor parameters
	realLosRadius = int(unitDef->losRadius);
	realAirLosRadius = int(unitDef->airLosRadius);

	radarRadius      = unitDef->radarRadius    / (SQUARE_SIZE * 8);
	sonarRadius      = unitDef->sonarRadius    / (SQUARE_SIZE * 8);
	jammerRadius     = unitDef->jammerRadius   / (SQUARE_SIZE * 8);
	sonarJamRadius   = unitDef->sonarJamRadius / (SQUARE_SIZE * 8);
	seismicRadius    = unitDef->seismicRadius  / (SQUARE_SIZE * 8);
	seismicSignature = unitDef->seismicSignature;
	hasRadarCapacity =
		(radarRadius   > 0.0f) || (sonarRadius    > 0.0f) ||
		(jammerRadius  > 0.0f) || (sonarJamRadius > 0.0f) ||
		(seismicRadius > 0.0f);
	stealth = unitDef->stealth;
	sonarStealth = unitDef->sonarStealth;
	decloakDistance = unitDef->decloakDistance;
	cloakTimeout = unitDef->cloakTimeout;


	floatOnWater =
		unitDef->floater ||
		(unitDef->movedata && ((unitDef->movedata->moveType == MoveData::Hover_Move) ||
							   (unitDef->movedata->moveType == MoveData::Ship_Move)));

	if (floatOnWater && (pos.y <= 0.0f))
		pos.y = -unitDef->waterline;

	maxSpeed = unitDef->speed / GAME_SPEED;
	maxReverseSpeed = unitDef->rSpeed / GAME_SPEED;

	flankingBonusMode        = unitDef->flankingBonusMode;
	flankingBonusDir         = unitDef->flankingBonusDir;
	flankingBonusMobility    = unitDef->flankingBonusMobilityAdd * 1000;
	flankingBonusMobilityAdd = unitDef->flankingBonusMobilityAdd;
	flankingBonusAvgDamage   = (unitDef->flankingBonusMax + unitDef->flankingBonusMin) * 0.5f;
	flankingBonusDifDamage   = (unitDef->flankingBonusMax - unitDef->flankingBonusMin) * 0.5f;

	useHighTrajectory = (unitDef->highTrajectoryType == 1);

	if (build) {
		ChangeLos(1, 1);
		health = 0.1f;
	} else {
		ChangeLos(realLosRadius, realAirLosRadius);
	}

	energyTickMake =
		unitDef->energyMake +
		unitDef->tidalGenerator * mapInfo->map.tidalStrength;

	SetRadius((model = unitDef->LoadModel())->radius);
	modelParser->CreateLocalModel(this);

	// copy the UnitDef volume instance
	//
	// aircraft still get half-size spheres for coldet purposes
	// iif no custom volume is defined (unit->model->radius and
	// unit->radius themselves are no longer altered)
	//
	// note: gets deleted in ~CSolidObject
	collisionVolume = new CollisionVolume(unitDef->collisionVolume, model->radius * ((unitDef->canfly)? 0.5f: 1.0f));
	moveType = MoveTypeFactory::GetMoveType(this, unitDef);
	script = CUnitScriptFactory::CreateScript(unitDef->scriptPath, this);
}

void CUnit::PostInit(const CUnit* builder)
{
	weapons.reserve(unitDef->weapons.size());

	for (unsigned int i = 0; i < unitDef->weapons.size(); i++) {
		weapons.push_back(unitLoader->LoadWeapon(this, &unitDef->weapons[i]));
	}

	// Call initializing script functions
	script->Create();

	relMidPos = model->relMidPos;
	losHeight = relMidPos.y + (radius * 0.5f);
	height = model->height;

	UpdateMidPos();

	// all ships starts on water, all others on ground.
	// TODO: Improve this. There might be cases when this is not correct.
	if (unitDef->movedata &&
	    (unitDef->movedata->moveType == MoveData::Hover_Move)) {
		physicalState = CSolidObject::Hovering;
	} else if (floatOnWater) {
		physicalState = CSolidObject::Floating;
	} else {
		physicalState = CSolidObject::OnGround;
	}

	// all units are blocking (ie. register on the blk-map
	// when not flying) except mines, since their position
	// would be given away otherwise by the PF, etc.
	// note: this does mean that mines can be stacked (would
	// need an extra yardmap character to prevent)
	immobile = unitDef->IsImmobileUnit();
	blocking = !(immobile && unitDef->canKamikaze);

	if (blocking) {
		Block();
	}

	if (unitDef->windGenerator > 0.0f) {
		wind.AddUnit(this);
	}

	UpdateTerrainType();

	if (unitDef->canmove || unitDef->builder) {
		if (unitDef->moveState <= FIRESTATE_NONE) {
			if (builder != NULL) {
				// always inherit our builder's movestate
				// if none, set CUnit's default (maneuver)
				moveState = builder->moveState;
			}
		} else {
			// use our predefined movestate
			moveState = unitDef->moveState;
		}

		Command c;
		c.id = CMD_MOVE_STATE;
		c.params.push_back(moveState);
		commandAI->GiveCommand(c);
	}

	if (commandAI->CanChangeFireState()) {
		if (unitDef->fireState <= MOVESTATE_NONE) {
			if (builder != NULL && dynamic_cast<CFactoryCAI*>(builder->commandAI) != NULL) {
				// inherit our builder's firestate (if it is a factory)
				// if no builder, CUnit's default (fire-at-will) is set
				fireState = builder->fireState;
			}
		} else {
			// use our predefined firestate
			fireState = unitDef->fireState;
		}

		Command c;
		c.id = CMD_FIRE_STATE;
		c.params.push_back(fireState);
		commandAI->GiveCommand(c);
	}

	eventHandler.UnitCreated(this, builder);
	eoh->UnitCreated(*this, builder);

	if (!beingBuilt) {
		FinishedBuilding();
	}
}




void CUnit::ForcedMove(const float3& newPos)
{
	if (blocking) {
		UnBlock();
	}

	CBuilding* building = dynamic_cast<CBuilding*>(this);
	if (building)
		groundDecals->RemoveBuilding(building, NULL);

	pos = newPos;
	UpdateMidPos();

	if (building)
		groundDecals->AddBuilding(building);

	if (blocking) {
		Block();
	}

	qf->MovedUnit(this);
	loshandler->MoveUnit(this, false);
	radarhandler->MoveUnit(this);
}


void CUnit::ForcedSpin(const float3& newDir)
{
	frontdir = newDir;
	heading = GetHeadingFromVector(newDir.x, newDir.z);
	ForcedMove(pos); // lazy, don't need to update the quadfield, etc...
}


void CUnit::SetFront(const SyncedFloat3& newDir)
{
	frontdir = newDir;
	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);
	updir.Normalize();
	heading = GetHeadingFromVector(frontdir.x, frontdir.z);
	UpdateMidPos();
}

void CUnit::SetUp(const SyncedFloat3& newDir)
{
	updir = newDir;
	updir.Normalize();
	frontdir = updir.cross(rightdir);
	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	heading = GetHeadingFromVector(frontdir.x, frontdir.z);
	UpdateMidPos();
}


void CUnit::SetRight(const SyncedFloat3& newDir)
{
	rightdir = newDir;
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);
	updir.Normalize();
	frontdir = updir.cross(rightdir);
	frontdir.Normalize();
	heading = GetHeadingFromVector(frontdir.x, frontdir.z);
	UpdateMidPos();
}


void CUnit::UpdateMidPos()
{
	midPos = pos +
		(frontdir * relMidPos.z) +
		(updir    * relMidPos.y) +
		(rightdir * relMidPos.x);
}


void CUnit::Drop(const float3& parentPos, const float3& parentDir, CUnit* parent)
{
	// drop unit from position
	fallSpeed = unitDef->unitFallSpeed > 0 ? unitDef->unitFallSpeed : parent->unitDef->fallSpeed;
	falling = true;
	pos.y = parentPos.y - height;
	frontdir = parentDir;
	frontdir.y = 0.0f;
	speed.y = 0.0f;

	// start parachute animation
	script->Falling();
}


void CUnit::EnableScriptMoveType()
{
	if (usingScriptMoveType) {
		return;
	}
	prevMoveType = moveType;
	moveType = new CScriptMoveType(this);
	usingScriptMoveType = true;
}


void CUnit::DisableScriptMoveType()
{
	if (!usingScriptMoveType) {
		return;
	}

	delete moveType;
	moveType = prevMoveType;
	prevMoveType = NULL;
	usingScriptMoveType = false;

	// FIXME: prevent the issuing of extra commands ?
	if (moveType) {
		moveType->oldPos = pos;
		moveType->SetGoal(pos);
		moveType->StopMoving();
	}
}


void CUnit::Update()
{
	ASSERT_SYNCED_FLOAT3(pos);

	posErrorVector += posErrorDelta;

	if (deathScriptFinished) {
		uh->DeleteUnit(this);
		return;
	}

	if (beingBuilt) {
		return;
	}

	// 0.968 ** 16 is slightly less than 0.6, which was the old value used in SlowUpdate
	residualImpulse *= 0.968f;

	const bool oldInAir   = inAir;
	const bool oldInWater = inWater;

	inWater = (pos.y <= 0.0f);
	inAir   = (!inWater) && ((pos.y - ground->GetHeightAboveWater(pos.x,pos.z)) > 1.0f);
	isUnderWater = ((pos.y + ((mobility != NULL && mobility->subMarine)? 0.0f: model->height)) < 0.0f);

	if (inAir != oldInAir) {
		if (inAir) {
			eventHandler.UnitEnteredAir(this);
		} else {
			eventHandler.UnitLeftAir(this);
		}
	}
	if (inWater != oldInWater) {
		if (inWater) {
			eventHandler.UnitEnteredWater(this);
		} else {
			eventHandler.UnitLeftWater(this);
		}
	}

	if (travelPeriod != 0.0f) {
		travel += speed.Length();
		travel = fmod(travel, travelPeriod);
	}

	recentDamage *= 0.9f;
	flankingBonusMobility += flankingBonusMobilityAdd;

	if (stunned) {
		// leave the pad if reserved
		if (moveType->reservedPad) {
			airBaseHandler->LeaveLandingPad(moveType->reservedPad);
			moveType->reservedPad = 0;
			moveType->padStatus = 0;
		}
		// paralyzed weapons shouldn't reload
		std::vector<CWeapon*>::iterator wi;
		for (wi = weapons.begin(); wi != weapons.end(); ++wi) {
			++(*wi)->reloadStatus;
		}
		return;
	}

	restTime++;

	if (!dontUseWeapons) {
		std::vector<CWeapon*>::iterator wi;
		for (wi = weapons.begin(); wi != weapons.end(); ++wi) {
			(*wi)->Update();
		}
	}
}


void CUnit::UpdateResources()
{
	metalMake  = metalMakeI  + metalMakeold;
	metalUse   = metalUseI   + metalUseold;
	energyMake = energyMakeI + energyMakeold;
	energyUse  = energyUseI  + energyUseold;

	metalMakeold  = metalMakeI;
	metalUseold   = metalUseI;
	energyMakeold = energyMakeI;
	energyUseold  = energyUseI;

	metalMakeI = metalUseI = energyMakeI = energyUseI = 0.0f;
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


unsigned short CUnit::CalcLosStatus(int at)
{
	const unsigned short currStatus = losStatus[at];

	unsigned short newStatus = currStatus;
	unsigned short mask = ~(currStatus >> 8);

	if (loshandler->InLos(this, at)) {
		if (!beingBuilt) {
			newStatus |= (mask & (LOS_INLOS   | LOS_INRADAR |
			                      LOS_PREVLOS | LOS_CONTRADAR));
		} else {
			// we are being built, do not set LOS_PREVLOS
			// since we do not want ghosts for nanoframes
			newStatus |=  (mask & (LOS_INLOS   | LOS_INRADAR));
			newStatus &= ~(mask & (LOS_PREVLOS | LOS_CONTRADAR));
		}
	}
	else if (radarhandler->InRadar(this, at)) {
		newStatus |=  (mask & LOS_INRADAR);
		newStatus &= ~(mask & LOS_INLOS);
	}
	else {
		newStatus &= ~(mask & (LOS_INLOS | LOS_INRADAR | LOS_CONTRADAR));
	}

	return newStatus;
}


inline void CUnit::UpdateLosStatus(int at)
{
	const unsigned short currStatus = losStatus[at];
	if ((currStatus & LOS_ALL_MASK_BITS) == LOS_ALL_MASK_BITS) {
		return; // no need to update, all changes are masked
	}
	SetLosStatus(at, CalcLosStatus(at));
}


void CUnit::SlowUpdate()
{
	--nextPosErrorUpdate;
	if (nextPosErrorUpdate == 0) {
		float3 newPosError(gs->randVector());
		newPosError.y *= 0.2f;
		if (posErrorVector.dot(newPosError) < 0.0f) {
			newPosError = -newPosError;
		}
		posErrorDelta = (newPosError - posErrorVector) * (1.0f / 256.0f);
		nextPosErrorUpdate = 16;
	}

	for (int at = 0; at < teamHandler->ActiveAllyTeams(); ++at) {
		UpdateLosStatus(at);
	}

	DoWaterDamage();

	if (health < 0.0f) {
		KillUnit(false, true, NULL);
		return;
	}

	repairAmount=0.0f;

	if (paralyzeDamage > 0) {
		paralyzeDamage -= maxHealth * 0.5f * CUnit::empDecline;
		if (paralyzeDamage < 0) {
			paralyzeDamage = 0;
		}
	}

	UpdateResources();

	if (stunned) {
		// de-stun only if we are not (still) inside a non-firebase transport
		if ((paralyzeDamage <= (modInfo.paralyzeOnMaxHealth? maxHealth: health)) &&
			!(transporter && !transporter->unitDef->isFirePlatform)) {
			stunned = false;
		}

		SlowUpdateCloak(true);
		return;
	}

	if (selfDCountdown && !stunned) {
		selfDCountdown--;
		if (selfDCountdown <= 1) {
			if (!beingBuilt) {
				KillUnit(true, false, NULL);
			} else {
				KillUnit(false, true, NULL);	//avoid unfinished buildings making an explosion
			}
			selfDCountdown = 0;
			return;
		}
		if ((selfDCountdown & 1) && (team == gu->myTeam)) {
			logOutput.Print("%s: Self destruct in %i s",
			                unitDef->humanName.c_str(), selfDCountdown / 2);
		}
	}

	if (beingBuilt) {
		if (modInfo.constructionDecay && lastNanoAdd < gs->frameNum - modInfo.constructionDecayTime) {
			const float buildDecay = 1.0f / (buildTime * modInfo.constructionDecaySpeed);
			health -= maxHealth * buildDecay;
			buildProgress -= buildDecay;
			AddMetal(metalCost * buildDecay, false);
			if (health < 0.0f) {
				KillUnit(false, true, NULL);
			}
		}
		ScriptDecloak(false);
		return;
	}

	// below is stuff that should not be run while being built

	lastSlowUpdate=gs->frameNum;

	commandAI->SlowUpdate();
	moveType->SlowUpdate();

	// FIXME: scriptMakeMetal ...?
	AddMetal(uncondMakeMetal);
	AddEnergy(uncondMakeEnergy);
	UseMetal(uncondUseMetal);
	UseEnergy(uncondUseEnergy);
	if (activated) {
		if (UseMetal(condUseMetal)) {
			AddEnergy(condMakeEnergy);
		}
		if (UseEnergy(condUseEnergy)) {
			AddMetal(condMakeMetal);
		}
	}

	AddMetal(unitDef->metalMake * 0.5f);
	if (activated) {
		if (UseEnergy(unitDef->energyUpkeep * 0.5f)) {
			AddMetal(unitDef->makesMetal * 0.5f);
			if (unitDef->extractsMetal > 0.0f) {
				AddMetal(metalExtract * 0.5f);
			}
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

	if (health<maxHealth) {
		health += unitDef->autoHeal;

		if (restTime > unitDef->idleTime) {
			health += unitDef->idleAutoHeal;
		}
		if (health > maxHealth) {
			health = maxHealth;
		}
	}

	SlowUpdateCloak(false);

	if (unitDef->canKamikaze && (fireState >= FIRESTATE_FIREATWILL || userTarget || userAttackGround)) {
		if (fireState >= FIRESTATE_FIREATWILL) {
			std::vector<int> nearbyUnits;
			if (unitDef->kamikazeUseLOS) {
				helper->GetEnemyUnits(pos, unitDef->kamikazeDist, allyteam, nearbyUnits);
			} else {
				helper->GetEnemyUnitsNoLosTest(pos, unitDef->kamikazeDist, allyteam, nearbyUnits);
			}

			for (std::vector<int>::const_iterator it = nearbyUnits.begin(); it != nearbyUnits.end(); ++it) {
				const CUnit* victim = uh->GetUnitUnsafe(*it);
				const float3 dif = pos - victim->pos;

				if (dif.SqLength() < Square(unitDef->kamikazeDist)) {
					if (victim->speed.dot(dif) <= 0) {
						//! self destruct when we start moving away from the target, this should maximize the damage
						KillUnit(true, false, NULL);
						return;
					}
				}
			}
		}

		if (
			   (userTarget       && (userTarget->pos.SqDistance(pos) < Square(unitDef->kamikazeDist)))
			|| (userAttackGround && (userAttackPos.SqDistance(pos))  < Square(unitDef->kamikazeDist))
		) {
			KillUnit(true, false, NULL);
			return;
		}
	}

	SlowUpdateWeapons();

	if (moveType->progressState == AMoveType::Active ) {
		if (seismicSignature && !GetTransporter()) {
			DoSeismicPing((int)seismicSignature);
		}
	}

	CalculateTerrainType();
	UpdateTerrainType();
}

void CUnit::SlowUpdateWeapons() {
	if (weapons.empty()) {
		return;
	}

	haveTarget = false;
	haveUserTarget = false;

	// aircraft and ScriptMoveType do not want this
	if (moveType->useHeading) {
		SetDirectionFromHeading();
	}

	if (!dontFire) {
		for (vector<CWeapon*>::iterator wi = weapons.begin(); wi != weapons.end(); ++wi) {
			CWeapon* w = *wi;
			
			if (!w->haveUserTarget) {
				if (haveDGunRequest == (unitDef->canDGun && w->weaponDef->manualfire)) { // == ((!haveDGunRequest && !isDGun) || (haveDGunRequest && isDGun))
					if (userTarget) {
						w->AttackUnit(userTarget, true);
					} else if (userAttackGround) {
						w->AttackGround(userAttackPos, true);
					}
				}
			}

			w->SlowUpdate();

			if (w->targetType == Target_None && fireState > FIRESTATE_HOLDFIRE && lastAttacker && (lastAttack + 200 > gs->frameNum))
				w->AttackUnit(lastAttacker, false);
		}
	}
}


void CUnit::SetDirectionFromHeading()
{
	if (GetTransporter() == NULL) {
		frontdir = GetVectorFromHeading(heading);

		if (upright || !unitDef->canmove) {
			updir = UpVector;
			rightdir = frontdir.cross(updir);
		} else  {
			updir = ground->GetNormal(pos.x, pos.z);
			rightdir = frontdir.cross(updir);
			rightdir.Normalize();
			frontdir = updir.cross(rightdir);
		}
	}
}



void CUnit::DoWaterDamage()
{
	if (uh->waterDamage > 0.0f) {
		if (pos.IsInBounds()) {
			const int  px            = int(pos.x / (SQUARE_SIZE * 2));
			const int  pz            = int(pos.z / (SQUARE_SIZE * 2));
			const bool isFloating    = (physicalState == CSolidObject::Floating);
			const bool onGround      = (physicalState == CSolidObject::OnGround);
			const bool isWaterSquare = (readmap->mipHeightmap[1][pz * gs->hmapx + px] <= 0.0f);

			if ((pos.y <= 0.0f) && isWaterSquare && (isFloating || onGround)) {
				DoDamage(DamageArray(uh->waterDamage), 0, ZeroVector, -1);
			}
		}
	}
}

void CUnit::DoDamage(const DamageArray& damages, CUnit* attacker, const float3& impulse, int weaponDefId)
{
	if (isDead) {
		return;
	}

	float damage = damages[armorType];

	if (damage > 0.0f) {
		if (attacker) {
			SetLastAttacker(attacker);
			if (flankingBonusMode) {
				const float3 adir = (attacker->pos - pos).SafeNormalize(); // FIXME -- not the impulse direction?

				if (flankingBonusMode == 1) {
					// mode 1 = global coordinates, mobile
					flankingBonusDir += adir * flankingBonusMobility;
					flankingBonusDir.Normalize();
					flankingBonusMobility = 0.0f;
					damage *= flankingBonusAvgDamage - adir.dot(flankingBonusDir) * flankingBonusDifDamage;
				}
				else {
					float3 adirRelative;
					adirRelative.x = adir.dot(rightdir);
					adirRelative.y = adir.dot(updir);
					adirRelative.z = adir.dot(frontdir);
					if (flankingBonusMode == 2) {	// mode 2 = unit coordinates, mobile
						flankingBonusDir += adirRelative * flankingBonusMobility;
						flankingBonusDir.Normalize();
						flankingBonusMobility = 0.0f;
					}
					// modes 2 and 3 both use this; 3 is unit coordinates, immobile
					damage *= flankingBonusAvgDamage - adirRelative.dot(flankingBonusDir) * flankingBonusDifDamage;
				}
			}
		}
		damage *= curArmorMultiple;
		restTime = 0; // bleeding != resting
	}

	float3 hitDir = -impulse;
	hitDir.y = 0.0f;
	hitDir.SafeNormalize();

	script->HitByWeapon(hitDir, weaponDefId, /*inout*/ damage);

	float experienceMod = expMultiplier;
	const int paralyzeTime = damages.paralyzeDamageTime;
	float newDamage = damage;
	float impulseMult = 1.0f;

	if (luaRules && luaRules->UnitPreDamaged(this, attacker, damage, weaponDefId,
			!!paralyzeTime, &newDamage, &impulseMult)) {
		damage = newDamage;
	}

	residualImpulse += ((impulse * impulseMult) / mass);
	moveType->ImpulseAdded();

	if (paralyzeTime == 0) { // real damage
		if (damage > 0.0f) {
			// Dont log overkill damage (so dguns/nukes etc dont inflate values)
			const float statsdamage = std::max(0.0f, std::min(maxHealth - health, damage));
			if (attacker) {
				teamHandler->Team(attacker->team)->currentStats->damageDealt += statsdamage;
			}
			teamHandler->Team(team)->currentStats->damageReceived += statsdamage;
			health -= damage;
		}
		else { // healing
			health -= damage;
			if (health > maxHealth) {
				health = maxHealth;
			}
			if (health > paralyzeDamage && !modInfo.paralyzeOnMaxHealth) {
				stunned = false;
			}
		}
	}
	else { // paralyzation
		experienceMod *= 0.1f; // reduced experience
		if (damage > 0.0f) {
			// paralyzeDamage may not get higher than maxHealth * (paralyzeTime + 1),
			// which means the unit will be destunned after paralyzeTime seconds.
			// (maximum paralyzeTime of all paralyzer weapons which recently hit it ofc)
			const float maxParaDmg = (modInfo.paralyzeOnMaxHealth? maxHealth: health) + maxHealth * CUnit::empDecline * paralyzeTime - paralyzeDamage;
			if (damage > maxParaDmg) {
				if (maxParaDmg > 0.0f) {
					damage = maxParaDmg;
				} else {
					damage = 0.0f;
				}
			}
			if (stunned) {
				experienceMod = 0.0f;
			}
			paralyzeDamage += damage;
			if (paralyzeDamage > (modInfo.paralyzeOnMaxHealth? maxHealth: health)) {
				stunned = true;
			}
		}
		else { // paralyzation healing
			if (paralyzeDamage <= 0.0f) {
				experienceMod = 0.0f;
			}
			paralyzeDamage += damage;

			if (paralyzeDamage < (modInfo.paralyzeOnMaxHealth? maxHealth: health)) {
				stunned = false;
				if (paralyzeDamage < 0.0f) {
					paralyzeDamage = 0.0f;
				}
			}
		}
	}

	if (damage > 0.0f) {
		recentDamage += damage;

		if ((attacker != NULL) && !teamHandler->Ally(allyteam, attacker->allyteam)) {
			attacker->AddExperience(0.1f * experienceMod
			                             * (power / attacker->power)
			                             * (damage + std::min(0.0f, health)) / maxHealth);
		}
	}

	eventHandler.UnitDamaged(this, attacker, damage, weaponDefId, !!damages.paralyzeDamageTime);
	eoh->UnitDamaged(*this, attacker, damage, weaponDefId, !!damages.paralyzeDamageTime);

	if (health <= 0.0f) {
		KillUnit(false, false, attacker);
		if (isDead && (attacker != 0) &&
		    !teamHandler->Ally(allyteam, attacker->allyteam) && !beingBuilt) {
			attacker->AddExperience(expMultiplier * 0.1f * (power / attacker->power));
			teamHandler->Team(attacker->team)->currentStats->unitsKilled++;
		}
	}
//	if(attacker!=0 && attacker->team==team)
//		logOutput.Print("FF by %i %s on %i %s",attacker->id,attacker->tooltip.c_str(),id,tooltip.c_str());

#ifdef TRACE_SYNC
	tracefile << "Damage: ";
	tracefile << id << " " << damage << "\n";
#endif
}



void CUnit::Kill(const float3& impulse) {
	DamageArray da(health);
	DoDamage(da, NULL, impulse, -1);
}



/******************************************************************************/
/******************************************************************************/

CMatrix44f CUnit::GetTransformMatrix(const bool synced, const bool error) const
{
	float3 interPos = synced ? pos : drawPos;

	if (error && !synced && !gu->spectatingFullView) {
		interPos += helper->GetUnitErrorPos(this, gu->myAllyTeam) - midPos;
	}

	CTransportUnit* trans = NULL;

	if (usingScriptMoveType ||
	    (!beingBuilt && (physicalState == CSolidObject::Flying) && unitDef->canmove)) {
		// aircraft, skidding ground unit, or active ScriptMoveType
		// note: (CAirMoveType) aircraft under construction should not
		// use this matrix, or their nanoframes won't spin on pad
		return CMatrix44f(interPos, -rightdir, updir, frontdir);
	}
	else if ((trans = GetTransporter()) != NULL) {
		// we're being transported, transporter sets our vectors
		return CMatrix44f(interPos, -rightdir, updir, frontdir);
	}
	else if (upright || !unitDef->canmove) {
		if (heading == 0) {
			return CMatrix44f(interPos);
		} else {
			// making local copies of vectors
			float3 frontDir = GetVectorFromHeading(heading);
			float3 upDir    = updir;
			float3 rightDir = frontDir.cross(upDir);
			rightDir.Normalize();
			frontDir = upDir.cross(rightDir);
			return CMatrix44f(interPos, -rightdir, updir, frontdir);
		}
	}
	// making local copies of vectors
	float3 frontDir = GetVectorFromHeading(heading);
	float3 upDir    = ground->GetSmoothNormal(pos.x, pos.z);
	float3 rightDir = frontDir.cross(upDir);
	rightDir.Normalize();
	frontDir = upDir.cross(rightDir);
	return CMatrix44f(interPos, -rightDir, upDir, frontDir);
}



/******************************************************************************/
/******************************************************************************/

void CUnit::ChangeSensorRadius(int* valuePtr, int newValue)
{
	radarhandler->RemoveUnit(this);

	*valuePtr = newValue;

	if (newValue != 0) {
		hasRadarCapacity = true;
	} else if (hasRadarCapacity) {
		hasRadarCapacity = (radarRadius   > 0.0f) || (jammerRadius   > 0.0f) ||
		                   (sonarRadius   > 0.0f) || (sonarJamRadius > 0.0f) ||
		                   (seismicRadius > 0.0f);
	}

	radarhandler->MoveUnit(this);
}


void CUnit::AddExperience(float exp)
{
	const float oldExp = experience;
	experience += exp;

	const float oldLimExp = limExperience;
	limExperience = experience / (experience + 1.0f);

	if (expGrade != 0.0f) {
		const int oldGrade = (int)(oldLimExp     / expGrade);
		const int newGrade = (int)(limExperience / expGrade);
		if (oldGrade != newGrade) {
			eventHandler.UnitExperience(this, oldExp);
		}
	}

	if (expPowerScale > 0.0f) {
		power = unitDef->power * (1.0f + (limExperience * expPowerScale));
	}
	if (expReloadScale > 0.0f) {
		reloadSpeed = (1.0f + (limExperience * expReloadScale));
	}
	if (expHealthScale > 0.0f) {
		const float oldMaxHealth = maxHealth;
		maxHealth = unitDef->health * (1.0f + (limExperience * expHealthScale));
		health = health * (maxHealth / oldMaxHealth);
	}
}


void CUnit::DoSeismicPing(float pingSize)
{
	float rx = gs->randFloat();
	float rz = gs->randFloat();

	const float* errorScale = &radarhandler->radarErrorSize[0];
	if (!(losStatus[gu->myAllyTeam] & LOS_INLOS) &&
	    radarhandler->InSeismicDistance(this, gu->myAllyTeam)) {
		const float3 err(errorScale[gu->myAllyTeam] * (0.5f - rx), 0.0f,
		                 errorScale[gu->myAllyTeam] * (0.5f - rz));

		new CSeismicGroundFlash(pos + err, 30, 15, 0, pingSize, 1, float3(0.8f, 0.0f, 0.0f));
	}
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		if (radarhandler->InSeismicDistance(this, a)) {
			const float3 err(errorScale[a] * (0.5f - rx), 0.0f,
			                 errorScale[a] * (0.5f - rz));
			const float3 pingPos = (pos + err);
			eventHandler.UnitSeismicPing(this, a, pingPos, pingSize);
			eoh->SeismicPing(a, *this, pingPos, pingSize);
		}
	}
}


void CUnit::ChangeLos(int l, int airlos)
{
	loshandler->FreeInstance(los);
	los = NULL;
	losRadius = l;
	airLosRadius = airlos;
	loshandler->MoveUnit(this, false);
}


bool CUnit::ChangeTeam(int newteam, ChangeType type)
{
	if (isDead) {
		return false;
	}

	// do not allow unit count violations due to team swapping
	// (this includes unit captures)
	if (uh->unitsByDefs[newteam][unitDef->id].size() >= unitDef->maxThisUnit) {
		return false;
	}

	const bool capture = (type == ChangeCaptured);
	if (luaRules && !luaRules->AllowUnitTransfer(this, newteam, capture)) {
		return false;
	}

	// do not allow old player to keep controlling the unit
	if (directControl) {
		directControl->myController->StopControllingUnit();
		directControl = NULL;
	}

	const int oldteam = team;

	selectedUnits.RemoveUnit(this);
	SetGroup(NULL);

	// reset states and clear the queues
	if (!teamHandler->AlliedTeams(oldteam, newteam)) {
		ChangeTeamReset();
	}

	eventHandler.UnitTaken(this, newteam);
	eoh->UnitCaptured(*this, newteam);

	qf->RemoveUnit(this);
	quads.clear();
	loshandler->FreeInstance(los);
	los = 0;
	losStatus[allyteam] = 0;
	radarhandler->RemoveUnit(this);

	if (unitDef->isAirBase) {
		airBaseHandler->DeregisterAirBase(this);
	}

	if (type == ChangeGiven) {
		teamHandler->Team(oldteam)->RemoveUnit(this, CTeam::RemoveGiven);
		teamHandler->Team(newteam)->AddUnit(this,    CTeam::AddGiven);
	} else {
		teamHandler->Team(oldteam)->RemoveUnit(this, CTeam::RemoveCaptured);
		teamHandler->Team(newteam)->AddUnit(this,    CTeam::AddCaptured);
	}

	if (!beingBuilt) {
		teamHandler->Team(oldteam)->metalStorage  -= metalStorage;
		teamHandler->Team(oldteam)->energyStorage -= energyStorage;

		teamHandler->Team(newteam)->metalStorage  += metalStorage;
		teamHandler->Team(newteam)->energyStorage += energyStorage;
	}


	team = newteam;
	allyteam = teamHandler->AllyTeam(newteam);

	uh->unitsByDefs[oldteam][unitDef->id].erase(this);
	uh->unitsByDefs[newteam][unitDef->id].insert(this);

	neutral = false;

	loshandler->MoveUnit(this,false);
	losStatus[allyteam] = LOS_ALL_MASK_BITS |
		LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;

	qf->MovedUnit(this);
	radarhandler->MoveUnit(this);

	SetLODCount(0);

	if (unitDef->isAirBase) {
		airBaseHandler->RegisterAirBase(this);
	}

	eventHandler.UnitGiven(this, oldteam);
	eoh->UnitGiven(*this, oldteam);

	return true;
}


void CUnit::ChangeTeamReset()
{
	Command c;

	// clear the commands (newUnitCommands for factories)
	c.id = CMD_STOP;
	commandAI->GiveCommand(c);

	// clear the build commands for factories
	CFactoryCAI* facAI = dynamic_cast<CFactoryCAI*>(commandAI);
	if (facAI) {
		c.options = RIGHT_MOUSE_KEY; // clear option
		CCommandQueue& buildCommands = facAI->commandQue;
		CCommandQueue::iterator it;
		std::vector<Command> clearCommands;
		clearCommands.reserve(buildCommands.size());
		for (it = buildCommands.begin(); it != buildCommands.end(); ++it) {
			c.id = it->id;
			clearCommands.push_back(c);
		}
		for (int i = 0; i < (int)clearCommands.size(); i++) {
			facAI->GiveCommand(clearCommands[i]);
		}
	}

	// deactivate to prevent the old give metal maker trick
	c.id = CMD_ONOFF;
	c.params.push_back(0); // always off
	commandAI->GiveCommand(c);
	c.params.clear();
	// reset repeat state
	c.id = CMD_REPEAT;
	c.params.push_back(0);
	commandAI->GiveCommand(c);
	c.params.clear();
	// reset cloak state
	if (unitDef->canCloak) {
		c.id = CMD_CLOAK;
		c.params.push_back(0); // always off
		commandAI->GiveCommand(c);
		c.params.clear();
	}
	// reset move state
	if (unitDef->canmove || unitDef->builder) {
		c.id = CMD_MOVE_STATE;
		c.params.push_back(MOVESTATE_MANEUVER);
		commandAI->GiveCommand(c);
		c.params.clear();
	}
	// reset fire state
	if (commandAI->CanChangeFireState()) {
		c.id = CMD_FIRE_STATE;
		c.params.push_back(FIRESTATE_FIREATWILL);
		commandAI->GiveCommand(c);
		c.params.clear();
	}
	// reset trajectory state
	if (unitDef->highTrajectoryType > 1) {
		c.id = CMD_TRAJECTORY;
		c.params.push_back(0);
		commandAI->GiveCommand(c);
		c.params.clear();
	}
}


bool CUnit::AttackUnit(CUnit *unit,bool dgun)
{
	bool r = false;
	haveDGunRequest = dgun;
	userAttackGround = false;
	commandShotCount = 0;
	SetUserTarget(unit);

	std::vector<CWeapon*>::iterator wi;
	for (wi = weapons.begin(); wi != weapons.end(); ++wi) {
		(*wi)->haveUserTarget = false;
		(*wi)->targetType = Target_None;
		if (haveDGunRequest == (unitDef->canDGun && (*wi)->weaponDef->manualfire)) // == ((!haveDGunRequest && !isDGun) || (haveDGunRequest && isDGun))
			if ((*wi)->AttackUnit(unit, true))
				r = true;
	}
	return r;
}


bool CUnit::AttackGround(const float3& pos, bool dgun)
{
	bool r = false;

	haveDGunRequest = dgun;
	SetUserTarget(0);
	userAttackPos = pos;
	userAttackGround = true;
	commandShotCount = 0;

	for (std::vector<CWeapon*>::iterator wi = weapons.begin(); wi != weapons.end(); ++wi) {
		(*wi)->haveUserTarget = false;

		if (haveDGunRequest == (unitDef->canDGun && (*wi)->weaponDef->manualfire)) { // == ((!haveDGunRequest && !isDGun) || (haveDGunRequest && isDGun))
			if ((*wi)->AttackGround(pos, true)) {
				r = true;
			}
		}
	}

	return r;
}


void CUnit::SetLastAttacker(CUnit* attacker)
{
	if (teamHandler->AlliedTeams(team, attacker->team)) {
		return;
	}
	if (lastAttacker)
		DeleteDeathDependence(lastAttacker, DEPENDENCE_ATTACKER);

	lastAttack = gs->frameNum;
	lastAttacker = attacker;
	if (attacker)
		AddDeathDependence(attacker);
}


void CUnit::DependentDied(CObject* o)
{
	if (o == userTarget)   { userTarget   = NULL; }
	if (o == soloBuilder)  { soloBuilder  = NULL; }
	if (o == transporter)  { transporter  = NULL; }
	if (o == lastAttacker) { lastAttacker = NULL; }

	incomingMissiles.remove((CMissileProjectile*) o);

	CSolidObject::DependentDied(o);
}


void CUnit::SetUserTarget(CUnit* target)
{
	if (userTarget)
		DeleteDeathDependence(userTarget, DEPENDENCE_TARGET);

	userTarget=target;
	for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi) {
		(*wi)->haveUserTarget = false;
	}

	if (target) {
		AddDeathDependence(target);
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
	//Optimization: there's only about one unit that actually needs this information
	if (!script->HasSetSFXOccupy())
		return;

	if (transporter) {
		curTerrainType = 0;
		return;
	}

	float height = ground->GetApproximateHeight(pos.x, pos.z);

	//Deep sea?
	if (height < -5) {
		if (upright)
			curTerrainType = 2;
		else
			curTerrainType = 1;
	}
	//Shore
	else if (height < 0) {
		if (upright)
			curTerrainType = 1;
	}
	//Land
	else {
		curTerrainType = 4;
	}
}


bool CUnit::SetGroup(CGroup* newGroup)
{
	if (group != NULL) {
		group->RemoveUnit(this);
	}
	group = newGroup;

	if (group) {
		if (!group->AddUnit(this)){
			group = NULL; // group ai did not accept us
			return false;
		} else { // add us to selected units, if group is selected

			GML_RECMUTEX_LOCK(sel); // SetGroup

			if (selectedUnits.selectedGroup == group->id) {
				selectedUnits.AddUnit(this);
			}
		}
	}
	return true;
}


bool CUnit::AddBuildPower(float amount, CUnit* builder)
{
	const float part = amount / buildTime;
	const float metalUse  = (metalCost  * part);
	const float energyUse = (energyCost * part);

	if (amount >= 0.0f) { //  build / repair
		if (!beingBuilt && (health >= maxHealth)) {
			return false;
		}

		lastNanoAdd = gs->frameNum;


		if (beingBuilt) { //build
			if ((teamHandler->Team(builder->team)->metal  >= metalUse) &&
			    (teamHandler->Team(builder->team)->energy >= energyUse) &&
			    (!luaRules || luaRules->AllowUnitBuildStep(builder, this, part))) {
				if (builder->UseMetal(metalUse)) { //just because we checked doesn't mean they were deducted since upkeep can prevent deduction
					if (builder->UseEnergy(energyUse)) {
						health += (maxHealth * part);
						buildProgress += part;
						if (buildProgress >= 1.0f) {
							if (health > maxHealth) {
								health = maxHealth;
							}
							FinishedBuilding();
						}
					}
					else {
						builder->UseMetal(-metalUse); //refund the metal if the energy cannot be deducted
					}
				}
				return true;
			} else {
				// update the energy and metal required counts
				teamHandler->Team(builder->team)->metalPull  += metalUse;
				teamHandler->Team(builder->team)->energyPull += energyUse;
			}
			return false;
		}
		else { //repair
			const float energyUseScaled = energyUse * modInfo.repairEnergyCostFactor;

	  		if (!builder->UseEnergy(energyUseScaled)) {
				teamHandler->Team(builder->team)->energyPull += energyUseScaled;
				return false;
			}

			if (health < maxHealth) {
				repairAmount += amount;
				health += maxHealth * part;
				if (health > maxHealth) {
					health = maxHealth;
				}
				return true;
			}
			return false;
		}
	}
	else { // reclaim
		if (isDead) {
			return false;
		}

		if (luaRules && !luaRules->AllowUnitBuildStep(builder, this, part)) {
			return false;
		}

		restTime = 0;

		if (!AllowedReclaim(builder)) {
			builder->DependentDied(this);
			return false;
		}

		const float energyUseScaled = energyUse * modInfo.reclaimUnitEnergyCostFactor;

		if (!builder->UseEnergy(-energyUseScaled)) {
			teamHandler->Team(builder->team)->energyPull += energyUseScaled;
			return false;
		}

		if ((modInfo.reclaimUnitMethod == 0) && (!beingBuilt)) {
			beingBuilt = true;
		}

		health += maxHealth * part;
		if (beingBuilt) {
			builder->AddMetal(-metalUse * modInfo.reclaimUnitEfficiency, false);
			buildProgress+=part;
			if(buildProgress<0 || health<0){
				KillUnit(false, true, NULL);
				return false;
			}
		}


		else {
			if (health < 0) {
				builder->AddMetal(metalCost * modInfo.reclaimUnitEfficiency, false);
				KillUnit(false, true, NULL);
				return false;
			}
		}
		return true;
	}
	return false;
}


void CUnit::FinishedBuilding()
{
	beingBuilt = false;
	buildProgress = 1.0f;

	if (soloBuilder) {
		DeleteDeathDependence(soloBuilder, DEPENDENCE_BUILDER);
		soloBuilder = NULL;
	}

	if (!(immobile && (mass == CSolidObject::DEFAULT_MASS))) {
		mass = unitDef->mass;
	}

	ChangeLos(realLosRadius, realAirLosRadius);

	if (unitDef->activateWhenBuilt) {
		Activate();
	}
	SetMetalStorage(unitDef->metalStorage);
	SetEnergyStorage(unitDef->energyStorage);


	// Sets the frontdir in sync with heading.
	frontdir = GetVectorFromHeading(heading) + float3(0, frontdir.y, 0);

	if (unitDef->isAirBase) {
		airBaseHandler->RegisterAirBase(this);
	}

	eventHandler.UnitFinished(this);
	eoh->UnitFinished(*this);

	if (unitDef->startCloaked) {
		wantCloak = true;
	}

	if (unitDef->isFeature && uh->morphUnitToFeature) {
		UnBlock();
		CFeature* f =
			featureHandler->CreateWreckage(pos, wreckName, heading, buildFacing,
										   0, team, allyteam, false, "");
		if (f) {
			f->blockHeightChanges = true;
		}
		KillUnit(false, true, NULL);
	}
}


void CUnit::KillUnit(bool selfDestruct, bool reclaimed, CUnit* attacker, bool showDeathSequence)
{
	if (isDead) {
		return;
	}

	if (dynamic_cast<CAirMoveType*>(moveType) && !beingBuilt) {
		if (unitDef->canCrash && !selfDestruct && !reclaimed &&
		    (gs->randFloat() > (recentDamage * 0.7f / maxHealth + 0.2f))) {
			((CAirMoveType*) moveType)->SetState(AAirMoveType::AIRCRAFT_CRASHING);
			health = maxHealth * 0.5f;
			return;
		}
	}

	isDead = true;
	deathSpeed = speed;

	eventHandler.UnitDestroyed(this, attacker);
	eoh->UnitDestroyed(*this, attacker);
	// Will be called in the destructor again, but this can not hurt
	this->SetGroup(NULL);

	blockHeightChanges = false;

	if (unitDef->windGenerator > 0.0f) {
		wind.DelUnit(this);
	}

	if (showDeathSequence && (!reclaimed && !beingBuilt)) {
		const std::string& exp = (selfDestruct) ? unitDef->selfDExplosion : unitDef->deathExplosion;

		if (!exp.empty()) {
			const WeaponDef* wd = weaponDefHandler->GetWeapon(exp);
			if (wd) {
				helper->Explosion(
					midPos, wd->damages, wd->areaOfEffect, wd->edgeEffectiveness,
					wd->explosionSpeed, this, true, wd->damages[0] > 500 ? 1 : 2,
					false, false, wd->explosionGenerator, 0, ZeroVector, wd->id
				);

				#if (PLAY_SOUNDS == 1)
				// play explosion sound
				if (wd->soundhit.getID(0) > 0) {
					// HACK: loading code doesn't set sane defaults for explosion sounds, so we do it here
					// NOTE: actually no longer true, loading code always ensures that sound volume != -1
					float volume = wd->soundhit.getVolume(0);
					Channels::Battle.PlaySample(wd->soundhit.getID(0), pos, (volume == -1) ? 1.0f : volume);
				}
				#endif
			}
		}

		if (selfDestruct) {
			recentDamage += maxHealth * 2;
		}

		// start running the unit's kill-script
		script->Killed();
	} else {
		deathScriptFinished = true;
	}

	if (!deathScriptFinished) {
		// put the unit in a pseudo-zombie state until Killed finishes
		speed = ZeroVector;
		stunned = true;
		paralyzeDamage = 100.0f * maxHealth;
		if (health < 0.0f) {
			health = 0.0f;
		}
	}
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
	if (metal < 0) {
		AddMetal(-metal);
		return true;
	}
	teamHandler->Team(team)->metalPull += metal;
	bool canUse = teamHandler->Team(team)->UseMetal(metal);
	if (canUse)
		metalUseI += metal;
	return canUse;
}


void CUnit::AddMetal(float metal, bool useIncomeMultiplier)
{
	if (metal < 0) {
		UseMetal(-metal);
		return;
	}
	metalMakeI += metal;
	teamHandler->Team(team)->AddMetal(metal, useIncomeMultiplier);
}


bool CUnit::UseEnergy(float energy)
{
	if (energy < 0) {
		AddEnergy(-energy);
		return true;
	}
	teamHandler->Team(team)->energyPull += energy;
	bool canUse = teamHandler->Team(team)->UseEnergy(energy);
	if (canUse)
		energyUseI += energy;
	return canUse;
}


void CUnit::AddEnergy(float energy, bool useIncomeMultiplier)
{
	if (energy < 0) {
		UseEnergy(-energy);
		return;
	}
	energyMakeI += energy;
	teamHandler->Team(team)->AddEnergy(energy, useIncomeMultiplier);
}



void CUnit::Activate()
{
	if (activated)
		return;

	activated = true;
	script->Activate();

	if (unitDef->targfac) {
		radarhandler->radarErrorSize[allyteam] /= radarhandler->targFacEffect;
	}

	radarhandler->MoveUnit(this);

	#if (PLAY_SOUNDS == 1)
	int soundIdx = unitDef->sounds.activate.getRandomIdx();
	if (soundIdx >= 0) {
		Channels::UnitReply.PlaySample(
			unitDef->sounds.activate.getID(soundIdx), this,
			unitDef->sounds.activate.getVolume(soundIdx));
	}
	#endif
}

void CUnit::Deactivate()
{
	if (!activated)
		return;

	activated = false;
	script->Deactivate();

	if (unitDef->targfac) {
		radarhandler->radarErrorSize[allyteam] *= radarhandler->targFacEffect;
	}

	radarhandler->RemoveUnit(this);

	#if (PLAY_SOUNDS == 1)
	int soundIdx = unitDef->sounds.deactivate.getRandomIdx();
	if (soundIdx >= 0) {
		Channels::UnitReply.PlaySample(
			unitDef->sounds.deactivate.getID(soundIdx), this,
			unitDef->sounds.deactivate.getVolume(soundIdx));
	}
	#endif
}



void CUnit::UpdateWind(float x, float z, float strength)
{
	const float windHeading = ClampRad(GetHeadingFromVectorF(-x, -z) - heading * TAANG2RAD);
	const float windStrength = std::min(strength, unitDef->windGenerator);

	script->WindChanged(windHeading, windStrength);
}


void CUnit::LoadSave(CLoadSaveInterface* file, bool loading)
{
	file->lsShort(heading);
	file->lsFloat(buildProgress);
	file->lsFloat(health);
	file->lsFloat(experience);
	if (loading) {
		const float exp = experience;
		experience = 0.0f;
		limExperience = 0.0f;
		AddExperience(exp);
	}
	file->lsInt(moveState);
	file->lsInt(fireState);
	file->lsBool(isCloaked);
	file->lsBool(wantCloak);
	commandAI->LoadSave(file, loading);
}


void CUnit::IncomingMissile(CMissileProjectile* missile)
{
	if (unitDef->canDropFlare) {
		incomingMissiles.push_back(missile);
		AddDeathDependence(missile);

		if (lastFlareDrop < (gs->frameNum - unitDef->flareReloadTime * 30)) {
			new CFlareProjectile(pos, speed, this, (int) (gs->frameNum + unitDef->flareDelay * (1 + gs->randFloat()) * 15));
			lastFlareDrop = gs->frameNum;
		}
	}
}


void CUnit::TempHoldFire()
{
	dontFire = true;
	AttackUnit(0, true);
}


void CUnit::ReleaseTempHoldFire()
{
	dontFire = false;
}


/******************************************************************************/

float CUnit::lodFactor = 1.0f;


void CUnit::SetLODFactor(float value)
{
	lodFactor = (value * camera->lppScale);
}


void CUnit::SetLODCount(unsigned int count)
{
	const unsigned int oldCount = lodCount;

	lodCount = count;

	lodLengths.resize(count);
	for (unsigned int i = oldCount; i < count; i++) {
		lodLengths[i] = -1.0f;
	}

	localmodel->SetLODCount(count);

	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		luaMats[m].SetLODCount(count);
	}

	return;
}


unsigned int CUnit::CalcLOD(unsigned int lastLOD) const
{
	if (lastLOD == 0) { return 0; }

	const float3 diff = (pos - camera->pos);
	const float dist = diff.dot(camera->forward);
	const float lpp = std::max(0.0f, dist * lodFactor);
	for (/* no-op */; lastLOD != 0; lastLOD--) {
		if (lpp > lodLengths[lastLOD]) {
			break;
		}
	}
	return lastLOD;
}


unsigned int CUnit::CalcShadowLOD(unsigned int lastLOD) const
{
	return CalcLOD(lastLOD); // FIXME

	// FIXME -- the more 'correct' method
	if (lastLOD == 0) { return 0; }

	// FIXME: fix it, cap it for shallow shadows?
	const float3& sun = mapInfo->light.sunDir;
	const float3 diff = (camera->pos - pos);
	const float  dot  = diff.dot(sun);
	const float3 gap  = diff - (sun * dot);
	const float  lpp  = std::max(0.0f, gap.Length() * lodFactor);

	for (/* no-op */; lastLOD != 0; lastLOD--) {
		if (lpp > lodLengths[lastLOD]) {
			break;
		}
	}
	return lastLOD;
}


/******************************************************************************/

void CUnit::PostLoad()
{
	//HACK:Initializing after load
	unitDef = unitDefHandler->GetUnitDefByName(unitDefName);

	curYardMap = (unitDef->yardmaps[buildFacing].empty())? NULL: &unitDef->yardmaps[buildFacing][0];

	SetRadius((model = unitDef->LoadModel())->radius);

	modelParser->CreateLocalModel(this);
	// FIXME: how to handle other script types (e.g. Lua) here?
	script = CUnitScriptFactory::CreateScript(unitDef->scriptPath, this);

	// Call initializing script functions
	script->Create();

	for (vector<CWeapon*>::iterator i = weapons.begin(); i != weapons.end(); ++i) {
		(*i)->weaponDef = unitDef->weapons[(*i)->weaponNum].def;
	}

	script->SetSFXOccupy(curTerrainType);

	if (unitDef->windGenerator > 0.0f) {
		wind.AddUnit(this);
	}

	if (activated) {
		script->Activate();
	}
}


void CUnit::StopAttackingAllyTeam(int ally)
{
	commandAI->StopAttackingAllyTeam(ally);
	for (std::vector<CWeapon*>::iterator it = weapons.begin(); it != weapons.end(); ++it) {
		(*it)->StopAttackingAllyTeam(ally);
	}
}

void CUnit::SlowUpdateCloak(bool stunCheck)
{
	const bool oldCloak = isCloaked;

	if (stunCheck) {
		if (stunned && isCloaked && scriptCloak <= 3) {
			isCloaked = false;
		}
	} else {
		if (scriptCloak >= 3) {
			isCloaked = true;
		} else if (wantCloak || (scriptCloak >= 1)) {
			if ((decloakDistance > 0.0f) &&
				helper->GetClosestEnemyUnitNoLosTest(midPos, decloakDistance, allyteam, unitDef->decloakSpherical, false)) {
				curCloakTimeout = gs->frameNum + cloakTimeout;
				isCloaked = false;
			}
			if (isCloaked || (gs->frameNum >= curCloakTimeout)) {
				if (scriptCloak >= 2) {
					isCloaked = true;
				}
				else {
					float cloakCost = unitDef->cloakCost;
					if (speed.SqLength() > 0.2f) {
						cloakCost = unitDef->cloakCostMoving;
					}
					if (UseEnergy(cloakCost * 0.5f)) {
						isCloaked = true;
					} else {
						isCloaked = false;
					}
				}
			} else {
				isCloaked = false;
			}
		} else {
			isCloaked = false;
		}
	}

	if (oldCloak != isCloaked) {
		if (isCloaked) {
			eventHandler.UnitCloaked(this);
		} else {
			eventHandler.UnitDecloaked(this);
		}
	}
}

void CUnit::ScriptDecloak(bool updateCloakTimeOut)
{
	if (scriptCloak <= 2) {
		if (isCloaked) {
			isCloaked = false;
			eventHandler.UnitDecloaked(this);
		}

		if (updateCloakTimeOut) {
			curCloakTimeout = gs->frameNum + cloakTimeout;
		}
	}
}


void CUnit::DeleteDeathDependence(CObject* o, DependenceType dep) {
	switch(dep) {
		case DEPENDENCE_BUILD:
		case DEPENDENCE_CAPTURE:
		case DEPENDENCE_RECLAIM:
		case DEPENDENCE_TERRAFORM:
		case DEPENDENCE_TRANSPORTEE:
		case DEPENDENCE_TRANSPORTER:
			if(o == lastAttacker || o == soloBuilder || o == userTarget) return;
			break;
		case DEPENDENCE_ATTACKER:
			if(o == soloBuilder || o == userTarget) return;
			break;
		case DEPENDENCE_BUILDER:
			if(o == lastAttacker || o == userTarget) return;
			break;
		case DEPENDENCE_RESURRECT:
			// feature
			break;
		case DEPENDENCE_TARGET:
			if(o == lastAttacker || o == soloBuilder) return;
			break;
	}
	CObject::DeleteDeathDependence(o);
}


// Member bindings
CR_REG_METADATA(CUnit, (
	//CR_MEMBER(unitDef),
	CR_MEMBER(unitDefName),
	CR_MEMBER(collisionVolume),
	CR_MEMBER(frontdir),
	CR_MEMBER(rightdir),
	CR_MEMBER(updir),
	CR_MEMBER(upright),
	CR_MEMBER(relMidPos),
	CR_MEMBER(deathSpeed),
	CR_MEMBER(travel),
	CR_MEMBER(travelPeriod),
	CR_MEMBER(power),
	CR_MEMBER(paralyzeDamage),
	CR_MEMBER(captureProgress),
	CR_MEMBER(maxHealth),
	CR_MEMBER(health),
	CR_MEMBER(experience),
	CR_MEMBER(limExperience),
	CR_MEMBER(neutral),
	CR_MEMBER(soloBuilder),
	CR_MEMBER(beingBuilt),
	CR_MEMBER(lastNanoAdd),
	CR_MEMBER(repairAmount),
	CR_MEMBER(transporter),
	CR_MEMBER(toBeTransported),
	CR_MEMBER(buildProgress),
	CR_MEMBER(groundLevelled),
	CR_MEMBER(terraformLeft),
	CR_MEMBER(realLosRadius),
	CR_MEMBER(realAirLosRadius),
	CR_MEMBER(losStatus),
	CR_MEMBER(inBuildStance),
	CR_MEMBER(stunned),
	CR_MEMBER(useHighTrajectory),
	CR_MEMBER(dontUseWeapons),
	CR_MEMBER(deathScriptFinished),
	CR_MEMBER(delayedWreckLevel),
	CR_MEMBER(restTime),
	CR_MEMBER(weapons),
	CR_MEMBER(shieldWeapon),
	CR_MEMBER(stockpileWeapon),
	CR_MEMBER(reloadSpeed),
	CR_MEMBER(maxRange),
	CR_MEMBER(haveTarget),
	CR_MEMBER(haveUserTarget),
	CR_MEMBER(haveDGunRequest),
	CR_MEMBER(lastMuzzleFlameSize),
	CR_MEMBER(lastMuzzleFlameDir),
	CR_MEMBER(armorType),
	CR_MEMBER(category),
	CR_MEMBER(quads),
	CR_MEMBER(los),
	CR_MEMBER(tempNum),
	CR_MEMBER(lastSlowUpdate),
	CR_MEMBER(mapSquare),
	CR_MEMBER(losRadius),
	CR_MEMBER(airLosRadius),
	CR_MEMBER(losHeight),
	CR_MEMBER(lastLosUpdate),
	CR_MEMBER(radarRadius),
	CR_MEMBER(sonarRadius),
	CR_MEMBER(jammerRadius),
	CR_MEMBER(sonarJamRadius),
	CR_MEMBER(seismicRadius),
	CR_MEMBER(seismicSignature),
	CR_MEMBER(hasRadarCapacity),
	CR_MEMBER(radarSquares),
	CR_MEMBER(oldRadarPos.x),
	CR_MEMBER(oldRadarPos.y),
	CR_MEMBER(hasRadarPos),
	CR_MEMBER(stealth),
	CR_MEMBER(sonarStealth),
	CR_MEMBER(moveType),
	CR_MEMBER(prevMoveType),
	CR_MEMBER(usingScriptMoveType),
	CR_MEMBER(commandAI),
	CR_MEMBER(group),
	CR_MEMBER(condUseMetal),
	CR_MEMBER(condUseEnergy),
	CR_MEMBER(condMakeMetal),
	CR_MEMBER(condMakeEnergy),
	CR_MEMBER(uncondUseMetal),
	CR_MEMBER(uncondUseEnergy),
	CR_MEMBER(uncondMakeMetal),
	CR_MEMBER(uncondMakeEnergy),
	CR_MEMBER(metalUse),
	CR_MEMBER(energyUse),
	CR_MEMBER(metalMake),
	CR_MEMBER(energyMake),
	CR_MEMBER(metalUseI),
	CR_MEMBER(energyUseI),
	CR_MEMBER(metalMakeI),
	CR_MEMBER(energyMakeI),
	CR_MEMBER(metalUseold),
	CR_MEMBER(energyUseold),
	CR_MEMBER(metalMakeold),
	CR_MEMBER(energyMakeold),
	CR_MEMBER(energyTickMake),
	CR_MEMBER(metalExtract),
	CR_MEMBER(metalCost),
	CR_MEMBER(energyCost),
	CR_MEMBER(buildTime),
	CR_MEMBER(metalStorage),
	CR_MEMBER(energyStorage),
	CR_MEMBER(lastAttacker),
	// CR_MEMBER(lastAttackedPiece),
	CR_MEMBER(lastAttackedPieceFrame),
	CR_MEMBER(lastAttack),
	CR_MEMBER(lastFireWeapon),
	CR_MEMBER(recentDamage),
	CR_MEMBER(userTarget),
	CR_MEMBER(userAttackPos),
	CR_MEMBER(userAttackGround),
	CR_MEMBER(commandShotCount),
	CR_MEMBER(fireState),
	CR_MEMBER(dontFire),
	CR_MEMBER(moveState),
	CR_MEMBER(activated),
//#if defined(USE_GML) && GML_ENABLE_SIM
//	CR_MEMBER(lastUnitUpdate),
//#endif
	//CR_MEMBER(model),
	//CR_MEMBER(localmodel),
	//CR_MEMBER(cob),
	//CR_MEMBER(script),
	CR_MEMBER(tooltip),
	CR_MEMBER(crashing),
	CR_MEMBER(isDead),
	CR_MEMBER(falling),
	CR_MEMBER(fallSpeed),
	CR_MEMBER(inAir),
	CR_MEMBER(inWater),
	CR_MEMBER(flankingBonusMode),
	CR_MEMBER(flankingBonusDir),
	CR_MEMBER(flankingBonusMobility),
	CR_MEMBER(flankingBonusMobilityAdd),
	CR_MEMBER(flankingBonusAvgDamage),
	CR_MEMBER(flankingBonusDifDamage),
	CR_MEMBER(armoredState),
	CR_MEMBER(armoredMultiple),
	CR_MEMBER(curArmorMultiple),
	CR_MEMBER(wreckName),
	CR_MEMBER(posErrorVector),
	CR_MEMBER(posErrorDelta),
	CR_MEMBER(nextPosErrorUpdate),
	CR_MEMBER(hasUWWeapons),
	CR_MEMBER(wantCloak),
	CR_MEMBER(scriptCloak),
	CR_MEMBER(cloakTimeout),
	CR_MEMBER(curCloakTimeout),
	CR_MEMBER(isCloaked),
	CR_MEMBER(decloakDistance),
	CR_MEMBER(lastTerrainType),
	CR_MEMBER(curTerrainType),
	CR_MEMBER(selfDCountdown),
//	CR_MEMBER(directControl),
	//CR_MEMBER(myTrack), // unused
	CR_MEMBER(incomingMissiles),
	CR_MEMBER(lastFlareDrop),
	CR_MEMBER(currentFuel),
	CR_MEMBER(luaDraw),
	CR_MEMBER(noDraw),
	CR_MEMBER(noSelect),
	CR_MEMBER(noMinimap),
//	CR_MEMBER(isIcon),
//	CR_MEMBER(iconRadius),
	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxReverseSpeed),
//	CR_MEMBER(weaponHitMod),
//	CR_MEMBER(lodCount),
//	CR_MEMBER(currentLOD),
//	CR_MEMBER(lodLengths),
//	CR_MEMBER(luaMats),
	CR_MEMBER(alphaThreshold),
	CR_MEMBER(cegDamage),
//	CR_MEMBER(lastDrawFrame),
//	CR_MEMBER(lodFactor), // unsynced
//	CR_MEMBER(expMultiplier),
//	CR_MEMBER(expPowerScale),
//	CR_MEMBER(expHealthScale),
//	CR_MEMBER(expReloadScale),
//	CR_MEMBER(expGrade),

	CR_POSTLOAD(PostLoad)
	));
