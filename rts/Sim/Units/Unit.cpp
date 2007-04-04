// Unit.cpp: implementation of the CUnit class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "StdAfx.h"
#include "Unit.h"
#include "UnitDef.h"
#include "UnitHandler.h"
#include "COB/CobFile.h"
#include "COB/CobInstance.h"
#include "CommandAI/CommandAI.h"
#include "CommandAI/FactoryCAI.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "ExternalAI/Group.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/MiniMap.h"
#include "Map/Ground.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GroundFlash.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Lua/LuaCallInHandler.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/ScriptMoveType.h"
#include "Sim/Projectiles/FlareProjectile.h"
#include "Sim/Projectiles/MissileProjectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/myMath.h"
#include "System/LoadSaveInterface.h"
#include "System/LogOutput.h"
#include "System/Matrix44f.h"
#include "System/Sound.h"
#include "Sync/SyncTracer.h"
#include "UnitTypes/Building.h"
#include "UnitTypes/TransportUnit.h"
#include "Game/GameSetup.h"
#include "creg/STL_List.h"
#include "mmgr.h"

CR_BIND_DERIVED(CUnit, CSolidObject, );

// See end of source for member bindings
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnit::CUnit ()
:	unitDef(0),
	team(0),
	maxHealth(100),
	health(100),
	power(100),
	experience(0),
	logExperience(0),
	limExperience(0),
	armorType(0),
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
	bonusShieldDir(1,0,0),
	bonusShieldSaved(10),
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
	stockpileWeapon(0),
	haveDGunRequest(false),
	jammerRadius(0),
	sonarJamRadius(0),
	radarRadius(0),
	sonarRadius(0),
	hasRadarCapacity(false),
	stunned(false),
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
	noSelect(false),
	noMinimap(false),
	isIcon(false),
	iconRadius(0),
	prevMoveType(NULL),
	usingScriptMoveType(false)
{
#ifdef DIRECT_CONTROL_ALLOWED
	directControl=0;
#endif

	activated = false;
}

