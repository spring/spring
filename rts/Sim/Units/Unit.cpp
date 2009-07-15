// Unit.cpp: implementation of the CUnit class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "creg/STL_List.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/MiniMap.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MetalMap.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/UnitModels/IModelParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GroundFlash.h"

#include "Sim/Units/Groups/Group.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sync/SyncedPrimitive.h"
#include "Sync/SyncTracer.h"
#include "EventHandler.h"
#include "LoadSaveInterface.h"
#include "LogOutput.h"
#include "Matrix44f.h"
#include "myMath.h"
#include "Sound/AudioChannel.h"
#include "UnitDef.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "UnitLoader.h"
#include "UnitTypes/Building.h"
#include "UnitTypes/TransportUnit.h"

#include "COB/UnitScript.h"
#include "COB/UnitScriptFactory.h"
#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/MobileCAI.h"
#include "CommandAI/TransportCAI.h"
#include "UnitDefHandler.h"

CLogSubsystem LOG_UNIT("unit");

CR_BIND_DERIVED(CUnit, CSolidObject, );

// See end of source for member bindings
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

float CUnit::expMultiplier  = 1.0f;
float CUnit::expPowerScale  = 1.0f;
float CUnit::expHealthScale = 0.7f;
float CUnit::expReloadScale = 0.4f;
float CUnit::expGrade = 0.0f;


CUnit::CUnit():
	unitDef(0),
	collisionVolume(0),
	team(0),
	allyteam(0),
	frontdir(0,0,1),
	rightdir(-1,0,0),
	updir(0,1,0),
	upright(true),
	relMidPos(0,0,0),
	travel(0.0f),
	travelPeriod(0.0f),
	power(100),
	maxHealth(100),
	health(100),
	paralyzeDamage(0),
	captureProgress(0),
	experience(0),
	limExperience(0),
	neutral(false),
	soloBuilder(NULL),
	beingBuilt(true),
	lastNanoAdd(gs->frameNum),
	repairAmount(0.0f),
	transporter(0),
	toBeTransported(false),
	buildProgress(0),
	groundLevelled(true),
	terraformLeft(0),
	realLosRadius(0),
	realAirLosRadius(0),
	losStatus(teamHandler->ActiveAllyTeams(), 0),
	inBuildStance(false),
	stunned(false),
	useHighTrajectory(false),
	dontUseWeapons(false),
	deathScriptFinished(false),
	deathCountdown(0),
	delayedWreckLevel(-1),
	restTime(0),
	shieldWeapon(0),
	stockpileWeapon(0),
	reloadSpeed(1),
	maxRange(0),
	haveTarget(false),
	haveDGunRequest(false),
	lastMuzzleFlameSize(0),
	lastMuzzleFlameDir(0,1,0),
	armorType(0),
	category(0),
	los(0),
	tempNum(0),
	lastSlowUpdate(0),
	controlRadius(2),
	losRadius(0),
	airLosRadius(0),
	losHeight(0),
	lastLosUpdate(0),
	radarRadius(0),
	sonarRadius(0),
	jammerRadius(0),
	sonarJamRadius(0),
	hasRadarCapacity(false),
	prevMoveType(NULL),
	usingScriptMoveType(false),
	commandAI(0),
	group(0),
	condUseMetal(0.0f),
	condUseEnergy(0.0f),
	condMakeMetal(0.0f),
	condMakeEnergy(0.0f),
	uncondUseMetal(0.0f),
	uncondUseEnergy(0.0f),
	uncondMakeMetal(0.0f),
	uncondMakeEnergy(0.0f),
	metalUse(0),
	energyUse(0),
	metalMake(0),
	energyMake(0),
	metalUseI(0),
	energyUseI(0),
	metalMakeI(0),
	energyMakeI(0),
	metalUseold(0),
	energyUseold(0),
	metalMakeold(0),
	energyMakeold(0),
	energyTickMake(0),
	metalExtract(0),
	metalCost(100),
	energyCost(0),
	buildTime(100),
	metalStorage(0),
	energyStorage(0),
	lastAttacker(0),
	lastAttackedPiece(0),
	lastAttackedPieceFrame(-1),
	lastAttack(-200),
	lastFireWeapon(0),
	recentDamage(0),
	userTarget(0),
	userAttackPos(0,0,0),
	userAttackGround(false),
	commandShotCount(-1),
	fireState(2),
	dontFire(false),
	moveState(0),
	script(NULL),
	crashing(false),
	isDead(false),
	falling(false),
	fallSpeed(0.2),
	inAir(false),
	inWater(false),
	flankingBonusMode(0),
	flankingBonusDir(1.0f, 0.0f, 0.0f),
	flankingBonusMobility(10.0f),
	flankingBonusMobilityAdd(0.01f),
	flankingBonusAvgDamage(1.4f),
	flankingBonusDifDamage(0.5f),
	armoredState(false),
	armoredMultiple(1),
	curArmorMultiple(1),
	posErrorDelta(0,0,0),
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
	myTrack(0),
	lastFlareDrop(0),
	currentFuel(0),
	luaDraw(false),
	noDraw(false),
	noSelect(false),
	noMinimap(false),
	isIcon(false),
	iconRadius(0),
	lodCount(0),
	currentLOD(0),
	alphaThreshold(0.1f),
	cegDamage(1)
