// Unit.cpp: implementation of the CUnit class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "COB/CobFile.h"
#include "COB/CobInstance.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "creg/STL_List.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/MiniMap.h"
#include "Lua/LuaCallInHandler.h"
#include "Lua/LuaRules.h"
#include "Map/Ground.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/ModInfo.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sync/SyncTracer.h"
#include "System/LoadSaveInterface.h"
#include "System/LogOutput.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Sound.h"
#include "UnitDef.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "UnitLoader.h"
#include "UnitTypes/Building.h"
#include "UnitTypes/TransportUnit.h"
#include "mmgr.h"

#include "COB/CobEngine.h"
#include "CommandAI/AirCAI.h"
#include "CommandAI/BuilderCAI.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "CommandAI/MobileCAI.h"
#include "CommandAI/TransportCAI.h"
#include "UnitDefHandler.h"

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


CUnit::CUnit ()
:	unitDef(0),
	team(0),
	maxHealth(100),
	health(100),
	travel(0.0f),
	travelPeriod(0.0f),
	power(100),
	experience(0),
	limExperience(0),
	neutral(false),
	armorType(0),
	soloBuilder(NULL),
	beingBuilt(true),
	allyteam(0),
	restTime(0),
	controlRadius(2),
	reloadSpeed(1),
	commandAI(0),
	tempNum(0),
	losRadius(0),
	airLosRadius(0),
	losHeight(0),
	metalCost(100),
	energyCost(0),
	buildTime(100),
	frontdir(0,0,1),
	rightdir(-1,0,0),
	updir(0,1,0),
	upright(true),
	falling(false),
	fallSpeed(0.2),
	maxRange(0),
	haveTarget(false),
	lastAttacker(0),
	lastAttack(-200),
	userTarget(0),
	userAttackGround(false),
	commandShotCount(-1),
	lastLosUpdate(0),
	fireState(2),
	moveState(0),
	lastSlowUpdate(0),
	los(0),
	userAttackPos(0,0,0),
	crashing(false),
	cob(0),
	flankingBonusMode(0),
	flankingBonusDir(1.0f, 0.0f, 0.0f),
	flankingBonusAvgDamage(1.4f),
	flankingBonusDifDamage(0.5f),
	flankingBonusMobility(10.0f),
	flankingBonusMobilityAdd(0.01f),
	group(0),
	lastDamage(-100),
	lastFireWeapon(0),
	lastMuzzleFlameSize(0),
	lastMuzzleFlameDir(0,1,0),
	category(0),
	recentDamage(0),
	armoredState(false),
	armoredMultiple(1),
	curArmorMultiple(1),
	buildProgress(0),
	realLosRadius(0),
	realAirLosRadius(0),
	inBuildStance(false),
	isDead(false),
	nextPosErrorUpdate(1),
	posErrorDelta(0,0,0),
	transporter(0),
	toBeTransported(false),
	hasUWWeapons(false),
	lastNanoAdd(gs->frameNum),
	cloakTimeout(128),
	curCloakTimeout(gs->frameNum),
	isCloaked(false),
	wantCloak(false),
	scriptCloak(0),
	decloakDistance(0.0f),
	shieldWeapon(0),
	stockpileWeapon(0),
	haveDGunRequest(false),
	jammerRadius(0),
	sonarJamRadius(0),
	radarRadius(0),
	sonarRadius(0),
	hasRadarCapacity(false),
	stunned(false),
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
	metalStorage(0),
	energyStorage(0),
	lastTerrainType(-1),
	curTerrainType(0),
	relMidPos(0,0,0),
	selfDCountdown(0),
	useHighTrajectory(false),
	deathCountdown(0),
	delayedWreckLevel(-1),
	paralyzeDamage(0),
	captureProgress(0),
	myTrack(0),
	lastFlareDrop(0),
	dontFire(false),
	deathScriptFinished(false),
	dontUseWeapons(false),
	currentFuel(0),
	luaDraw(false),
	noDraw(false),
	noSelect(false),
	noMinimap(false),
	isIcon(false),
	iconRadius(0),
	prevMoveType(NULL),
	usingScriptMoveType(false),
	lodCount(0),
	currentLOD(0),
	alphaThreshold(0.1f),
	cegDamage(1)
{
#ifdef DIRECT_CONTROL_ALLOWED
	directControl = 0;
#endif
	activated = false;
}

CUnit::~CUnit()
{
	if(delayedWreckLevel>=0){
		featureHandler->CreateWreckage(pos,wreckName, heading, buildFacing, delayedWreckLevel,team,-1,true,unitDef->name);
	}

	if(unitDef->isAirBase){
		airBaseHandler->DeregisterAirBase(this);
	}

#ifdef TRACE_SYNC
	tracefile << "Unit died: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif

#ifdef DIRECT_CONTROL_ALLOWED
	if(directControl){
		directControl->myController->StopControllingUnit();
		directControl=0;
	}
#endif

	if(activated && unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]*=radarhandler->targFacEffect;
	}

//	if(!beingBuilt){
	SetMetalStorage(0);
	SetEnergyStorage(0);
//	}

	delete commandAI; commandAI = 0;
	delete moveType; moveType = 0;
	delete prevMoveType; prevMoveType = 0;

	if(group)
		group->RemoveUnit(this);
	group=0;

	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi)
		delete *wi;

	qf->RemoveUnit(this);
	loshandler->DelayedFreeInstance(los);
	los=0;

	if(hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	delete cob;
	delete localmodel;
}


void CUnit::SetMetalStorage(float newStorage)
{
	gs->Team(team)->metalStorage-=metalStorage;
	metalStorage = newStorage;
	gs->Team(team)->metalStorage+=metalStorage;
}


void CUnit::SetEnergyStorage(float newStorage)
{
	gs->Team(team)->energyStorage-=energyStorage;
	energyStorage = newStorage;
	gs->Team(team)->energyStorage+=energyStorage;
}