CUnit::~CUnit()
{
	if(delayedWreckLevel>=0){
		featureHandler->CreateWreckage(pos,wreckName, heading, buildFacing, delayedWreckLevel,team,true,unitDef->name);
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

	if(!beingBuilt){
		gs->Team(team)->metalStorage-=unitDef->metalStorage;
		gs->Team(team)->energyStorage-=unitDef->energyStorage;
	}

	delete commandAI;
	delete moveType;
	delete prevMoveType;

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

void CUnit::UnitInit (UnitDef* def, int Team, const float3& position)
{
	pos = position;
	team = Team;
	allyteam = gs->AllyTeam(Team);
	lineage = Team;
	unitDef = def;

	localmodel=NULL;
	SetRadius(1.2f);
	mapSquare=ground->GetSquare(pos);
	qf->MovedUnit(this);
	id=uh->AddUnit(this);
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
}


void CUnit::Update()
{
	posErrorVector+=posErrorDelta;

	if(deathCountdown){
		--deathCountdown;
		if(!deathCountdown){
			if(deathScriptFinished)
				uh->DeleteUnit(this);
			else
				deathCountdown=1;
		}
		return;
	}

	if(beingBuilt)
		return;

	moveType->Update();

	recentDamage*=0.9f;
	bonusShieldSaved+=0.005f;

	if(stunned)
		return;

	restTime++;

	if(!dontUseWeapons){
		std::vector<CWeapon*>::iterator wi;
		for(wi=weapons.begin();wi!=weapons.end();++wi)
			(*wi)->Update();
	}
}

void CUnit::SlowUpdate()
{
	--nextPosErrorUpdate;
	if(nextPosErrorUpdate==0){
		float3 newPosError(gs->randVector());
		newPosError.y*=0.2f;
		if(posErrorVector.dot(newPosError)<0)
			newPosError=-newPosError;
		posErrorDelta=(newPosError-posErrorVector)*(1.0f/256);
		nextPosErrorUpdate=16;
	}

	for (int a = 0; a < gs->activeAllyTeams; ++a) {
		if (losStatus[a] & LOS_INTEAM) {
			continue; // allied, no need to update
		}
		else if (loshandler->InLos(this, a)) {
			if (!(losStatus[a] & LOS_INLOS)) {
				int prevLosStatus = losStatus[a];

				if (beingBuilt) {
					losStatus[a] |= (LOS_INLOS | LOS_INRADAR);
				} else {
					losStatus[a] |= (LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR);
				}

				if(!(prevLosStatus&LOS_INRADAR)){
					luaCallIns.UnitEnteredRadar(this, a);
					globalAI->UnitEnteredRadar(this, a);
				}
				luaCallIns.UnitEnteredLos(this, a);
				globalAI->UnitEnteredLos(this, a);
			}
		} else if (radarhandler->InRadar(this, a)) {
			if ((losStatus[a] & LOS_INLOS)) {
				luaCallIns.UnitLeftLos(this, a);
				globalAI->UnitLeftLos(this, a);
				losStatus[a] &= ~LOS_INLOS;
			} else if (!(losStatus[a] & LOS_INRADAR)) {
				losStatus[a] |= LOS_INRADAR;
				luaCallIns.UnitEnteredRadar(this, a);
				globalAI->UnitEnteredRadar(this, a);
			}
		} else {
			if ((losStatus[a] & LOS_INRADAR)) {
				if ((losStatus[a] & LOS_INLOS)) {
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

	if(paralyzeDamage>0){
		paralyzeDamage-=maxHealth*(16.f/30.f/40.f);
		if(paralyzeDamage<0)
			paralyzeDamage=0;
		if(paralyzeDamage<health)
			stunned=false;
	}
	if(stunned){
		isCloaked=false;
		return;
	}

	if(selfDCountdown && !stunned){
		selfDCountdown--;
		if(selfDCountdown<=1){
			if(!beingBuilt)
				KillUnit(true,false,0);
			else
				KillUnit(false,true,0);	//avoid unfinished buildings making an explosion
			selfDCountdown=0;
			return;
		}
		ENTER_MIXED;
		if(selfDCountdown&1 && team==gu->myTeam)
			logOutput.Print("%s: Self destruct in %i s",unitDef->humanName.c_str(),selfDCountdown/2);
		ENTER_SYNCED;
	}

	if(beingBuilt){
		if(lastNanoAdd<gs->frameNum-200){
			health-=maxHealth/(buildTime*0.03f);
			buildProgress-=1/(buildTime*0.03f);
			AddMetal(metalCost/(buildTime*0.03f));
			if(health<0)
				KillUnit(false,true,0);
		}
		return;
	}
	//below is stuff that shouldnt be run while being built

	lastSlowUpdate=gs->frameNum;

	commandAI->SlowUpdate();
	moveType->SlowUpdate();

	metalMake = metalMakeI + metalMakeold;
	metalUse = metalUseI+ metalUseold;
	energyMake = energyMakeI + energyMakeold;
	energyUse = energyUseI + energyUseold;
	metalMakeold = metalMakeI;
	metalUseold = metalUseI;
	energyMakeold = energyMakeI;
	energyUseold = energyUseI;

	metalMakeI=metalUseI=energyMakeI=energyUseI=0;

	AddMetal(unitDef->metalMake*0.5f);
	if(activated)
	{
		if(UseEnergy(unitDef->energyUpkeep*0.5f))
		{
			if(unitDef->isMetalMaker){
				AddMetal(unitDef->makesMetal*0.5f*uh->metalMakerEfficiency);
				uh->metalMakerIncome+=unitDef->makesMetal;
			} else {
				AddMetal(unitDef->makesMetal*0.5f);
			}
			if(unitDef->extractsMetal>0)
				AddMetal(metalExtract * 0.5f);
		}
		UseMetal(unitDef->metalUpkeep*0.5f);

		if(unitDef->windGenerator>0)
		{
			if(wind.curStrength > unitDef->windGenerator)
			{
 				AddEnergy(unitDef->windGenerator*0.5f);
			}
			else
			{
 				AddEnergy(wind.curStrength*0.5f);
			}
		}
	}
	AddEnergy(energyTickMake*0.5f);

	if(health<maxHealth)
	{
		health += unitDef->autoHeal;

		if(restTime > unitDef->idleTime)
		{
			health += unitDef->idleAutoHeal;
		}
		if(health>maxHealth)
			health=maxHealth;
	}

	bonusShieldSaved+=0.05f;
	residualImpulse*=0.6f;

	if(wantCloak){
		if(helper->GetClosestEnemyUnitNoLosTest(pos,unitDef->decloakDistance,allyteam)){
			curCloakTimeout=gs->frameNum+cloakTimeout;
			isCloaked=false;
		}
		if(isCloaked || gs->frameNum>=curCloakTimeout){
			float cloakCost=unitDef->cloakCost;
			if(speed.SqLength()>0.2f)
				cloakCost=unitDef->cloakCostMoving;
			if(UseEnergy(cloakCost * 0.5f)){
				isCloaked=true;
			} else {
				isCloaked=false;
			}
		} else {
			isCloaked=false;
		}
	} else {
		isCloaked=false;
	}


	if(uh->waterDamage && (physicalState==CSolidObject::Floating || (physicalState==CSolidObject::OnGround && pos.y<=-3 && readmap->mipHeightmap[1][int((pos.z/(SQUARE_SIZE*2))*gs->hmapx+(pos.x/(SQUARE_SIZE*2)))]<-1))){
		DoDamage(DamageArray()*uh->waterDamage,0,ZeroVector, -1);
	}

	if(unitDef->canKamikaze){
		if(fireState==2){
			CUnit* u=helper->GetClosestEnemyUnitNoLosTest(pos,unitDef->kamikazeDist,allyteam);
			if(u && u->physicalState!=CSolidObject::Flying && u->speed.dot(pos - u->pos)<=0)		//self destruct when unit start moving away from mine, should maximize damage
				KillUnit(true,false,0);
		}
		if(userTarget && userTarget->pos.distance(pos)<unitDef->kamikazeDist)
			KillUnit(true,false,0);
		if(userAttackGround && userAttackPos.distance(pos)<unitDef->kamikazeDist)
			KillUnit(true,false,0);
	}

	if(!weapons.empty()){
		haveTarget=false;
		haveUserTarget=false;

		// aircraft and ScriptMoveType do not want this
		if (moveType->useHeading) {
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
			float rx = gs->randFloat();
			float rz = gs->randFloat();

			const float* errorScale = radarhandler->radarErrorSize;
			if (!(losStatus[gu->myAllyTeam] & LOS_INLOS) &&
			    radarhandler->InSeismicDistance(this, gu->myAllyTeam)) {
				const float3 err(errorScale[gu->myAllyTeam] * (0.5f - rx), 0.0f,
				                 errorScale[gu->myAllyTeam] * (0.5f - rz));

				SAFE_NEW CSeismicGroundFlash(pos + err,
                                     ph->seismictex, 30, 15, 0, seismicSignature, 1,
                                     float3(0.8f,0.0f,0.0f));
			}
			for (int a=0; a<gs->activeAllyTeams; ++a) {
				if (radarhandler->InSeismicDistance(this, a)) {
					const float3 err(errorScale[a] * (0.5f - rx), 0.0f,
					                 errorScale[a] * (0.5f - rz));
					const float3 pingPos = (pos + err);
					luaCallIns.UnitSeismicPing(this, a, pingPos, seismicSignature);
					globalAI->SeismicPing(a, this, pingPos, seismicSignature);
				}
			}
		}
	}

	CalculateTerrainType();
	UpdateTerrainType();
}

void CUnit::DoDamage(const DamageArray& damages, CUnit *attacker,const float3& impulse, int weaponId)
{
	if(isDead)
		return;

	residualImpulse+=impulse/mass;
	moveType->ImpulseAdded();

	float damage=damages[armorType];

	if(damage<0){
//		logOutput.Print("Negative damage");
		return;
	}

	if(attacker){
		SetLastAttacker(attacker);
		float3 adir=attacker->pos-pos;
		adir.Normalize();
		bonusShieldDir+=adir*bonusShieldSaved;		//not the best way to do it(but fast)
		bonusShieldDir.Normalize();
//		logOutput.Print("shield mult %f %f %i %i", 2-adir.dot(bonusShieldDir),bonusShieldSaved,id,attacker->id);
		bonusShieldSaved=0;
		damage*=1.4f-adir.dot(bonusShieldDir)*0.5f;
	}

	damage*=curArmorMultiple;

	float3 hitDir = impulse;
	hitDir.y = 0;
	hitDir = -hitDir.Normalize();
	std::vector<int> cobargs;

	cobargs.push_back((int)(500 * hitDir.z));
	cobargs.push_back((int)(500 * hitDir.x));

	if(cob->FunctionExist(COBFN_HitByWeaponId))
	{
		if(weaponId!=-1)
			cobargs.push_back(weaponDefHandler->weaponDefs[weaponId].tdfId);
		else
			cobargs.push_back(-1);

		cobargs.push_back((int)(100 * damage));
		weaponHitMod = 1.0f;
		cob->Call(COBFN_HitByWeaponId, cobargs, hitByWeaponIdCallback, this, NULL);
		damage = damage * weaponHitMod;//weaponHitMod gets set in callback function
	}
	else
	{
		cob->Call(COBFN_HitByWeapon, cobargs);
	}

	restTime=0;

	float experienceMod=1;

	if(damages.paralyzeDamageTime){
		paralyzeDamage+=damage;
		experienceMod=0.1f;		//reduce experience for paralyzers
		if(health-paralyzeDamage<0){
			if(stunned)
				experienceMod=0;	//dont get any experience for paralyzing paralyzed enemy
			stunned=true;
			if(paralyzeDamage>health+(maxHealth*0.025f*damages.paralyzeDamageTime)){
				paralyzeDamage=health+(maxHealth*0.025f*damages.paralyzeDamageTime);
			}
		}
	} else {
		// Dont log overkill damage (so dguns/nukes etc dont inflate values)
		float statsdamage = std::min(health, damage);
		if (attacker)
			gs->Team(attacker->team)->currentStats.damageDealt += statsdamage;
		gs->Team(team)->currentStats.damageReceived += statsdamage;
		health-=damage;
	}
	recentDamage+=damage;

	if(attacker!=0 && !gs->Ally(allyteam,attacker->allyteam)){
//		logOutput.Print("%s has %f/%f h attacker %s do %f d",tooltip.c_str(),health,maxHealth,attacker->tooltip.c_str(),damage);
		attacker->experience+=0.1f*power/attacker->power*(damage+min(0.f,health))/maxHealth*experienceMod;
		attacker->ExperienceChange();
		ENTER_UNSYNCED;
		if (((!unitDef->isCommander && uh->lastDamageWarning+100<gs->frameNum) ||
		     (unitDef->isCommander && uh->lastCmdDamageWarning+100<gs->frameNum))
		    && (team == gu->myTeam) && !camera->InView(midPos,radius+50) && !gu->spectatingFullView) {
			logOutput.Print("%s is being attacked",unitDef->humanName.c_str());
			logOutput.SetLastMsgPos(pos);
			if (unitDef->isCommander || uh->lastDamageWarning+150<gs->frameNum) {
				sound->PlaySample(unitDef->sounds.underattack.id,unitDef->isCommander?4:2);
			}
			minimap->AddNotification(pos,float3(1,0.3f,0.3f),unitDef->isCommander?1:0.5f);	//todo: make compatible with new gui

			uh->lastDamageWarning=gs->frameNum;
			if(unitDef->isCommander)
				uh->lastCmdDamageWarning=gs->frameNum;
		}
		ENTER_SYNCED;
	}

	luaCallIns.UnitDamaged(this, attacker, damage);
	globalAI->UnitDamaged(this, attacker, damage);

	if(health<=0){
		KillUnit(false,false,attacker);
		if(isDead && attacker!=0 && !gs->Ally(allyteam,attacker->allyteam) && !beingBuilt){
			attacker->experience+=0.1f*power/attacker->power;
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
	DoDamage(da*(health/da[armorType]), 0, impulse, -1);
}


void CUnit::Draw()
{
	glPushMatrix();

	const float3 interPos = pos + (speed * gu->timeOffset);

	if (usingScriptMoveType ||
	    (physicalState == Flying && unitDef->canmove)) {
		// aircraft, skidding ground unit, or active ScriptMoveType
		CMatrix44f transMatrix(interPos, -rightdir, updir, frontdir);
		glMultMatrixf(&transMatrix[0]);
	}
	else if (transporter && transporter->unitDef->holdSteady){

		float3 frontDir=GetVectorFromHeading(heading);		//making local copies of vectors
		float3 upDir=updir;
		float3 rightDir=frontDir.cross(upDir);
		rightDir.Normalize();
		frontDir=upDir.cross(rightDir);

		CMatrix44f transMatrix(interPos,-rightDir,upDir,frontDir);

		glMultMatrixf(&transMatrix[0]);
	}
	else if(upright || !unitDef->canmove){
		glTranslatef3(interPos);
		if(heading!=0)
			glRotatef(heading*(180.0f/32768.0f),0,1,0);
	}
	else {
		float3 frontDir=GetVectorFromHeading(heading);		//making local copies of vectors
		float3 upDir=ground->GetSmoothNormal(pos.x,pos.z);
		float3 rightDir=frontDir.cross(upDir);
		rightDir.Normalize();
		frontDir=upDir.cross(rightDir);

		CMatrix44f transMatrix(interPos,-rightDir,upDir,frontDir);

		glMultMatrixf(&transMatrix[0]);
	}

	if (beingBuilt && unitDef->showNanoFrame) {
		if (shadowHandler->inShadowPass) {
			if (buildProgress>0.66f) {
				localmodel->Draw();
			}
		} else {
			float height=model->height;
			float start=model->miny;
			glEnable(GL_CLIP_PLANE0);
			glEnable(GL_CLIP_PLANE1);
			float col=fabs(128.0f-((gs->frameNum*4)&255))/255.0f+0.5f;
			float3 fc;// fc frame color
			if(gu->teamNanospray){
				unsigned char* tcol=gs->Team(team)->color;
				fc = float3(tcol[0]*(1.f/255.f),tcol[1]*(1.f/255.f),tcol[2]*(1.f/255.f));
			}else{
				fc = unitDef->nanoColor;
			}
			glColorf3(fc*col);

			unitDrawer->UnitDrawingTexturesOff(model);

			double plane[4]={0,-1,0,start+height*buildProgress*3};
			glClipPlane(GL_CLIP_PLANE0 ,plane);
			double plane2[4]={0,1,0,-start-height*(buildProgress*10-9)};
			glClipPlane(GL_CLIP_PLANE1 ,plane2);
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			localmodel->Draw();
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

			if(buildProgress>0.33f){
				glColorf3(fc*(1.5f-col));
				double plane[4]={0,-1,0,start+height*(buildProgress*3-1)};
				glClipPlane(GL_CLIP_PLANE0 ,plane);
				double plane2[4]={0,1,0,-start-height*(buildProgress*3-2)};
				glClipPlane(GL_CLIP_PLANE1 ,plane2);

				localmodel->Draw();
			}
			glDisable(GL_CLIP_PLANE1);
			unitDrawer->UnitDrawingTexturesOn(model);

			if(buildProgress>0.66f){
				double plane[4]={0,-1,0,start+height*(buildProgress*3-2)};
				glClipPlane(GL_CLIP_PLANE0 ,plane);
				if(shadowHandler->drawShadows && !water->drawReflection){
					glPolygonOffset(1,1);
					glEnable(GL_POLYGON_OFFSET_FILL);
				}
				localmodel->Draw();
				if(shadowHandler->drawShadows && !water->drawReflection){
					glDisable(GL_POLYGON_OFFSET_FILL);
				}
			}
			glDisable(GL_CLIP_PLANE0);
		}
	} else {
		localmodel->Draw();
	}

	if(gu->drawdebug){
		glPushMatrix();
		glTranslatef3(frontdir*relMidPos.z + updir*relMidPos.y + rightdir*relMidPos.x);
		GLUquadricObj* q=gluNewQuadric();
		gluQuadricDrawStyle(q,GLU_LINE);
		gluSphere(q,radius,10,10);
		gluDeleteQuadric(q);
		glPopMatrix();
	}/**/
	glPopMatrix();
}

void CUnit::DrawStats()
{
	if ((gu->myAllyTeam != allyteam) &&
	    !gu->spectatingFullView && unitDef->hideDamage) {
		return;
	}

	float3 interPos = pos + (speed * gu->timeOffset);
	interPos.y += model->height + 5.0f;

	glBegin(GL_QUADS);

	const float3& camUp    = camera->up;
	const float3& camRight = camera->right;

	const float hpp = max(0.0f, health / maxHealth);
	const float end = (0.5f - hpp) * 10.0f;

	// black background for healthbar
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertexf3(interPos + (camUp * 6.0f) - (camRight * end));
	glVertexf3(interPos + (camUp * 6.0f) + (camRight * 5.0f));
	glVertexf3(interPos + (camUp * 4.0f) + (camRight * 5.0f));
	glVertexf3(interPos + (camUp * 4.0f) - (camRight * end));

	if (stunned) {
		glColor3f(0.0f, 0.0f, 1.0f);
	} else {
		if (hpp > 0.5f) {
			glColor3f(1.0f - ((hpp - 0.5f) * 2.0f), 1.0f, 0.0f);
		} else {
			glColor3f(1.0f, hpp * 2.0f, 0.0f);
		}
	}
	// healthbar
	glVertexf3(interPos + (camUp * 6.0f) - (camRight * 5.0f));
	glVertexf3(interPos + (camUp * 6.0f) - (camRight * end));
	glVertexf3(interPos + (camUp * 4.0f) - (camRight * end));
	glVertexf3(interPos + (camUp * 4.0f) - (camRight * 5.0f));

	if ((gu->myTeam != team) && !gu->spectatingFullView) {
		glEnd();
		return;
	}

	// experience bar
	glColor3f(1.0f, 1.0f, 1.0f);
	const float hEnd = (limExperience * 0.8f) * 10.0f;
	glVertexf3(interPos + (-camUp * 2.0f)         + (camRight * 6.0f));
	glVertexf3(interPos + (-camUp * 2.0f)         + (camRight * 8.0f));
	glVertexf3(interPos + (camUp * (hEnd - 2.0f)) + (camRight * 8.0f));
	glVertexf3(interPos + (camUp * (hEnd - 2.0f)) + (camRight * 6.0f));
	glEnd();

	if (group) {
		glPushMatrix();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glTranslatef3(interPos - (camRight * 10.0f));
		glScalef(10.0f, 10.0f, 10.0f);
		font->glWorldPrint("%d", group->id);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

	if (beingBuilt) {
		glColor3f(1.0f, 0.0f, 0.0f);
		const float end = (buildProgress * 0.8f) * 10.0f;
		glBegin(GL_QUADS);
		glVertexf3(interPos - (camUp * 2.0f)         - (camRight * 6.0f));
		glVertexf3(interPos - (camUp * 2.0f)         - (camRight * 8.0f));
		glVertexf3(interPos + (camUp * (end - 2.0f)) - (camRight * 8.0f));
		glVertexf3(interPos + (camUp * (end - 2.0f)) - (camRight * 6.0f));
		glEnd();
	}
	else if (stockpileWeapon) {
		glColor3f(1.0f, 0.0f, 0.0f);
		const float end = (stockpileWeapon->buildPercent * 0.8f) * 10.0f;
		glBegin(GL_QUADS);
		glVertexf3(interPos - (camUp * 2.0f)         - (camRight * 6.0f));
		glVertexf3(interPos - (camUp * 2.0f)         - (camRight * 8.0f));
		glVertexf3(interPos + (camUp * (end - 2.0f)) - (camRight * 8.0f));
		glVertexf3(interPos + (camUp * (end - 2.0f)) - (camRight * 6.0f));
		glEnd();
	}
}

void CUnit::ExperienceChange()
{
	logExperience=log(experience+1);
	limExperience=1-(1/(experience+1));

	power=unitDef->power*(1+limExperience);
	reloadSpeed=(1+limExperience*0.4f);

	float oldMaxHealth=maxHealth;
	maxHealth=unitDef->health*(1+limExperience*0.7f);
	health+=(maxHealth-oldMaxHealth)*(health/oldMaxHealth);
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
	if (uh->unitsType[newteam][unitDef->id] >= unitDef->maxThisUnit) {
		return false;
	}

	const bool capture = (type == ChangeGiven);
	if (luaRules && !luaRules->AllowUnitTransfer(this, newteam, capture)) {
		return false;
	}

	const int oldteam = team;

	selectedUnits.RemoveUnit(this);

	luaCallIns.UnitTaken(this, newteam);
	globalAI->UnitTaken(this, oldteam);

	// reset states and clear the queues
	if (!gs->AlliedTeams(oldteam, newteam)) {
		ChangeTeamReset();
	}

	if (uh->unitsType[oldteam][unitDef->id] > 0) {
		uh->unitsType[oldteam][unitDef->id]--;
	} else {
		logOutput.Print("CUnit::ChangeTeam() unitsType underflow\n");
	}
	uh->unitsType[newteam][unitDef->id]++;

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
		gs->Team(oldteam)->metalStorage  -= unitDef->metalStorage;
		gs->Team(oldteam)->energyStorage -= unitDef->energyStorage;

		gs->Team(newteam)->metalStorage  += unitDef->metalStorage;
		gs->Team(newteam)->energyStorage += unitDef->energyStorage;
	}

	team = newteam;
	allyteam = gs->AllyTeam(newteam);

	loshandler->MoveUnit(this,false);
	losStatus[allyteam] = LOS_INTEAM | LOS_INLOS | LOS_INRADAR | LOS_PREVLOS | LOS_CONTRADAR;
	qf->MovedUnit(this);
	if (hasRadarCapacity) {
		radarhandler->MoveUnit(this);
	}

	//model=unitModelLoader->GetModel(model->name,newteam);
	if (unitDef->useCSOffset) {
		model = modelParser->Load3DO(model->name.c_str(),
				unitDef->collisionSphereScale,newteam,
				unitDef->collisionSphereOffset);
	}
	else {
		model = modelParser->Load3DO(model->name.c_str(),
				unitDef->collisionSphereScale,newteam);
	}
//	model = modelParser->Load3DO(model->name.c_str(), 1, newteam);
	delete localmodel;
	localmodel = modelParser->CreateLocalModel(model, &cob->pieces);

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
	if(o==transporter)
		transporter=0;
	if(o==lastAttacker){
//		logOutput.Print("Last attacker died %i %i",lastAttacker->id,id);
		lastAttacker=0;
	}
	if(o==userTarget)
		userTarget=0;

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
	if(group!=0){
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

bool CUnit::AddBuildPower(float amount,CUnit* builder)
{
	if(amount>0){	//build/repair
		if(!beingBuilt && health>=maxHealth)
			return false;

		lastNanoAdd=gs->frameNum;
		float part=amount/buildTime;

		if(beingBuilt){
			float metalUse=metalCost*part;
			float energyUse=energyCost*part;
			if (gs->Team(builder->team)->metal >= metalUse && gs->Team(builder->team)->energy >= energyUse) {
				builder->UseMetal(metalUse);
				builder->UseEnergy(energyUse);
				health+=maxHealth*part;
				buildProgress+=part;
				if(buildProgress>=1){
					if(health>maxHealth)
						health=maxHealth;
					FinishedBuilding();
				}
				return true;
			} else {
				// update the energy and metal required counts
				gs->Team(builder->team)->energyPull += energyUse;
				gs->Team(builder->team)->metalPull += metalUse;
			}
			return false;
		} else {
			if(health<maxHealth){
				health+=maxHealth*part;
				if(health>maxHealth)
					health=maxHealth;
				return true;
			}
			return false;
		}
	} else {	//reclaim
		if(isDead)
			return false;
		restTime=0;
		float part=amount/buildTime;
		health+=maxHealth*part;
		if(beingBuilt){
			builder->AddMetal(metalCost*-part);
			buildProgress+=part;
			if(buildProgress<0 || health<0){
				KillUnit(false,true,0);
				return false;
			}
		} else {
			if(health<0){
				builder->AddMetal(metalCost);
				KillUnit(false,true,0);
				return false;
			}
		}
		return true;
	}
	return false;
}

void CUnit::FinishedBuilding(void)
{
	beingBuilt=false;
	buildProgress=1;

	if(!(mass==100000 && immobile))
		mass=unitDef->mass;		//set this now so that the unit is harder to move during build

	ChangeLos(realLosRadius,realAirLosRadius);

	if(unitDef->startCloaked)
		wantCloak=true;

	if(unitDef->windGenerator>0)
	{
		if(wind.curStrength > unitDef->windGenerator)
		{
			cob->Call(COBFN_SetSpeed, (int)(unitDef->windGenerator*3000.0f));
		}
		else
		{
			cob->Call(COBFN_SetSpeed, (int)(wind.curStrength*3000.0f));
		}
		cob->Call(COBFN_SetDirection, (int)GetHeadingFromVector(-wind.curDir.x, -wind.curDir.z));
	}

	if(unitDef->activateWhenBuilt)
	{
		Activate();
	}

	gs->Team(team)->metalStorage+=unitDef->metalStorage;
	gs->Team(team)->energyStorage+=unitDef->energyStorage;

	//Sets the frontdir in sync with heading.
	frontdir = GetVectorFromHeading(heading) + float3(0,frontdir.y,0);

	if(unitDef->isAirBase){
		airBaseHandler->RegisterAirBase(this);
	}

	luaCallIns.UnitFinished(this);
	globalAI->UnitFinished(this);

	if(unitDef->isFeature){
		UnBlock();
		CFeature* f=featureHandler->CreateWreckage(pos,wreckName, heading, buildFacing, 0,team,false,"");
		if(f){
			f->blockHeightChanges=true;
		}
		KillUnit(false,true,0);
	}
}

// Called when a unit's Killed script finishes executing
static void CUnitKilledCB(int retCode, void* p1, void* p2)
{
	CUnit* self = (CUnit *)p1;
	self->deathScriptFinished = true;
	self->delayedWreckLevel = retCode;
}

void CUnit::KillUnit(bool selfDestruct,bool reclaimed,CUnit *attacker)
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

	if(!reclaimed && !beingBuilt){
		string exp;
		if(selfDestruct)
			exp=unitDef->selfDExplosion;
		else
			exp=unitDef->deathExplosion;

		if(!exp.empty()){
			WeaponDef* wd=weaponDefHandler->GetWeapon(exp);
			if(wd){
				helper->Explosion(midPos,wd->damages,wd->areaOfEffect,wd->edgeEffectiveness,wd->explosionSpeed,this,true,wd->damages[0]>500?1:2,false,wd->explosionGenerator,0, ZeroVector, wd->id);

				// Play explosion sound
				CWeaponDefHandler::LoadSound(wd->soundhit);
				if (wd->soundhit.id) {

					// HACK loading code doesn't set sane defaults for explosion sounds, so we do it here
					if (wd->soundhit.volume == -1)
						wd->soundhit.volume = 5.0f;

					sound->PlaySample(wd->soundhit.id, pos, wd->soundhit.volume);
				}

				//logOutput.Print("Should play %s (%d)", wd->soundhit.name.c_str(), wd->soundhit.id);
			}
		}
		if(selfDestruct)
			recentDamage+=maxHealth*2;

		vector<int> args;
		args.push_back((int)(recentDamage/maxHealth*100));
		args.push_back(0);
		cob->Call(COBFN_Killed, args, &CUnitKilledCB, this, NULL);

		UnBlock();
		delayedWreckLevel=args[1];
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

	if(activated)
		return;
	activated = true;

	cob->Call(COBFN_Activate);

	if(unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]/=radarhandler->targFacEffect;
	}
	if(hasRadarCapacity)
		radarhandler->MoveUnit(this);

	if(unitDef->sounds.activate.id)
		sound->PlayUnitActivate(unitDef->sounds.activate.id, this, unitDef->sounds.activate.volume);
}

void CUnit::Deactivate()
{
	if(!activated)
		return;
	activated = false;

	cob->Call(COBFN_Deactivate);

	if(unitDef->targfac){
		radarhandler->radarErrorSize[allyteam]*=radarhandler->targFacEffect;
	}
	if(hasRadarCapacity)
		radarhandler->RemoveUnit(this);

	if(unitDef->sounds.deactivate.id)
		sound->PlayUnitActivate(unitDef->sounds.deactivate.id, this, unitDef->sounds.deactivate.volume);
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
	if(loading)
		ExperienceChange();
	file->lsInt(moveState);
	file->lsInt(fireState);
	file->lsBool(isCloaked);
	file->lsBool(wantCloak);
	commandAI->LoadSave(file,loading);
}

void CUnit::IncomingMissile(CMissileProjectile* missile)
{
	if(unitDef->canDropFlare){
		incomingMissiles.push_back(missile);
		AddDeathDependence(missile);

		if(lastFlareDrop < gs->frameNum - unitDef->flareReloadTime*30){
			SAFE_NEW CFlareProjectile(pos,speed,this,(int)(gs->frameNum+unitDef->flareDelay*(1+gs->randFloat())*15));
			lastFlareDrop=gs->frameNum;
		}
	}
}

void CUnit::TempHoldFire(void)
{
	dontFire=true;
	AttackUnit(0,true);
}

void CUnit::ReleaseTempHoldFire(void)
{
	dontFire=false;
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

// Member bindings
CR_REG_METADATA(CUnit, (
				CR_MEMBER(unitDef),
				CR_MEMBER(id),
				CR_MEMBER(team),
				CR_MEMBER(allyteam),
				CR_MEMBER(aihint),
				CR_MEMBER(frontdir),
				CR_MEMBER(rightdir),
				CR_MEMBER(updir),
				CR_MEMBER(upright),
				CR_MEMBER(relMidPos),
				CR_MEMBER(power),
				CR_MEMBER(noSelect),
				CR_MEMBER(noMinimap),

				CR_MEMBER(maxHealth),
				CR_MEMBER(health),
				CR_MEMBER(paralyzeDamage),
				CR_MEMBER(captureProgress),
				CR_MEMBER(experience),
				CR_MEMBER(limExperience),
				CR_MEMBER(logExperience),
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

				CR_MEMBER(deathCountdown),
				CR_MEMBER(delayedWreckLevel),
				CR_MEMBER(restTime),

				CR_MEMBER(weapons),
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
				CR_MEMBER(hasRadarCapacity),
				CR_MEMBER(radarSquares),
				CR_MEMBER(oldRadarPos),
				CR_MEMBER(stealth),

//				CR_MEMBER(commandAI),
				CR_MEMBER(moveType),
				CR_MEMBER(prevMoveType),
				CR_MEMBER(usingScriptMoveType),
//				CR_MEMBER(group),

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

				//CR_MEMBER(model),
				//CR_MEMBER(cob),
				//CR_MEMBER(localmodel),

				CR_MEMBER(tooltip),
				CR_MEMBER(crashing),
				CR_MEMBER(isDead),

				CR_MEMBER(bonusShieldDir),
				CR_MEMBER(bonusShieldSaved),

				CR_MEMBER(armoredState),
				CR_MEMBER(armoredMultiple),
				CR_MEMBER(curArmorMultiple),

				CR_MEMBER(wreckName),
				CR_MEMBER(posErrorVector),
				CR_MEMBER(posErrorDelta),
				CR_MEMBER(nextPosErrorUpdate),

				CR_MEMBER(hasUWWeapons),

				CR_MEMBER(wantCloak),
				CR_MEMBER(cloakTimeout),
				CR_MEMBER(curCloakTimeout),
				CR_MEMBER(isCloaked),

				CR_MEMBER(lastTerrainType),
				CR_MEMBER(curTerrainType),

				CR_MEMBER(selfDCountdown),

				//CR_MEMBER(directControl),

				//CR_MEMBER(myTrack),
				CR_MEMBER(incomingMissiles),
				CR_MEMBER(lastFlareDrop))
				);