#ifdef USE_GML
	, lastDrawFrame(-30)
#endif
{
	directControl = NULL;
	activated = false;
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
		                               unitDef->name, deathSpeed);
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
	delete collisionVolume; collisionVolume = NULL;

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
	los = 0;

	if (hasRadarCapacity) {
		radarhandler->RemoveUnit(this);
	}

	delete script;
	modelParser->DeleteLocalModel(this);
}


void CUnit::SetMetalStorage(float newStorage)
{
	teamHandler->Team(team)->metalStorage-=metalStorage;
	metalStorage = newStorage;
	teamHandler->Team(team)->metalStorage+=metalStorage;
}


void CUnit::SetEnergyStorage(float newStorage)
{
	teamHandler->Team(team)->energyStorage-=energyStorage;
	energyStorage = newStorage;
	teamHandler->Team(team)->energyStorage+=energyStorage;
}


void CUnit::UnitInit(const UnitDef* def, int Team, const float3& position)
{
	pos = position;
	team = Team;
	allyteam = teamHandler->AllyTeam(Team);
	lineage = Team;
	unitDef = def;
	unitDefName = unitDef->name;

	localmodel = NULL;
	SetRadius(1.2f);
	mapSquare = ground->GetSquare(pos);
	uh->AddUnit(this);
	qf->MovedUnit(this);
	hasRadarPos = false;

	losStatus[allyteam] = LOS_ALL_MASK_BITS |
		LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;

	posErrorVector = gs->randVector();
	posErrorVector.y *= 0.2f;
#ifdef TRACE_SYNC
	tracefile << "New unit: " << unitDefName.c_str() << " ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif
	ASSERT_SYNCED_FLOAT3(pos);
}


void CUnit::ForcedMove(const float3& newPos)
{
	// hack to make mines not block ground
	const bool blocking = !unitDef->canKamikaze || (unitDef->type == "Building" || unitDef->type == "Factory");
	if (blocking) {
		UnBlock();
	}

	CBuilding* building = dynamic_cast<CBuilding*>(this);
	if (building && unitDef->useBuildingGroundDecal) {
		groundDecals->RemoveBuilding(building, NULL);
	}

	pos = newPos;
	UpdateMidPos();

	if (building && unitDef->useBuildingGroundDecal) {
		groundDecals->AddBuilding(building);
	}

	if (blocking) {
		Block();
	}

	qf->MovedUnit(this);
	loshandler->MoveUnit(this, false);
	if (hasRadarCapacity) {
		radarhandler->MoveUnit(this);
	}
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
	midPos = pos + (frontdir * relMidPos.z) + (updir * relMidPos.y)
		+ (rightdir * relMidPos.x);
}


void CUnit::Drop(float3 parentPos,float3 parentDir, CUnit* parent)
{
	//drop unit from position
	fallSpeed = unitDef->unitFallSpeed > 0 ? unitDef->unitFallSpeed : parent->unitDef->fallSpeed;
	falling = true;
	pos.y = parentPos.y - height;
	frontdir = parentDir;
	frontdir.y = 0;
	speed.y = 0;
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
		moveType->SetGoal(pos);
		moveType->StopMoving();
	}

	// ??
	CMobileCAI* mobile = dynamic_cast<CMobileCAI*>(moveType);
	if (mobile) {
		mobile->lastUserGoal = pos;
	}
}