void CUnit::UnitInit (const UnitDef* def, int Team, const float3& position)
{
	pos = position;
	team = Team;
	allyteam = gs->AllyTeam(Team);
	lineage = Team;
	unitDef = def;
	unitDefName = unitDef->name;

	localmodel=NULL;
	SetRadius(1.2f);
	mapSquare=ground->GetSquare(pos);
	uh->AddUnit(this);
	qf->MovedUnit(this);
	oldRadarPos.x=-1;

	for(int a=0;a<MAX_TEAMS;++a)
		losStatus[a]=0;

	losStatus[allyteam]=LOS_INTEAM | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;

	posErrorVector=gs->randVector();
	posErrorVector.y*=0.2f;
#ifdef TRACE_SYNC
	tracefile << "New unit: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << id << "\n";
#endif
}


void CUnit::ForcedMove(const float3& newPos)
{
	// hack to make mines not block ground
	const bool blocking = !unitDef->canKamikaze || (unitDef->type != "Building");
	if (blocking) {
		UnBlock();
	}

	CBuilding* building = dynamic_cast<CBuilding*>(this);
	if (building && unitDef->useBuildingGroundDecal) {
		groundDecals->RemoveBuilding(building, NULL);
	}

	pos = newPos;
	midPos = pos + (frontdir * relMidPos.z) +
	               (updir    * relMidPos.y) +
	               (rightdir * relMidPos.x);

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
	moveType = SAFE_NEW CScriptMoveType(this);
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
	CMobileCAI* mobile = dynamic_cast<CMobileCAI*>(moveType);
	if (mobile) {
		mobile->lastUserGoal = pos;
	}
}


void CUnit::Update()
{
	posErrorVector += posErrorDelta;

	if (deathCountdown) {
		--deathCountdown;
		if (!deathCountdown) {
			if (deathScriptFinished) {
				uh->DeleteUnit(this);
			} else {
				deathCountdown = 1;
			}
		}
		return;
	}

	if (beingBuilt) {
		return;
	}

	moveType->Update();

	if (travelPeriod != 0.0f) {
		travel += speed.Length();
		travel = fmodf(travel, travelPeriod);
	}

	recentDamage *= 0.9f;
	flankingBonusMobility += flankingBonusMobilityAdd;

	if (stunned) {
		return;
	}

	restTime++;

	if (!dontUseWeapons) {
		std::vector<CWeapon*>::iterator wi;
		for (wi = weapons.begin();wi != weapons.end(); ++wi) {
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

	for (int a = 0; a < gs->activeAllyTeams; ++a) {
		const int prevLosStatus = losStatus[a];
		if (prevLosStatus & LOS_INTEAM) {
			continue; // allied, no need to update
		}
		else if (loshandler->InLos(this, a)) {
			if (!(prevLosStatus & LOS_INLOS)) {

				if (beingBuilt) {
					losStatus[a] |= (LOS_INLOS | LOS_INRADAR);
				} else {
					losStatus[a] |= (LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR);
				}

				if (!(prevLosStatus & LOS_INRADAR)) {
					luaCallIns.UnitEnteredRadar(this, a);
					globalAI->UnitEnteredRadar(this, a);
				}
				luaCallIns.UnitEnteredLos(this, a);
				globalAI->UnitEnteredLos(this, a);
			}
		}
		else if (radarhandler->InRadar(this, a)) {
			if ((prevLosStatus & LOS_INLOS)) {
				luaCallIns.UnitLeftLos(this, a);
				globalAI->UnitLeftLos(this, a);
				losStatus[a] &= ~LOS_INLOS;
			}
			else if (!(prevLosStatus & LOS_INRADAR)) {
				luaCallIns.UnitEnteredRadar(this, a);
				globalAI->UnitEnteredRadar(this, a);
				losStatus[a] |= LOS_INRADAR;
			}
		}
		else {
			if ((prevLosStatus & LOS_INRADAR)) {
				if ((prevLosStatus & LOS_INLOS)) {
					luaCallIns.UnitLeftLos(this, a);
					luaCallIns.UnitLeftRadar(this, a);
					globalAI->UnitLeftLos(this, a);
					globalAI->UnitLeftRadar(this, a);
				} else {
					luaCallIns.UnitLeftRadar(this, a);
					globalAI->UnitLeftRadar(this, a);
				}
				losStatus[a] &= ~(LOS_INLOS | LOS_INRADAR | LOS_CONTRADAR);
			}
		}
	}

	if (paralyzeDamage > 0) {
		paralyzeDamage -= maxHealth * (16.0f / 30.0f / 40.0f);
		if (paralyzeDamage < 0) {
			paralyzeDamage = 0;
		}
	}

	if (stunned) {
		// de-stun only if we are not (still) inside a non-firebase transport
		if (paralyzeDamage < health && !(transporter && !transporter->unitDef->isfireplatform) ) {
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
				luaCallIns.UnitCloaked(this);
			} else {
				luaCallIns.UnitDecloaked(this);
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
		ENTER_MIXED;
		if ((selfDCountdown & 1) && (team == gu->myTeam)) {
			logOutput.Print("%s: Self destruct in %i s",
			                unitDef->humanName.c_str(), selfDCountdown / 2);
		}
		ENTER_SYNCED;
	}

	if (beingBuilt) {
		if (lastNanoAdd < gs->frameNum - 200) {
			const float buildDecay = 1.0f / (buildTime * 0.03f);
			health -= maxHealth * buildDecay;
			buildProgress -= buildDecay;
			AddMetal(metalCost * buildDecay);
			if (health < 0.0f) {
				KillUnit(false, true, NULL);
			}
			UpdateResources();
		}
		return;
	}
	//below is stuff that shouldnt be run while being built

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
		                                         allyteam, unitDef->decloakSpherical)) {
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
			luaCallIns.UnitCloaked(this);
		} else {
			luaCallIns.UnitDecloaked(this);
		}
	}

	if (uh->waterDamage) {
		bool inWater = (pos.y <= -3);
		bool isFloating = (physicalState == CSolidObject::Floating);
		bool onGround = (physicalState == CSolidObject::OnGround);
		bool waterSquare = (readmap->mipHeightmap[1][int((pos.z / (SQUARE_SIZE * 2)) * gs->hmapx + (pos.x / (SQUARE_SIZE * 2)))] < -1);

		// old: "floating or (on ground and height < -3 and mapheight < -1)"
		// new: "height < -3 and (floating or on ground) and mapheight < -1"
		if (inWater && (isFloating || onGround) && waterSquare) {
			DoDamage(DamageArray() * uh->waterDamage, 0, ZeroVector, -1);
		}
	}

	if (unitDef->canKamikaze) {
		if (fireState >= 2) {
			CUnit* u = helper->GetClosestEnemyUnitNoLosTest(pos, unitDef->kamikazeDist, allyteam, false);
			if (u && u->physicalState != CSolidObject::Flying && u->speed.dot(pos - u->pos) <= 0) {
				// self destruct when unit start moving away from mine, should maximize damage
				KillUnit(true, false, NULL);
			}
		}
		if(userTarget && userTarget->pos.distance(pos)<unitDef->kamikazeDist)
			KillUnit(true, false, NULL);
		if(userAttackGround && userAttackPos.distance(pos)<unitDef->kamikazeDist)
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

	if (moveType->progressState == CMoveType::Active) {
		if (seismicSignature) {
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
	std::vector<int> cobargs;

	cobargs.push_back((int)(500 * hitDir.z));
	cobargs.push_back((int)(500 * hitDir.x));

	if (cob->FunctionExist(COBFN_HitByWeaponId)) {
		if (weaponId != -1) {
			cobargs.push_back(weaponDefHandler->weaponDefs[weaponId].tdfId);
		} else {
			cobargs.push_back(-1);
		}
		cobargs.push_back((int)(100 * damage));
		weaponHitMod = 1.0f;
		cob->Call(COBFN_HitByWeaponId, cobargs, hitByWeaponIdCallback, this, NULL);
		damage = damage * weaponHitMod; // weaponHitMod gets set in callback function
	}
	else {
		cob->Call(COBFN_HitByWeapon, cobargs);
	}

	float experienceMod = expMultiplier;

	const int paralyzeTime = damages.paralyzeDamageTime;

	if (paralyzeTime == 0) { // real damage
		if (damage > 0.0f) {
			// Dont log overkill damage (so dguns/nukes etc dont inflate values)
			const float statsdamage = max(0.0f, min(maxHealth - health, damage));
			if (attacker) {
				gs->Team(attacker->team)->currentStats.damageDealt += statsdamage;
			}
			gs->Team(team)->currentStats.damageReceived += statsdamage;
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
			if (paralyzeDamage >= maxParaDmg) {
				experienceMod = 0.0f;
			}
			else {
				if (stunned) {
					experienceMod = 0.0f;
				}
				paralyzeDamage += damage;
				if (paralyzeDamage > health) {
					stunned = true;
				}
				paralyzeDamage = min(paralyzeDamage, maxParaDmg);
			}
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
		if ((attacker != NULL) && !gs->Ally(allyteam, attacker->allyteam)) {
			attacker->AddExperience(0.1f * experienceMod
			                             * (power / attacker->power)
			                             * (damage + min(0.0f, health)) / maxHealth);
			ENTER_UNSYNCED;
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
						sound->PlaySample(
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
			ENTER_SYNCED;
		}
	}

	luaCallIns.UnitDamaged(this, attacker, damage, weaponId, !!damages.paralyzeDamageTime);
	globalAI->UnitDamaged(this, attacker, damage);

	if (health <= 0.0f) {
		KillUnit(false, false, attacker);
		if (isDead && (attacker != 0) &&
		    !gs->Ally(allyteam, attacker->allyteam) && !beingBuilt) {
			attacker->AddExperience(expMultiplier * 0.1f * (power / attacker->power));
			gs->Team(attacker->team)->currentStats.unitsKilled++;
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


/******************************************************************************/
/******************************************************************************/
//
//  Drawing routines
//

inline void CUnit::DrawModel()
{
	if (luaDraw && luaRules && luaRules->DrawUnit(id)) {
		return;
	}

	if (lodCount <= 0) {
		localmodel->Draw();
	} else {
		localmodel->DrawLOD(currentLOD);
	}
}


void CUnit::DrawRawModel()
{
	if (lodCount <= 0) {
		localmodel->Draw();
	} else {
		localmodel->DrawLOD(currentLOD);
	}
}


inline void CUnit::DrawDebug()
{
	// draw the collision sphere
	if (gu->drawdebug) {
		glPushMatrix();
		glTranslatef3((frontdir * relMidPos.z) +
					   (updir    * relMidPos.y) +
					   (rightdir * relMidPos.x));
		GLUquadricObj* q = gluNewQuadric();
		gluQuadricDrawStyle(q, GLU_LINE);
		gluSphere(q, radius, 10, 10);
		gluDeleteQuadric(q);
		glPopMatrix();
	}
}


void CUnit::Draw()
{
	glAlphaFunc(GL_GEQUAL, alphaThreshold);

	glPushMatrix();
	ApplyTransformMatrix();

	if (!beingBuilt || !unitDef->showNanoFrame) {
		DrawModel();
	} else {
		DrawBeingBuilt();
	}

	DrawDebug();
	glPopMatrix();
}


void CUnit::DrawRaw()
{
	glPushMatrix();
	ApplyTransformMatrix();
	DrawModel();
	glPopMatrix();
}


void CUnit::DrawWithLists(unsigned int preList, unsigned int postList)
{
	glPushMatrix();
	ApplyTransformMatrix();

	if (preList != 0) {
		glCallList(preList);
	}

	if (!beingBuilt || !unitDef->showNanoFrame) {
		DrawModel();
	} else {
		DrawBeingBuilt();
	}

	if (postList != 0) {
		glCallList(postList);
	}

	DrawDebug();
	glPopMatrix();
}


void CUnit::DrawRawWithLists(unsigned int preList, unsigned int postList)
{
	glPushMatrix();
	ApplyTransformMatrix();

	if (preList != 0) {
		glCallList(preList);
	}

	DrawModel();

	if (postList != 0) {
		glCallList(postList);
	}

	glPopMatrix();
}


void CUnit::DrawBeingBuilt()
{
	if (shadowHandler->inShadowPass) {
		if (buildProgress > 0.66f) {
			DrawModel();
		}
		return;
	}

	const float start  = model->miny;
	const float height = model->height;

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	const float col = fabs(128.0f - ((gs->frameNum * 4) & 255)) / 255.0f + 0.5f;
	float3 fc;// fc frame color
	if (!gu->teamNanospray) {
		fc = unitDef->nanoColor;
	}
	else {
		const unsigned char* tcol = gs->Team(team)->color;
		fc = float3(tcol[0] * (1.0f / 255.0f),
								tcol[1] * (1.0f / 255.0f),
								tcol[2] * (1.0f / 255.0f));
	}
	glColorf3(fc * col);

	unitDrawer->UnitDrawingTexturesOff(model);

	const double plane0[4] = { 0, -1, 0, start + height * buildProgress * 3 };
	glClipPlane(GL_CLIP_PLANE0, plane0);
	const double plane1[4] = { 0, 1, 0, -start - height * (buildProgress * 10 - 9) };
	glClipPlane(GL_CLIP_PLANE1, plane1);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawModel();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (buildProgress>0.33f) {
		glColorf3(fc * (1.5f - col));
		const double plane0[4] = { 0, -1, 0, start + height * (buildProgress * 3 - 1) };
		glClipPlane(GL_CLIP_PLANE0, plane0);
		const double plane1[4] = { 0, 1, 0, -start - height * (buildProgress * 3 - 2) };
		glClipPlane(GL_CLIP_PLANE1, plane1);

		DrawModel();
	}
	glDisable(GL_CLIP_PLANE1);

	unitDrawer->UnitDrawingTexturesOn(model);

	if (buildProgress > 0.66f){
		const double plane0[4] = { 0, -1, 0 , start + height * (buildProgress * 3 - 2) };
		glClipPlane(GL_CLIP_PLANE0, plane0);
		if (shadowHandler->drawShadows && !water->drawReflection) {
			glPolygonOffset(1.0f, 1.0f);
			glEnable(GL_POLYGON_OFFSET_FILL);
		}
		DrawModel();
		if (shadowHandler->drawShadows && !water->drawReflection) {
			glDisable(GL_POLYGON_OFFSET_FILL);
		}
	}
	glDisable(GL_CLIP_PLANE0);
}


void CUnit::ApplyTransformMatrix() const
{
	CMatrix44f m;
	GetTransformMatrix(m);
	glMultMatrixf(&m[0]);
}


void CUnit::GetTransformMatrix(CMatrix44f& matrix) const
{
	float3 interPos;
	if (!transporter) {
		interPos = pos + (speed * gu->timeOffset);
	} else {
		interPos = pos + (transporter->speed * gu->timeOffset);
	}

	if (!beingBuilt && (usingScriptMoveType || ((physicalState == Flying) && unitDef->canmove))) {
		// aircraft, skidding ground unit, or active ScriptMoveType
		// note: (CAirMoveType) aircraft under construction should not
		// use this matrix, or their nanoframes won't spin on pad
		CMatrix44f transMatrix(interPos, -rightdir, updir, frontdir);
		matrix = transMatrix;
	}
	else if (transporter && transporter->unitDef->holdSteady) {
		// making local copies of vectors
		float3 frontDir = GetVectorFromHeading(heading);
		float3 upDir    = updir;
		float3 rightDir = frontDir.cross(upDir);
		rightDir.Normalize();
		frontDir = upDir.cross(rightDir);
		CMatrix44f transMatrix(interPos, -rightDir, upDir, frontDir);
		matrix = transMatrix;
	}
	else if (upright || !unitDef->canmove) {
		if (heading == 0) {
			matrix.LoadIdentity();
			matrix.Translate(interPos);
		} else {
			// making local copies of vectors
			float3 frontDir = GetVectorFromHeading(heading);
			float3 upDir    = updir;
			float3 rightDir = frontDir.cross(upDir);
			rightDir.Normalize();
			frontDir = upDir.cross(rightDir);
			CMatrix44f transMatrix(interPos, -rightdir, updir, frontdir);
			matrix = transMatrix;
		}
	}
	else {
		// making local copies of vectors
		float3 frontDir = GetVectorFromHeading(heading);
		float3 upDir    = ground->GetSmoothNormal(pos.x, pos.z);
		float3 rightDir = frontDir.cross(upDir);
		rightDir.Normalize();
		frontDir = upDir.cross(rightDir);
		CMatrix44f transMatrix(interPos, -rightDir, upDir, frontDir);
		matrix = transMatrix;
	}
}


void CUnit::DrawStats()
{
	if ((gu->myAllyTeam != allyteam) &&
	    !gu->spectatingFullView && unitDef->hideDamage) {
		return;
	}

	float3 interPos;
	if (!transporter) {
		interPos = pos + (speed * gu->timeOffset);
	} else {
		interPos = pos + (transporter->speed * gu->timeOffset);
	}
	interPos.y += model->height + 5.0f;

	// setup the billboard transformation
	glPushMatrix();
	glTranslatef(interPos.x, interPos.y, interPos.z);
	//glMultMatrixd(camera->billboard);
	glCallList(CCamera::billboardList);

	// black background for healthbar
	glColor3f(0.0f, 0.0f, 0.0f);
	glRectf(-5.0f, 4.0f, +5.0f, 6.0f);

	// healthbar
	const float hpp = max(0.0f, health / maxHealth);
	const float hEnd = hpp * 10.0f;
	if (stunned) {
		glColor3f(0.0f, 0.0f, 1.0f);
	} else {
		if (hpp > 0.5f) {
			glColor3f(1.0f - ((hpp - 0.5f) * 2.0f), 1.0f, 0.0f);
		} else {
			glColor3f(1.0f, hpp * 2.0f, 0.0f);
		}
	}
	glRectf(-5.0f, 4.0f, hEnd - 5.0f, 6.0f);

	// stun level
	if (!stunned && (paralyzeDamage > 0.0f)) {
		const float pEnd = (paralyzeDamage / maxHealth) * 10.0f;
		glColor3f(0.0f, 0.0f, 1.0f);
		glRectf(-5.0f, 4.0f, pEnd - 5.0f, 6.0f);
	}

	// skip the rest of the indicators if it isn't a local unit
	if ((gu->myTeam != team) && !gu->spectatingFullView) {
		glPopMatrix();
		return;
	}

	// experience bar
	const float eEnd = (limExperience * 0.8f) * 10.0f;
	glColor3f(1.0f, 1.0f, 1.0f);
	glRectf(6.0f, -2.0f, 8.0f, eEnd - 2.0f);

	if (beingBuilt) {
		const float bEnd = (buildProgress * 0.8f) * 10.0f;
		glColor3f(1.0f, 0.0f, 0.0f);
		glRectf(-8.0f, -2.0f, -6.0f, bEnd - 2.0f);
	}
	else if (stockpileWeapon) {
		const float sEnd = (stockpileWeapon->buildPercent * 0.8f) * 10.0f;
		glColor3f(1.0f, 0.0f, 0.0f);
		glRectf(-8.0f, -2.0f, -6.0f, sEnd - 2.0f);
	}

	if (group) {
		const float scale = 10.0f;
		char buf[32];
		sprintf(buf, "%i", group->id);
		const float width = scale * font->CalcTextWidth(buf);

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glTranslatef(-7.0f - width, 0.0f, 0.0f); // right justified
		glScalef(scale, scale, scale);
		glColor3f(1.0f, 1.0f, 1.0f);
		font->glPrintSuperRaw(buf);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}


/******************************************************************************/
/******************************************************************************/

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
			luaCallIns.UnitExperience(this, oldExp);
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

	const float* errorScale = radarhandler->radarErrorSize;
	if (!(losStatus[gu->myAllyTeam] & LOS_INLOS) &&
	    radarhandler->InSeismicDistance(this, gu->myAllyTeam)) {
		const float3 err(errorScale[gu->myAllyTeam] * (0.5f - rx), 0.0f,
		                 errorScale[gu->myAllyTeam] * (0.5f - rz));

		SAFE_NEW CSeismicGroundFlash(pos + err,
		                             ph->seismictex, 30, 15, 0, pingSize, 1,
		                             float3(0.8f,0.0f,0.0f));
	}
	for (int a=0; a<gs->activeAllyTeams; ++a) {
		if (radarhandler->InSeismicDistance(this, a)) {
			const float3 err(errorScale[a] * (0.5f - rx), 0.0f,
							 errorScale[a] * (0.5f - rz));
			const float3 pingPos = (pos + err);
			luaCallIns.UnitSeismicPing(this, a, pingPos, pingSize);
			globalAI->SeismicPing(a, this, pingPos, pingSize);
		}
	}
}

void CUnit::ChangeLos(int l,int airlos)
{
	loshandler->FreeInstance(los);
	los=0;
	losRadius=l;
	airLosRadius=airlos;
	loshandler->MoveUnit(this,false);
}


bool CUnit::ChangeTeam(int newteam, ChangeType type)
{
	// do not allow unit count violations due to team swapping
	// (this includes unit captures)
	if (uh->unitsByDefs[newteam][unitDef->id].size() >= unitDef->maxThisUnit) {
		return false;
	}

	const bool capture = (type == ChangeCaptured);
	if (luaRules && !luaRules->AllowUnitTransfer(this, newteam, capture)) {
		return false;
	}

	const int oldteam = team;

	selectedUnits.RemoveUnit(this);
	SetGroup(0);

	luaCallIns.UnitTaken(this, newteam);
	globalAI->UnitTaken(this, oldteam);

	// reset states and clear the queues
	if (!gs->AlliedTeams(oldteam, newteam)) {
		ChangeTeamReset();
	}

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
		gs->Team(oldteam)->CommanderDied(this);
	}

	if (type == ChangeGiven) {
		gs->Team(oldteam)->RemoveUnit(this, CTeam::RemoveGiven);
		gs->Team(newteam)->AddUnit(this,    CTeam::AddGiven);
	} else {
		gs->Team(oldteam)->RemoveUnit(this, CTeam::RemoveCaptured);
		gs->Team(newteam)->AddUnit(this,    CTeam::AddCaptured);
	}

	if (!beingBuilt) {
		gs->Team(oldteam)->metalStorage  -= metalStorage;
		gs->Team(oldteam)->energyStorage -= energyStorage;

		gs->Team(newteam)->metalStorage  += metalStorage;
		gs->Team(newteam)->energyStorage += energyStorage;
	}


	team = newteam;
	allyteam = gs->AllyTeam(newteam);

	uh->unitsByDefs[oldteam][unitDef->id].erase(this);
	uh->unitsByDefs[newteam][unitDef->id].insert(this);

	neutral = false;

	loshandler->MoveUnit(this,false);
	losStatus[allyteam] = LOS_INTEAM | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;
	qf->MovedUnit(this);
	if (hasRadarCapacity) {
		radarhandler->MoveUnit(this);
	}

	model = unitDef->LoadModel(newteam);

	delete localmodel;
	localmodel = modelParser->CreateLocalModel(model, &cob->pieces);
	SetLODCount(0);

	if (unitDef->isAirBase) {
		airBaseHandler->RegisterAirBase(this);
	}

	luaCallIns.UnitGiven(this, oldteam);
	globalAI->UnitGiven(this, oldteam);

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
	if (!unitDef->noAutoFire &&
	    (!unitDef->weapons.empty() || (unitDef->type == "Factory"))) {
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
	for(wi = weapons.begin(); wi != weapons.end(); ++wi){
		(*wi)->haveUserTarget = false;
		(*wi)->targetType = Target_None;
		if(dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if((*wi)->AttackUnit(unit, true))
				r = true;
	}
	return r;
}

bool CUnit::AttackGround(const float3 &pos, bool dgun)
{
	bool r=false;
	haveDGunRequest=dgun;
	SetUserTarget(0);
	userAttackPos=pos;
	userAttackGround=true;
	commandShotCount=0;
	std::vector<CWeapon*>::iterator wi;
	for(wi=weapons.begin();wi!=weapons.end();++wi){
		(*wi)->haveUserTarget=false;
		if(dgun || !unitDef->canDGun || !(*wi)->weaponDef->manualfire)
			if((*wi)->AttackGround(pos,true))
				r=true;
	}
	return r;
}

void CUnit::SetLastAttacker(CUnit* attacker)
{
	if(gs->Ally(team, attacker->team)){
		return;
	}
	if(lastAttacker && lastAttacker!=userTarget)
		DeleteDeathDependence(lastAttacker);

	lastAttack=gs->frameNum;
	lastAttacker=attacker;
	if(attacker)
		AddDeathDependence(attacker);
}

void CUnit::DependentDied(CObject* o)
{
	if (o == userTarget)   { userTarget   = NULL; }
	if (o == soloBuilder)  { soloBuilder  = NULL; }
	if (o == transporter)  { transporter  = NULL; }
	if (o == lastAttacker) { lastAttacker = NULL; }

	incomingMissiles.remove((CMissileProjectile*)o);

	CSolidObject::DependentDied(o);
}

void CUnit::SetUserTarget(CUnit* target)
{
	if(userTarget && lastAttacker!=userTarget)
		DeleteDeathDependence(userTarget);

	userTarget=target;
	for(vector<CWeapon*>::iterator wi=weapons.begin();wi!=weapons.end();++wi)
		(*wi)->haveUserTarget=false;

	if(target){
		AddDeathDependence(target);
	}
}


void CUnit::Init(const CUnit* builder)
{
	relMidPos=model->relMidPos;
	midPos=pos+frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x;
	losHeight=relMidPos.y+radius*0.5f;
	height = model->height;		//TODO: This one would be much better to have either in Constructor or UnitLoader!//why this is always called in unitloader
	currentFuel=unitDef->maxFuel;

	//All ships starts on water, all other on ground.
	//TODO: Improve this. There might be cases when this is not correct.
	if(unitDef->movedata && unitDef->movedata->moveType==MoveData::Hover_Move){
		physicalState = Hovering;
	} else if(floatOnWater) {
		physicalState = Floating;
	} else {
		physicalState = OnGround;
	}

	//All units are set as ground-blocking.
	blocking = true;

	if(pos.y+model->height<1)	//some torp launchers etc is exactly in the surface and should be considered uw anyway
		isUnderWater=true;

	if(!unitDef->canKamikaze || unitDef->type!="Building")	//semi hack to make mines not block ground
		Block();

	UpdateTerrainType();

	luaCallIns.UnitCreated(this, builder);
	globalAI->UnitCreated(this); // FIXME -- add builder?
}

void CUnit::UpdateTerrainType()
{
	if (curTerrainType != lastTerrainType) {
		cob->Call(COBFN_SetSFXOccupy, curTerrainType);
		lastTerrainType = curTerrainType;
	}
}

void CUnit::CalculateTerrainType()
{
	//Optimization: there's only about one unit that actually needs this information
	if (!cob->HasScriptFunction(COBFN_SetSFXOccupy))
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
	if (group != 0) {
		group->RemoveUnit(this);
	}
	group=newGroup;

	if(group){
		if(!group->AddUnit(this)){
			group=0;									//group ai didnt accept us
			return false;
		} else { // add us to selected units if group is selected
			if(selectedUnits.selectedGroup == group->id)
				selectedUnits.AddUnit(this);
		}
	}
	return true;
}


bool CUnit::AddBuildPower(float amount, CUnit* builder)
{
	if (amount > 0.0f) { //  build / repair
		if (!beingBuilt && (health >= maxHealth)) {
			return false;
		}

		lastNanoAdd = gs->frameNum;
		const float part = amount / buildTime;

		if (beingBuilt) {
			const float metalUse  = (metalCost  * part);
			const float energyUse = (energyCost * part);
			if ((gs->Team(builder->team)->metal  >= metalUse) &&
			    (gs->Team(builder->team)->energy >= energyUse) &&
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
				gs->Team(builder->team)->metalPull  += metalUse;
				gs->Team(builder->team)->energyPull += energyUse;
			}
			return false;
		}
		else {
			if (health < maxHealth) {
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
		const float part = amount / buildTime;
		if (luaRules && !luaRules->AllowUnitBuildStep(builder, this, part)) {
			return false;
		}
		restTime = 0;
		health += maxHealth * part;
		if (beingBuilt) {
			builder->AddMetal(metalCost*-part);
			buildProgress+=part;
			if(buildProgress<0 || health<0){
				KillUnit(false, true, NULL);
				return false;
			}
		} else {
			if (health < 0) {
				builder->AddMetal(metalCost);
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
			cob->Call(COBFN_SetSpeed, (int)(unitDef->windGenerator * 3000.0f));
		} else {
			cob->Call(COBFN_SetSpeed, (int)(wind.GetCurrentStrength() * 3000.0f));
		}
		cob->Call(COBFN_SetDirection,
		          (int)GetHeadingFromVector(-wind.GetCurrentDirection().x,
		                                    -wind.GetCurrentDirection().z));
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

	luaCallIns.UnitFinished(this);
	globalAI->UnitFinished(this);

	if (oldCloak != isCloaked) {
		luaCallIns.UnitCloaked(this); // do this after the UnitFinished call-in
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

// Called when a unit's Killed script finishes executing
static void CUnitKilledCB(int retCode, void* p1, void* p2)
{
	CUnit* self = (CUnit *)p1;
	self->deathScriptFinished = true;
	self->delayedWreckLevel = retCode;
}

void CUnit::KillUnit(bool selfDestruct, bool reclaimed, CUnit* attacker)
{
	if(isDead)
		return;
	if(dynamic_cast<CAirMoveType*>(moveType) && !beingBuilt){
		if(!selfDestruct && !reclaimed && gs->randFloat()>recentDamage*0.7f/maxHealth+0.2f){
			((CAirMoveType*)moveType)->SetState(CAirMoveType::AIRCRAFT_CRASHING);
			health=maxHealth*0.5f;
			return;
		}
	}
	isDead=true;

	luaCallIns.UnitDestroyed(this, attacker);
	globalAI->UnitDestroyed(this, attacker);

	blockHeightChanges=false;
	if(unitDef->isCommander)
		gs->Team(team)->CommanderDied(this);
	gs->Team(this->lineage)->LeftLineage(this);

	if (!reclaimed && !beingBuilt) {
		string exp;
		if (selfDestruct)
			exp = unitDef->selfDExplosion;
		else
			exp = unitDef->deathExplosion;

		if (!exp.empty()) {
			const WeaponDef* wd = weaponDefHandler->GetWeapon(exp);
			if (wd) {
				helper->Explosion(
					midPos, wd->damages, wd->areaOfEffect, wd->edgeEffectiveness,
					wd->explosionSpeed, this, true, wd->damages[0] > 500? 1: 2,
					false, wd->explosionGenerator, 0, ZeroVector, wd->id
				);

				// play explosion sound
				if (wd->soundhit.getID(0) > 0) {
					// HACK: loading code doesn't set sane defaults for explosion sounds, so we do it here
					// NOTE: actually no longer true, loading code always ensures that sound volume != -1
					float volume = wd->soundhit.getVolume(0);
					sound->PlaySample(wd->soundhit.getID(0), pos, (volume == -1)? 5.0f: volume);
				}
			}
		}

		if (selfDestruct)
			recentDamage += maxHealth * 2;

		vector<int> args;
		args.push_back((int) (recentDamage / maxHealth * 100));
		args.push_back(0);
		cob->Call(COBFN_Killed, args, &CUnitKilledCB, this, NULL);

		UnBlock();
		delayedWreckLevel = args[1];
//		featureHandler->CreateWreckage(pos,wreckName, heading, args[1],-1,true);
	} else {
		deathScriptFinished=true;
	}
	if(beingBuilt || dynamic_cast<CAirMoveType*>(moveType) || reclaimed)
		uh->DeleteUnit(this);
	else{
		speed=ZeroVector;
		deathCountdown=5;
		stunned=true;
		paralyzeDamage=1000000;
		if(health<0)
			health=0;
	}
}

bool CUnit::UseMetal(float metal)
{
	if(metal<0){
		AddMetal(-metal);
		return true;
	}
	gs->Team(team)->metalPull += metal;
	bool canUse=gs->Team(team)->UseMetal(metal);
	if(canUse)
		metalUseI += metal;
	return canUse;
}

void CUnit::AddMetal(float metal)
{
	if(metal<0){
		UseMetal(-metal);
		return;
	}
	metalMakeI += metal;
	gs->Team(team)->AddMetal(metal);
}

bool CUnit::UseEnergy(float energy)
{
	if(energy<0){
		AddEnergy(-energy);
		return true;
	}
	gs->Team(team)->energyPull += energy;
	bool canUse=gs->Team(team)->UseEnergy(energy);
	if(canUse)
		energyUseI += energy;
	return canUse;
}

void CUnit::AddEnergy(float energy)
{
	if(energy<0){
		UseEnergy(-energy);
		return;
	}
	energyMakeI += energy;
	gs->Team(team)->AddEnergy(energy);
}

void CUnit::Activate()
{
	//if(unitDef->tidalGenerator>0)
	//	cob->Call(COBFN_SetSpeed, (int)(readmap->tidalStrength*3000.0f*unitDef->tidalGenerator));

	if (activated)
		return;

	activated = true;
	cob->Call(COBFN_Activate);

	if (unitDef->targfac){
		radarhandler->radarErrorSize[allyteam] /= radarhandler->targFacEffect;
	}
	if (hasRadarCapacity)
		radarhandler->MoveUnit(this);

	int soundIdx = unitDef->sounds.activate.getRandomIdx();
	if (soundIdx >= 0) {
		sound->PlayUnitActivate(
			unitDef->sounds.activate.getID(soundIdx), this,
			unitDef->sounds.activate.getVolume(soundIdx));
	}
}

void CUnit::Deactivate()
{
	if (!activated)
		return;

	activated = false;
	cob->Call(COBFN_Deactivate);

	if (unitDef->targfac){
		radarhandler->radarErrorSize[allyteam] *= radarhandler->targFacEffect;
	}
	if (hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	int soundIdx = unitDef->sounds.deactivate.getRandomIdx();
	if (soundIdx >= 0) {
		sound->PlayUnitActivate(
			unitDef->sounds.deactivate.getID(soundIdx), this,
			unitDef->sounds.deactivate.getVolume(soundIdx));
	}
}

void CUnit::PushWind(float x, float z, float strength)
{
	if(strength > unitDef->windGenerator)
	{
		cob->Call(COBFN_SetSpeed, (int)(unitDef->windGenerator*3000.0f));
	}
	else
	{
		cob->Call(COBFN_SetSpeed, (int)(strength*3000.0f));
	}

	cob->Call(COBFN_SetDirection, (int)GetHeadingFromVector(-x, -z));
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
			SAFE_NEW CFlareProjectile(pos, speed, this, (int) (gs->frameNum + unitDef->flareDelay * (1 + gs->randFloat()) * 15));
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

void CUnit::DrawS3O(void)
{
	unitDrawer->SetS3OTeamColour(team);
	Draw();
}

void CUnit::hitByWeaponIdCallback(int retCode, void *p1, void *p2)
{
	((CUnit*)p1)->weaponHitMod = retCode*0.01f;
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
	const float lpp = max(0.0f, dist * lodFactor);
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
	const float3& sun = gs->sunVector;
	const float3 diff = (camera->pos - pos);
	const float  dot  = diff.dot(sun);
	const float3 gap  = diff - (sun * dot);
	const float  lpp  = max(0.0f, gap.Length() * lodFactor);

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
	model = unitDef->LoadModel(team);
	SetRadius(model->radius);

	cob = SAFE_NEW CCobInstance(GCobEngine.GetCobFile("scripts/" + unitDef->name+".cob"), this);
	localmodel = modelParser->CreateLocalModel(model, &cob->pieces);

	// Calculate the max() of the available weapon reloadtimes
	int relMax = 0;
	for (vector<CWeapon*>::iterator i = weapons.begin(); i != weapons.end(); ++i) {
		if ((*i)->reloadTime > relMax)
			relMax = (*i)->reloadTime;
		if(dynamic_cast<CBeamLaser*>(*i))
			relMax=150;
	}
	relMax *= 30;		// convert ticks to milliseconds

	// TA does some special handling depending on weapon count
	if (weapons.size() > 1)
		relMax = max(relMax, 3000);

	// Call initializing script functions
	cob->Call(COBFN_Create);
	cob->Call("SetMaxReloadTime", relMax);
	for (vector<CWeapon*>::iterator i = weapons.begin(); i != weapons.end(); ++i) {
		(*i)->weaponDef = unitDef->weapons[(*i)->weaponNum].def;
	}

	cob->Call(COBFN_SetSFXOccupy, curTerrainType);

	if (unitDef->windGenerator>0) {
		if (wind.GetCurrentStrength() > unitDef->windGenerator) {
			cob->Call(COBFN_SetSpeed, (int)(unitDef->windGenerator * 3000.0f));
		} else {
			cob->Call(COBFN_SetSpeed, (int)(wind.GetCurrentStrength()       * 3000.0f));
		}
		cob->Call(COBFN_SetDirection, (int)GetHeadingFromVector(-wind.GetCurrentDirection().x, -wind.GetCurrentDirection().z));
	}

	if (activated) {
		cob->Call(COBFN_Activate);
	}
}

// Member bindings
CR_REG_METADATA(CUnit, (
				//CR_MEMBER(unitDef),
				CR_MEMBER(unitDefName),
				CR_MEMBER(id),
				CR_MEMBER(team),
				CR_MEMBER(allyteam),
				CR_MEMBER(lineage),
				CR_MEMBER(aihint),
				CR_MEMBER(frontdir),
				CR_MEMBER(rightdir),
				CR_MEMBER(updir),
				CR_MEMBER(upright),
				CR_MEMBER(relMidPos),
				CR_MEMBER(power),
				CR_MEMBER(luaDraw),
				CR_MEMBER(noDraw),
				CR_MEMBER(noSelect),
				CR_MEMBER(noMinimap),
				CR_MEMBER(travel),
				CR_MEMBER(travelPeriod),
				CR_RESERVED(8),

				CR_MEMBER(maxHealth),
				CR_MEMBER(health),
				CR_MEMBER(paralyzeDamage),
				CR_MEMBER(captureProgress),
				CR_MEMBER(experience),
				CR_MEMBER(limExperience),
				CR_MEMBER(neutral),
				CR_MEMBER(soloBuilder),
				CR_MEMBER(beingBuilt),
				CR_MEMBER(lastNanoAdd),
				CR_MEMBER(transporter),
				CR_MEMBER(toBeTransported),
				CR_MEMBER(buildProgress),
				CR_MEMBER(realLosRadius),
				CR_MEMBER(realAirLosRadius),
				CR_MEMBER(losStatus),
				CR_MEMBER(inBuildStance),
				CR_MEMBER(stunned),
				CR_MEMBER(useHighTrajectory),
				CR_RESERVED(16),

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
				CR_RESERVED(16),

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
				CR_RESERVED(16),

				CR_MEMBER(radarRadius),
				CR_MEMBER(sonarRadius),
				CR_MEMBER(jammerRadius),
				CR_MEMBER(sonarJamRadius),
				CR_MEMBER(hasRadarCapacity),
				CR_MEMBER(radarSquares),
				CR_MEMBER(oldRadarPos),
				CR_MEMBER(stealth),
				CR_RESERVED(16),

				CR_MEMBER(commandAI),
				CR_MEMBER(moveType),
				CR_MEMBER(prevMoveType),
				CR_MEMBER(usingScriptMoveType),
				CR_MEMBER(group),
				CR_RESERVED(16),

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
				CR_RESERVED(16),

				CR_MEMBER(metalStorage),
				CR_MEMBER(energyStorage),

				CR_MEMBER(lastAttacker),
				CR_MEMBER(lastAttack),
				CR_MEMBER(lastDamage),
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

				CR_RESERVED(32),
				//CR_MEMBER(model),
				//CR_MEMBER(cob),
				//CR_MEMBER(localmodel),

				CR_MEMBER(tooltip),
				CR_MEMBER(crashing),
				CR_MEMBER(isDead),
				CR_MEMBER(falling),
				CR_MEMBER(fallSpeed),

				CR_MEMBER(flankingBonusMode),
				CR_MEMBER(flankingBonusDir),
				CR_MEMBER(flankingBonusAvgDamage),
				CR_MEMBER(flankingBonusDifDamage),
				CR_MEMBER(flankingBonusMobility),
				CR_MEMBER(flankingBonusMobilityAdd),

				CR_MEMBER(armoredState),
				CR_MEMBER(armoredMultiple),
				CR_MEMBER(curArmorMultiple),
				CR_RESERVED(16),

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

				CR_MEMBER(alphaThreshold),

				CR_MEMBER(selfDCountdown),
				CR_RESERVED(16),

				//CR_MEMBER(directControl),
				//CR_MEMBER(myTrack),       //unused
				CR_MEMBER(incomingMissiles),
				CR_MEMBER(lastFlareDrop),
				CR_MEMBER(seismicRadius),
				CR_MEMBER(seismicSignature),
				CR_MEMBER(maxSpeed),
				CR_MEMBER(weaponHitMod),

				CR_RESERVED(128),

				CR_POSTLOAD(PostLoad)
				));