void CUnit::Update()
{
	ASSERT_SYNCED_FLOAT3(pos);

	posErrorVector += posErrorDelta;

	if (deathScriptFinished) {
		// if our kill-script is already finished, don't
		// wait for the deathCountdown to reach zero and
		// just delete us ASAP (costs one extra frame)
		uh->DeleteUnit(this);
		return;
	}

	if (deathCountdown) {
		--deathCountdown;

		if (!deathCountdown) {
			if (deathScriptFinished) {
				// kill-script has terminated, remove unit now
				uh->DeleteUnit(this);
			} else {
				// kill-script still running, delay one more frame
				deathCountdown = 1;
			}
		}
		return;
	}

	if (beingBuilt) {
		return;
	}

	const bool oldInAir   = inAir;
	const bool oldInWater = inWater;

	moveType->Update();
	GML_GET_TICKS(lastUnitUpdate);

	inAir   = ((pos.y + height) >= 0.0f);
	inWater =  (pos.y           <= 0.0f);

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
	losStatus[at] |= newStatus;

	if (diffBits) {
		if (diffBits & LOS_INLOS) {
			if (newStatus & LOS_INLOS) {
				eventHandler.UnitEnteredLos(this, at);
				eoh->UnitEnteredLos(*this, at);
			} else {
				eventHandler.UnitLeftLos(this, at);
				eoh->UnitLeftLos(*this, at);
			}
		}

		if (diffBits & LOS_INRADAR) {
			if (newStatus & LOS_INRADAR) {
				eventHandler.UnitEnteredRadar(this, at);
				eoh->UnitEnteredRadar(*this, at);
			} else {
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

	repairAmount=0.0f;

	if (paralyzeDamage > 0) {
		paralyzeDamage -= maxHealth * (16.0f / GAME_SPEED / 40.0f);
		if (paralyzeDamage < 0) {
			paralyzeDamage = 0;
		}
	}

	if (stunned) {
		// de-stun only if we are not (still) inside a non-firebase transport
		if (paralyzeDamage < health && !(transporter && !transporter->unitDef->isFirePlatform) ) {
			stunned = false;
		}
		const bool oldCloak = isCloaked;
		if (!isDead && (scriptCloak >= 4)) {
			isCloaked = true;
		} else {
			isCloaked = false;
		}
		if (oldCloak != isCloaked) {
			if (isCloaked) {
				eventHandler.UnitCloaked(this);
			} else {
				eventHandler.UnitDecloaked(this);
			}
		}
		UpdateResources();
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
			UpdateResources();
		}

		// damage nano-frames too
		DoWaterDamage();
		return;
	}

	// below is stuff that should not be run while being built

	lastSlowUpdate=gs->frameNum;

	commandAI->SlowUpdate();
	moveType->SlowUpdate();

	UpdateResources();

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

	AddMetal(unitDef->metalMake*0.5f);
	if (activated) {
		if (UseEnergy(unitDef->energyUpkeep * 0.5f)) {
			if (unitDef->isMetalMaker) {
				AddMetal(unitDef->makesMetal * 0.5f * uh->metalMakerEfficiency);
				uh->metalMakerIncome += unitDef->makesMetal;
			} else {
				AddMetal(unitDef->makesMetal * 0.5f);
			}
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

	residualImpulse *= 0.6f;

	const bool oldCloak = isCloaked;

	if (scriptCloak >= 3) {
		isCloaked = true;
	}
	else if (wantCloak || (scriptCloak >= 1)) {
		if ((decloakDistance > 0.0f) &&
		    helper->GetClosestEnemyUnitNoLosTest(midPos, decloakDistance,
		                                         allyteam, unitDef->decloakSpherical, false)) {
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

	if (oldCloak != isCloaked) {
		if (isCloaked) {
			eventHandler.UnitCloaked(this);
		} else {
			eventHandler.UnitDecloaked(this);
		}
	}

	DoWaterDamage();

	if (unitDef->canKamikaze) {
		if (fireState >= 2) {
			CUnit* u = helper->GetClosestEnemyUnitNoLosTest(pos, unitDef->kamikazeDist, allyteam, false, true);
			if (u && u->physicalState != CSolidObject::Flying && u->speed.dot(pos - u->pos) <= 0) {
				// self destruct when unit start moving away from mine, should maximize damage
				KillUnit(true, false, NULL);
			}
		}
		if (userTarget && (userTarget->pos.SqDistance(pos) < Square(unitDef->kamikazeDist)))
			KillUnit(true, false, NULL);
		if (userAttackGround && (userAttackPos.distance(pos)) < Square(unitDef->kamikazeDist))
			KillUnit(true, false, NULL);
	}

	if(!weapons.empty()){
		haveTarget=false;
		haveUserTarget=false;

		// aircraft and ScriptMoveType do not want this
		if (moveType->useHeading) {
			SetDirectionFromHeading();
		}

		if(!dontFire){
			for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi){
				CWeapon* w=*wi;
				if(userTarget && !w->haveUserTarget && (haveDGunRequest || !unitDef->canDGun || !w->weaponDef->manualfire))
					w->AttackUnit(userTarget,true);
				else if(userAttackGround && !w->haveUserTarget && (haveDGunRequest || !unitDef->canDGun || !w->weaponDef->manualfire))
					w->AttackGround(userAttackPos,true);

				w->SlowUpdate();

				if(w->targetType==Target_None && fireState>0 && lastAttacker && lastAttack+200>gs->frameNum)
					w->AttackUnit(lastAttacker,false);
			}
		}
	}

	if (moveType->progressState == AMoveType::Active ) {
		if (seismicSignature && !GetTransporter()) {
			DoSeismicPing((int)seismicSignature);
		}
	}

	CalculateTerrainType();
	UpdateTerrainType();
}


void CUnit::SetDirectionFromHeading(void)
{
	frontdir=GetVectorFromHeading(heading);
	if(transporter && transporter->unitDef->holdSteady) {
		updir = transporter->updir;
		rightdir=frontdir.cross(updir);
		rightdir.Normalize();
		frontdir=updir.cross(rightdir);
	} else if(upright || !unitDef->canmove){
		updir=UpVector;
		rightdir=frontdir.cross(updir);
	} else  {
		updir=ground->GetNormal(pos.x,pos.z);
		rightdir=frontdir.cross(updir);
		rightdir.Normalize();
		frontdir=updir.cross(rightdir);
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
				DoDamage(DamageArray() * uh->waterDamage, 0, ZeroVector, -1);
			}
		}
	}
}

void CUnit::DoDamage(const DamageArray& damages, CUnit *attacker,const float3& impulse, int weaponId)
{
	if (isDead) {
		return;
	}

	residualImpulse += impulse / mass;
	moveType->ImpulseAdded();

	float damage = damages[armorType];

	if (damage > 0.0f) {
		if (attacker) {
			SetLastAttacker(attacker);
			if (flankingBonusMode) {
				const float3 adir = (attacker->pos - pos).Normalize(); // FIXME -- not the impulse direction?
				if (flankingBonusMode == 1) {		// mode 1 = global coordinates, mobile
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

	float3 hitDir = impulse;
	hitDir.y = 0.0f;
	hitDir = -hitDir.Normalize();

	if (script->HasFunction(COBFN_HitByWeaponId)) {
		script->HitByWeaponId(hitDir, weaponId, /*inout*/ damage);
	}
	else {
		script->HitByWeapon(hitDir);
	}

	float experienceMod = expMultiplier;

	const int paralyzeTime = damages.paralyzeDamageTime;
	float newDamage = damage;

	if (luaRules && luaRules->UnitPreDamaged(this, attacker, damage, weaponId,
			!!damages.paralyzeDamageTime, &newDamage))
		damage = newDamage;


	if (paralyzeTime == 0) { // real damage
		if (damage > 0.0f) {
			// Dont log overkill damage (so dguns/nukes etc dont inflate values)
			const float statsdamage = std::max(0.0f, std::min(maxHealth - health, damage));
			if (attacker) {
				teamHandler->Team(attacker->team)->currentStats.damageDealt += statsdamage;
			}
			teamHandler->Team(team)->currentStats.damageReceived += statsdamage;
			health -= damage;
		}
		else { // healing
			health -= damage;
			if (health > maxHealth) {
				health = maxHealth;
			}
			if (health > paralyzeDamage) {
				stunned = false;
			}
		}
	}
	else { // paralyzation
		experienceMod *= 0.1f; // reduced experience
		if (damage > 0.0f) {
			const float maxParaDmg = health + (maxHealth * 0.025f * (float)paralyzeTime);
			if (paralyzeDamage >= maxParaDmg || stunned) {
				experienceMod = 0.0f;
			}

			paralyzeDamage += damage;
			if (paralyzeDamage > health) {
				stunned = true;
			}
			paralyzeDamage = std::min(paralyzeDamage, maxParaDmg);

		}
		else { // paralyzation healing
			if (paralyzeDamage <= 0.0f) {
				experienceMod = 0.0f;
			}
			paralyzeDamage += damage;
			if (paralyzeDamage < health) {
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
			const int warnFrame = (gs->frameNum - 100);
			if ((team == gu->myTeam)
			    && ((!unitDef->isCommander && (uh->lastDamageWarning < warnFrame)) ||
			        ( unitDef->isCommander && (uh->lastCmdDamageWarning < warnFrame)))
					&& !camera->InView(midPos, radius + 50) && !gu->spectatingFullView) {
				logOutput.Print("%s is being attacked", unitDef->humanName.c_str());
				logOutput.SetLastMsgPos(pos);

				if (unitDef->isCommander || uh->lastDamageWarning + 150 < gs->frameNum) {
					const int soundIdx = unitDef->sounds.underattack.getRandomIdx();
					if (soundIdx >= 0) {
						Channels::UserInterface.PlaySample(
							unitDef->sounds.underattack.getID(soundIdx),
							unitDef->isCommander ? 4 : 2);
					}
				}

				minimap->AddNotification(pos, float3(1.0f, 0.3f, 0.3f),
				                         unitDef->isCommander ? 1.0f : 0.5f);

				uh->lastDamageWarning = gs->frameNum;
				if (unitDef->isCommander) {
					uh->lastCmdDamageWarning = gs->frameNum;
				}
			}
		}
	}

	eventHandler.UnitDamaged(this, attacker, damage, weaponId, !!damages.paralyzeDamageTime);
	eoh->UnitDamaged(*this, attacker, damage);

	if (health <= 0.0f) {
		KillUnit(false, false, attacker);
		if (isDead && (attacker != 0) &&
		    !teamHandler->Ally(allyteam, attacker->allyteam) && !beingBuilt) {
			attacker->AddExperience(expMultiplier * 0.1f * (power / attacker->power));
			teamHandler->Team(attacker->team)->currentStats.unitsKilled++;
		}
	}
//	if(attacker!=0 && attacker->team==team)
//		logOutput.Print("FF by %i %s on %i %s",attacker->id,attacker->tooltip.c_str(),id,tooltip.c_str());

#ifdef TRACE_SYNC
	tracefile << "Damage: ";
	tracefile << id << " " << damage << "\n";
#endif
}



void CUnit::Kill(float3& impulse) {
	DamageArray da;
	DoDamage(da * (health / da[armorType]), 0, impulse, -1);
}



void CUnit::UpdateDrawPos() {
	CTransportUnit *trans=GetTransporter();
#if defined(USE_GML) && GML_ENABLE_SIM
	if (trans) {
		drawPos = pos + (trans->speed * ((float)gu->lastFrameStart - (float)lastUnitUpdate) * gu->weightedSpeedFactor);
	} else {
		drawPos = pos + (speed * ((float)gu->lastFrameStart - (float)lastUnitUpdate) * gu->weightedSpeedFactor);
	}
#else
	if (trans) {
		drawPos = pos + (trans->speed * gu->timeOffset);
	} else {
		drawPos = pos + (speed * gu->timeOffset);
	}
#endif
	drawMidPos = drawPos + (midPos - pos);
}

/******************************************************************************/
/******************************************************************************/

CMatrix44f CUnit::GetTransformMatrix(const bool synced, const bool error) const
{
	float3 interPos = synced ? pos : drawPos;

	if (error && !synced && !gu->spectatingFullView) {
		interPos += helper->GetUnitErrorPos(this, gu->myAllyTeam) - midPos;
	}

	CTransportUnit *trans;

	if (usingScriptMoveType ||
	    (!beingBuilt && (physicalState == Flying) && unitDef->canmove)) {
		// aircraft, skidding ground unit, or active ScriptMoveType
		// note: (CAirMoveType) aircraft under construction should not
		// use this matrix, or their nanoframes won't spin on pad
		return CMatrix44f(interPos, -rightdir, updir, frontdir);
	}
	else if ((trans=GetTransporter()) && trans->unitDef->holdSteady) {
		// making local copies of vectors
		float3 frontDir = GetVectorFromHeading(heading);
		float3 upDir    = updir;
		float3 rightDir = frontDir.cross(upDir);
		rightDir.Normalize();
		frontDir = upDir.cross(rightDir);
		return CMatrix44f(interPos, -rightDir, upDir, frontDir);
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
	if (hasRadarCapacity) {
		radarhandler->RemoveUnit(this);
	}

	*valuePtr = newValue;

	if (newValue != 0) {
		hasRadarCapacity = true;
	}
	else if (hasRadarCapacity) {
		hasRadarCapacity = radarRadius || jammerRadius   ||
		                   sonarRadius || sonarJamRadius ||
		                   seismicRadius;
	}

	if (hasRadarCapacity) {
		radarhandler->MoveUnit(this);
	}
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

		new CSeismicGroundFlash(pos + err,
		                             ph->seismictex, 30, 15, 0, pingSize, 1,
		                             float3(0.8f,0.0f,0.0f));
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
	los=0;
	losRadius=l;
	airLosRadius=airlos;
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
	if (hasRadarCapacity) {
		radarhandler->RemoveUnit(this);
	}

	if (unitDef->isAirBase) {
		airBaseHandler->DeregisterAirBase(this);
	}

	// Sharing commander in com ends game kills you.
	// Note that this will kill the com too.
	if (unitDef->isCommander) {
		teamHandler->Team(oldteam)->CommanderDied(this);
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
	if (hasRadarCapacity) {
		radarhandler->MoveUnit(this);
	}

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
		c.params.push_back(1);
		commandAI->GiveCommand(c);
		c.params.clear();
	}
	// reset fire state
	if (commandAI->CanChangeFireState()) {
		c.id = CMD_FIRE_STATE;
		c.params.push_back(2);
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
		if (dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if ((*wi)->AttackUnit(unit, true))
				r = true;
	}
	return r;
}


bool CUnit::AttackGround(const float3 &pos, bool dgun)
{
	bool r = false;
	haveDGunRequest = dgun;
	SetUserTarget(0);
	userAttackPos = pos;
	userAttackGround = true;
	commandShotCount = 0;

	std::vector<CWeapon*>::iterator wi;
	for (wi = weapons.begin(); wi != weapons.end(); ++wi) {
		(*wi)->haveUserTarget = false;
		if (dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if ((*wi)->AttackGround(pos, true))
				r = true;
	}
	return r;
}


void CUnit::SetLastAttacker(CUnit* attacker)
{
	if (teamHandler->AlliedTeams(team, attacker->team)) {
		return;
	}
	if (lastAttacker && lastAttacker != userTarget)
		DeleteDeathDependence(lastAttacker);

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
	if (userTarget && (lastAttacker != userTarget)) {
		DeleteDeathDependence(userTarget);
	}

	userTarget=target;
	for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi) {
		(*wi)->haveUserTarget = false;
	}

	if (target) {
		AddDeathDependence(target);
	}
}


void CUnit::Init(const CUnit* builder)
{
	relMidPos = model->relMidPos;
	midPos = pos + (frontdir * relMidPos.z)
	             + (updir    * relMidPos.y)
	             + (rightdir * relMidPos.x);
	losHeight = relMidPos.y + (radius * 0.5f);
	height = model->height;
	// TODO: This one would be much better to have either in Constructor
	//       or UnitLoader! // why this is always called in unitloader
	currentFuel = unitDef->maxFuel;

	// all ships starts on water, all others on ground.
	// TODO: Improve this. There might be cases when this is not correct.
	if (unitDef->movedata &&
	    (unitDef->movedata->moveType == MoveData::Hover_Move)) {
		physicalState = Hovering;
	} else if (floatOnWater) {
		physicalState = Floating;
	} else {
		physicalState = OnGround;
	}

	// all units are set as ground-blocking by default,
	// units that pretend to be "pseudo-buildings" (ie.
	// hubs, etc) are flagged as immobile so that their
	// positions are not considered valid for building
	blocking = true;
	immobile = (unitDef->speed < 0.001f || !unitDef->canmove);

	// some torp launchers etc are exactly in the surface and should be considered uw anyway
	if ((pos.y + model->height) < 0.0f) {
		isUnderWater = true;
	}

	// semi hack to make mines not block ground
	if (!unitDef->canKamikaze ||
	    (unitDef->type == "Building") ||
	    (unitDef->type == "Factory")) {
		Block();
	}

	UpdateTerrainType();

	Command c;
	if (unitDef->canmove || unitDef->builder) {
		if (unitDef->moveState < 0) {
			if (builder != NULL) {
				moveState = builder->moveState;
			} else {
				moveState = 1;
			}
		} else {
			moveState = unitDef->moveState;
		}

		c.id = CMD_MOVE_STATE;
		c.params.push_back(moveState);
		commandAI->GiveCommand(c);
		c.params.clear();
	}

	if (commandAI->CanChangeFireState()) {
		if (unitDef->fireState < 0) {
			if (builder != NULL && (builder->unitDef->type == "Factory"
						|| dynamic_cast<CFactoryCAI*>(builder->commandAI))) {
				fireState = builder->fireState;
			} else {
				fireState = 2;
			}
		} else {
			fireState = unitDef->fireState;
		}

		c.id = CMD_FIRE_STATE;
		c.params.push_back(fireState);
		commandAI->GiveCommand(c);
		c.params.clear();
	}

	eventHandler.UnitCreated(this, builder);
	eoh->UnitCreated(*this, builder);
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
	if (!script->HasFunction(COBFN_SetSFXOccupy))
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


void CUnit::FinishedBuilding(void)
{
	beingBuilt = false;
	buildProgress = 1.0f;

	if (soloBuilder) {
		DeleteDeathDependence(soloBuilder);
		soloBuilder = NULL;
	}

	if (!(immobile && (mass == 100000))) {
		mass = unitDef->mass;		//set this now so that the unit is harder to move during build
	}

	ChangeLos(realLosRadius, realAirLosRadius);

	const bool oldCloak = isCloaked;
	if (unitDef->startCloaked) {
		wantCloak = true;
		isCloaked = true;
	}

	if (unitDef->windGenerator > 0.0f) {
		// start pointing in direction of wind
		if (wind.GetCurrentStrength() > unitDef->windGenerator) {
			script->SetSpeed(unitDef->windGenerator, 3000.0f);
		} else {
			script->SetSpeed(wind.GetCurrentStrength(), 3000.0f);
		}
		script->SetDirection(GetHeadingFromVectorF(-wind.GetCurrentDirection().x, -wind.GetCurrentDirection().z));
	}

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

	if (oldCloak != isCloaked) {
		eventHandler.UnitCloaked(this); // do this after the UnitFinished call-in
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
	if (unitDef->isCommander) {
		teamHandler->Team(team)->CommanderDied(this);
	}
	teamHandler->Team(this->lineage)->LeftLineage(this);

	if (showDeathSequence && (!reclaimed && !beingBuilt)) {
		string exp;
		if (selfDestruct) {
			exp = unitDef->selfDExplosion;
		} else {
			exp = unitDef->deathExplosion;
		}

		if (!exp.empty()) {
			const WeaponDef* wd = weaponDefHandler->GetWeapon(exp);
			if (wd) {
				helper->Explosion(
					midPos, wd->damages, wd->areaOfEffect, wd->edgeEffectiveness,
					wd->explosionSpeed, this, true, wd->damages[0] > 500 ? 1 : 2,
					false, false, wd->explosionGenerator, 0, ZeroVector, wd->id
				);

				// play explosion sound
				if (wd->soundhit.getID(0) > 0) {
					// HACK: loading code doesn't set sane defaults for explosion sounds, so we do it here
					// NOTE: actually no longer true, loading code always ensures that sound volume != -1
					float volume = wd->soundhit.getVolume(0);
					Channels::Battle.PlaySample(wd->soundhit.getID(0), pos, (volume == -1) ? 1.0f : volume);
				}
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

	if (beingBuilt || dynamic_cast<CAirMoveType*>(moveType) || reclaimed) {
		uh->DeleteUnit(this);
	} else {
		speed = ZeroVector;
		// wait at least 5 more frames before
		// permanently deleting this unit obj
		deathCountdown = 5;
		stunned = true;
		paralyzeDamage = 1000000;
		if (health < 0.0f) {
			health = 0.0f;
		}
	}
}

bool CUnit::AllowedReclaim (CUnit *builder)
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


void CUnit::AddMetal(float metal, bool handicap)
{
	if (metal < 0) {
		UseMetal(-metal);
		return;
	}
	metalMakeI += metal;
	teamHandler->Team(team)->AddMetal(metal, handicap);
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


void CUnit::AddEnergy(float energy, bool handicap)
{
	if (energy < 0) {
		UseEnergy(-energy);
		return;
	}
	energyMakeI += energy;
	teamHandler->Team(team)->AddEnergy(energy, handicap);
}


void CUnit::Activate()
{
	//if(unitDef->tidalGenerator>0)
	//	script->SetSpeed(readmap->tidalStrength * unitDef->tidalGenerator, 3000.0f);

	if (activated)
		return;

	activated = true;
	script->Activate();

	if (unitDef->targfac){
		radarhandler->radarErrorSize[allyteam] /= radarhandler->targFacEffect;
	}
	if (hasRadarCapacity)
		radarhandler->MoveUnit(this);

	int soundIdx = unitDef->sounds.activate.getRandomIdx();
	if (soundIdx >= 0) {
		Channels::UnitReply.PlaySample(
			unitDef->sounds.activate.getID(soundIdx), this,
			unitDef->sounds.activate.getVolume(soundIdx));
	}
}


void CUnit::Deactivate()
{
	if (!activated)
		return;

	activated = false;
	script->Deactivate();

	if (unitDef->targfac){
		radarhandler->radarErrorSize[allyteam] *= radarhandler->targFacEffect;
	}
	if (hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	int soundIdx = unitDef->sounds.deactivate.getRandomIdx();
	if (soundIdx >= 0) {
		Channels::UnitReply.PlaySample(
			unitDef->sounds.deactivate.getID(soundIdx), this,
			unitDef->sounds.deactivate.getVolume(soundIdx));
	}
}


void CUnit::UpdateWind(float x, float z, float strength)
{
	if (strength > unitDef->windGenerator) {
		script->SetSpeed(unitDef->windGenerator, 3000.0f);
	}
	else {
		script->SetSpeed(strength, 3000.0f);
	}

	script->SetDirection(GetHeadingFromVectorF(-x, -z));
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


void CUnit::TempHoldFire(void)
{
	dontFire = true;
	AttackUnit(0, true);
}


void CUnit::ReleaseTempHoldFire(void)
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
	unitDef = unitDefHandler->GetUnitByName(unitDefName);

	yardMap = unitDef->yardmaps[buildFacing];

	model = unitDef->LoadModel();
	SetRadius(model->radius);

	modelParser->CreateLocalModel(this);
	// FIXME: how to handle other script types (e.g. Lua) here?
	script = CUnitScriptFactory::CreateScript(unitDef->scriptPath, this);

	// Call initializing script functions
	script->Create();

	for (vector<CWeapon*>::iterator i = weapons.begin(); i != weapons.end(); ++i) {
		(*i)->weaponDef = unitDef->weapons[(*i)->weaponNum].def;
	}

	script->SetSFXOccupy(curTerrainType);

	if (unitDef->windGenerator>0) {
		if (wind.GetCurrentStrength() > unitDef->windGenerator) {
			script->SetSpeed(unitDef->windGenerator, 3000.0f);
		} else {
			script->SetSpeed(wind.GetCurrentStrength(), 3000.0f);
		}
		script->SetDirection(GetHeadingFromVectorF(-wind.GetCurrentDirection().x, -wind.GetCurrentDirection().z));
	}

	if (activated) {
		script->Activate();
	}
}



void CUnit::DrawS3O()
{
	unitDrawer->DrawUnitS3O(this);
}


void CUnit::LogMessage(const char *fmt, ...)
{
#ifdef DEBUG
	va_list argp;
	int l = strlen(fmt) + unitDefName.size() + 15;
	char tmp[l];
	SNPRINTF(tmp, l, "%s(%d): %s", unitDefName.c_str(), id, fmt);

	va_start(argp, fmt);
	logOutput.Printv(LOG_UNIT, tmp, argp);
	va_end(argp);
#endif
}


void CUnit::StopAttackingAllyTeam(int ally)
{
	commandAI->StopAttackingAllyTeam(ally);
	for (std::vector<CWeapon*>::iterator it = weapons.begin(); it != weapons.end(); ++it) {
		(*it)->StopAttackingAllyTeam(ally);
	}
}


// Member bindings
CR_REG_METADATA(CUnit, (
	//CR_MEMBER(unitDef),
	CR_MEMBER(unitDefName),
	CR_MEMBER(collisionVolume),
	CR_MEMBER(team),
	CR_MEMBER(allyteam),
	CR_MEMBER(lineage),
	CR_MEMBER(aihint),
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
	CR_MEMBER(deathCountdown),
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
	CR_MEMBER(controlRadius),
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
//	CR_MEMBER(drawPos), ??
//	CR_MEMBER(drawMidPos), ??
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
