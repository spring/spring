// UnitLoader.cpp: implementation of the CUnitLoader class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "UnitLoader.h"
#include "Unit.h"
#include "InfoConsole.h"
#include "Cannon.h"
#include "bombdropper.h"
#include "FlameThrower.h"
#include "Rifle.h"
#include "Ground.h"
#include "Factory.h"
#include "Builder.h"
#include "MeleeWeapon.h"
#include "Sound.h"
#include "UnitDefHandler.h"
#include "3DModelParser.h"
#include "CobEngine.h"
#include "CobInstance.h"
#include "CobFile.h"
#include "CommandAI.h"
#include "BuilderCAI.h"
#include "FactoryCAI.h"
#include "AirCAI.h"
#include "MobileCAI.h"
#include "MissileLauncher.h"
#include "TorpedoLauncher.h"
#include "LaserCannon.h"
#include "EmgCannon.h"
#include "StarburstLauncher.h"
#include "LightingCannon.h"
#include "AirMoveType.h"
#include "groundmovetype.h"
#include "ExtractorBuilding.h"
#include "NoWeapon.h"
#include "TransportUnit.h"
#include "TransportCAI.h"
#include "TAAirMoveType.h"
#include "WeaponDefHandler.h"
#include "DGunWeapon.h"
#include "TimeProfiler.h"
#include "PlasmaRepulser.h"
#include "BeamLaser.h"
#include "errorhandler.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitLoader unitLoader;

CUnitLoader::CUnitLoader()
{

	CGroundMoveType::CreateLineTable();
}

CUnitLoader::~CUnitLoader()
{
}

CUnit* CUnitLoader::LoadUnit(const string& name, float3 pos, int side, bool build)
{
	CUnit* unit;
START_TIME_PROFILE;

	UnitDef* ud= unitDefHandler->GetUnitByName(name);
	if(!ud){
		char t[500];
		sprintf(t,"Couldnt find unittype %s",name.c_str());
		handleerror(0,t,"Error loading unit",0);
		exit(0);
	}
	string type = ud->type;

	if(!build){
		pos.y=ground->GetHeight2(pos.x,pos.z);
		if(ud->floater && pos.y<0)
			pos.y = -ud->waterline;
	}
	bool blocking = false;	//Used to tell if ground area shall be blocked of not.

	//unit = new CUnit(pos, side);
	if(type=="GroundUnit"){
		unit=new CUnit(pos,side,ud);
		blocking = true;
	} else if (type=="Transport"){
		unit=new CTransportUnit(pos,side,ud);
		blocking = true;
	} else if (type=="Building"){
		unit=new CBuilding(pos,side,ud);
		blocking = true;
	} else if (type=="Factory"){
		unit=new CFactory(pos,side,ud);
		blocking = true;
	} else if (type=="Builder"){
		unit=new CBuilder(pos,side,ud);
		blocking = true;
	} else if (type=="Bomber" || type=="Fighter"){
		unit=new CUnit(pos, side,ud);
	} else if (type == "MetalExtractor") {
		unit = new CExtractorBuilding(pos, side,ud);
		blocking = true;
	} else {
		(*info) << "Unknown unit type " << type.c_str() << "\n";
		return 0;
	}

	unit->beingBuilt=build;
	unit->power=ud->power;
	unit->maxHealth=ud->health;
	unit->health=ud->health;
	//unit->metalUpkeep=ud->metalUpkeep*16.0f/GAME_SPEED;
	//unit->energyUpkeep=ud->energyUpkeep*16.0f/GAME_SPEED;
	unit->controlRadius=(int)(ud->controlRadius/SQUARE_SIZE);
	unit->losHeight=ud->losHeight;
	unit->metalCost=ud->metalCost;
	unit->energyCost=ud->energyCost;
	unit->buildTime=ud->buildTime;
	unit->aihint=ud->aihint;
	unit->tooltip=ud->humanName + " " + ud->tooltip;
	unit->armoredMultiple=max(0.0001f,ud->armoredMultiple);		//armored multiple of 0 will crash spring
	unit->wreckName=ud->wreckName;
	unit->realLosRadius=(int) (ud->losRadius/(SQUARE_SIZE*2));
	unit->realAirLosRadius=(int) (ud->airLosRadius/(SQUARE_SIZE*4));
	unit->upright=ud->upright;
	unit->xsize=ud->xsize;
	unit->ysize=ud->ysize;
	unit->radarRadius=ud->radarRadius/(SQUARE_SIZE*8);
	unit->sonarRadius=ud->sonarRadius/(SQUARE_SIZE*8);
	unit->jammerRadius=ud->jammerRadius/(SQUARE_SIZE*8);
	unit->sonarJamRadius=ud->sonarJamRadius/(SQUARE_SIZE*8);
	unit->hasRadarCapacity=unit->radarRadius || unit->sonarRadius || unit->jammerRadius || unit->sonarJamRadius;
	unit->stealth=ud->stealth;
	unit->category=ud->category;
	unit->armorType=ud->armorType;
	unit->floatOnWater = ud->floater || (ud->movedata && ((ud->movedata->moveType == MoveData::Hover_Move) || (ud->movedata->moveType == MoveData::Ship_Move)));
	
	if(ud->highTrajectoryType==1)
		unit->useHighTrajectory=true;

	if(build){
		unit->ChangeLos(1,1);
		unit->health=0.1;
	} else {
		unit->ChangeLos((int)(ud->losRadius/SQUARE_SIZE),(int)(ud->airLosRadius/(SQUARE_SIZE*2)));
	}

	if(type=="GroundUnit"){
		new CMobileCAI(unit);
	} else if(type=="Transport"){
		new CTransportCAI(unit);
	} else if(type=="Factory"){
		new CFactoryCAI(unit);
	} else if(type=="Builder"){
		new CBuilderCAI(unit);
	} else if(type=="Bomber"){
		if (ud->hoverAttack)
			new CMobileCAI(unit);
		else
			new CAirCAI(unit);
	} else if(type=="Fighter"){
		if (ud->hoverAttack)
			new CMobileCAI(unit);
		else
			new CAirCAI(unit);
	} else {
		new CCommandAI(unit);
	}

	if(ud->canmove && !ud->canfly && type!="Factory"){
		CGroundMoveType* mt=new CGroundMoveType(unit);
		mt->maxSpeed=ud->speed/GAME_SPEED;
		mt->maxWantedSpeed=ud->speed/GAME_SPEED;
		mt->turnRate=ud->turnRate;
		mt->baseTurnRate=ud->turnRate;
		mt->moveType=ud->moveType;
		mt->accRate=ud->maxAcc;
		mt->floatOnWater=ud->movedata->moveType==MoveData::Hover_Move || ud->movedata->moveType==MoveData::Ship_Move;
		unit->mass=ud->mass;
		unit->moveType=mt;


		//Ground-mobility
		unit->mobility = new CMobility();
		unit->mobility->canFly = false;
		unit->mobility->subMarine = false;		//Not always correct, as submarines are treated as ships.
		unit->mobility->maxAcceleration = ud->maxAcc;
		unit->mobility->maxBreaking = -3*ud->maxAcc;
		unit->mobility->maxSpeed = ud->speed / GAME_SPEED;
		unit->mobility->maxTurnRate = (short int) ud->turnRate;
		unit->mobility->moveData = ud->movedata;

		//Unit parameters
		unit->mass = ud->mass;	//Should this not be done for all units?!  //buildings have mass 100000 to show they are immobile
//		unit->moveType = new CDummyMoveType(unit);	//Disable old movetype-system.

		//Uggly addition of UnitAI.
//		unit->ai = new CUnitAI();
//		unit->ai->moveAAI = new CSurfaceMoveAAI(unit);

	} else if(ud->canfly){
		//Air-mobility
		unit->mobility = new CMobility();
		unit->mobility->canFly = true;
		unit->mobility->subMarine = false;
		unit->mobility->maxAcceleration = ud->maxAcc;
		unit->mobility->maxBreaking = -3*ud->maxAcc;	//Correct?
		unit->mobility->maxSpeed = ud->speed / GAME_SPEED;
		unit->mobility->maxTurnRate = (short int) ud->turnRate;
		unit->mobility->moveData = ud->movedata;

		unit->mass=ud->mass;

		if ((type == "Builder") || ud->hoverAttack || ud->transportCapacity) {
			CTAAirMoveType *mt = new CTAAirMoveType(unit);

			mt->turnRate = ud->turnRate;
			mt->maxSpeed = ud->speed / GAME_SPEED;
			mt->accRate = ud->maxAcc;
			mt->decRate = ud->maxDec;
			mt->wantedHeight = ud->wantedHeight+gs->randFloat()*5;
			mt->orgWantedHeight=mt->wantedHeight;

			unit->moveType = mt;
		}
		else {
			CAirMoveType *mt = new CAirMoveType(unit);
		
			if(type=="Fighter")
				mt->isFighter=true;

			mt->wingAngle = ud->wingAngle;
			mt->invDrag = 1 - ud->drag;
			mt->frontToSpeed = ud->frontToSpeed;
			mt->speedToFront = ud->speedToFront;
			mt->myGravity = ud->myGravity;

			mt->maxBank = ud->maxBank;
			mt->maxPitch = ud->maxPitch;
			mt->turnRadius = ud->turnRadius;
			mt->wantedHeight = ud->wantedHeight*1.5+(gs->randFloat()-0.3)*15*(mt->isFighter?2:1);;

			mt->maxAcc = ud->maxAcc;
			mt->maxAileron = ud->maxAileron;
			mt->maxElevator = ud->maxElevator;
			mt->maxRudder = ud->maxRudder;

			unit->moveType = mt;
		}
	} else {
		unit->moveType=new CMoveType(unit);
		unit->upright=true;
	}

	unit->energyTickMake = ud->energyMake;
	if(ud->tidalGenerator>0)
		unit->energyTickMake += ud->tidalGenerator*readmap->tidalStrength;


//	if(!ud->weapons.empty())
//		unit->mainDamageType=unit->weapons.front()->damageType;

	//unit->model=unitModelLoader->GetModel(ud->model.modelname,side);
	unit->model = modelParser->Load3DO((ud->model.modelpath).c_str(),ud->canfly?0.5:1,side); 	//this is a hack to make aircrafts less likely to collide and get hit by nontracking weapons
	unit->SetRadius(unit->model->radius);

	if(ud->floater)
		unit->pos.y = max(-ud->waterline,ground->GetHeight2(unit->pos.x,unit->pos.z));
	else
		unit->pos.y=ground->GetHeight2(unit->pos.x,unit->pos.z);
	//unit->pos.y=ground->GetHeight(unit->pos.x,unit->pos.z);

	unit->cob = new CCobInstance(GCobEngine.GetCobFile("scripts/" + name+".cob"), unit);
	unit->localmodel = modelParser->CreateLocalModel(unit->model, &unit->cob->pieces);

	for(unsigned int i=0; i< ud->weapons.size(); i++)
		unit->weapons.push_back(LoadWeapon(ud->weapons[i].def,unit,&ud->weapons[i]));
	
	// Calculate the max() of the available weapon reloadtimes
	int relMax = 0;
	for (vector<CWeapon*>::iterator i = unit->weapons.begin(); i != unit->weapons.end(); ++i) {
		if ((*i)->reloadTime > relMax)
			relMax = (*i)->reloadTime;
		if(dynamic_cast<CBeamLaser*>(*i))
			relMax=150;
	}
	relMax *= 30;		// convert ticks to milliseconds

	// TA does some special handling depending on weapon count
	if (unit->weapons.size() > 1)
		relMax = max(relMax, 3000);

	// Call initializing script functions
	unit->cob->Call(COBFN_Create);
	unit->cob->Call("SetMaxReloadTime", relMax);

	unit->Init();

	if(!build)
		unit->FinishedBuilding();

END_TIME_PROFILE("Unit loader");
	return unit;
}

CWeapon* CUnitLoader::LoadWeapon(WeaponDef *weapondef, CUnit* owner,UnitDef::UnitDefWeapon* udw)
{

	CWeapon* weapon;

	if(!weapondef){
		info->AddLine("Error: No weapon def?");
	}

	if(udw->name=="NOWEAPON"){
		weapon=new CNoWeapon(owner);
	} else if(weapondef->type=="Cannon"){
		weapon=new CCannon(owner);
		((CCannon*)weapon)->selfExplode=weapondef->selfExplode;
	} else if(weapondef->type=="Rifle"){
		weapon=new CRifle(owner);
	} else if(weapondef->type=="Melee"){
		weapon=new CMeleeWeapon(owner);
	} else if(weapondef->type=="AircraftBomb"){
		weapon=new CBombDropper(owner,false);
	} else if(weapondef->type=="PlasmaRepulser"){
		weapon=new CPlasmaRepulser(owner);
	} else if(weapondef->type=="flame"){
		weapon=new CFlameThrower(owner);
	} else if(weapondef->type=="MissileLauncher"){
		weapon=new CMissileLauncher(owner);
	} else if(weapondef->type=="TorpedoLauncher"){
		if(weapondef->onlyForward){	//assume aircraft
			weapon=new CBombDropper(owner,true);
			if(weapondef->tracks)
				((CBombDropper*)weapon)->tracking=weapondef->turnrate;
			((CBombDropper*)weapon)->bombMoveRange=weapondef->range;
		} else {
			weapon=new CTorpedoLauncher(owner);
			if(weapondef->tracks)
				((CTorpedoLauncher*)weapon)->tracking=weapondef->turnrate;
		}
	} else if(weapondef->type=="LaserCannon"){
		weapon=new CLaserCannon(owner);
		((CLaserCannon*)weapon)->color=weapondef->visuals.color;
	} else if(weapondef->type=="BeamLaser"){
		weapon=new CBeamLaser(owner);
		((CBeamLaser*)weapon)->color=weapondef->visuals.color;
	} else if(weapondef->type=="LightingCannon"){
		weapon=new CLightingCannon(owner);
		((CLightingCannon*)weapon)->color=weapondef->visuals.color;
	} else if(weapondef->type=="EmgCannon"){
		weapon=new CEmgCannon(owner);
	} else if(weapondef->type=="DGun"){
		weapon=new CDGunWeapon(owner);
	} else if(weapondef->type=="StarburstLauncher"){
		weapon=new CStarburstLauncher(owner);
		if(weapondef->tracks)
			((CStarburstLauncher*)weapon)->tracking=weapondef->turnrate;
		else
			((CStarburstLauncher*)weapon)->tracking=0;
		((CStarburstLauncher*)weapon)->uptime=weapondef->uptime*30;
	}else {
		(*info) << "Unknown weapon type " << weapondef->type.c_str() << "\n";
		return 0;
	}
	weapon->weaponDef = weapondef;

	weapon->reloadTime= (int) (weapondef->reload*GAME_SPEED);
	if(weapon->reloadTime==0)
		weapon->reloadTime=1;
	weapon->range=weapondef->range;
//	weapon->baseRange=weapondef->range;
	weapon->heightMod=weapondef->heightmod;
	weapon->projectileSpeed=weapondef->projectilespeed;
//	weapon->baseProjectileSpeed=weapondef->projectilespeed/GAME_SPEED;

	weapon->damages = weapondef->damages; //not needed anymore, to be removed

	weapon->areaOfEffect=weapondef->areaOfEffect;
	weapon->accuracy=weapondef->accuracy;
	weapon->sprayangle=weapondef->sprayangle;

	weapon->salvoSize=weapondef->salvosize;
	weapon->salvoDelay=(int) (weapondef->salvodelay*GAME_SPEED);

	weapon->metalFireCost=weapondef->metalcost;
	weapon->energyFireCost=weapondef->energycost;

	/*CFileHandler ffile("sounds/"+weapondef->sfiresound);
	if(ffile.FileExists())
	{
		weapondef->firesound = sound->GetWaveId(weapondef->sfiresound);
		weapon->fireSoundId = weapondef->firesoundId;
	}
	else
	{
		weapondef->firesoundId = 0;
		weapon->fireSoundId = 0;
	}
	if(weapondef->ssoundhit!="none.wav")
	{
		weapondef->soundhitId = sound->GetWaveId(weapondef->ssoundhit);
	}
	else
	{
		weapondef->soundhitId = 0;
	}*/
	CWeaponDefHandler::LoadSound(weapondef->firesound);
	CWeaponDefHandler::LoadSound(weapondef->soundhit);

	if(weapon->damages[0]>50){
		float soundVolume=sqrt(weapon->damages[0]*0.5);
		if(weapondef->type=="LaserCannon")
			soundVolume*=0.5;
		float hitSoundVolume=soundVolume;
		if((weapondef->type=="MissileLauncher" || weapondef->type=="StarburstLauncher") && soundVolume>100)
			soundVolume=10*sqrt(soundVolume);
		weapondef->firesound.volume=soundVolume;
		soundVolume=hitSoundVolume;
		if(weapon->areaOfEffect>8)
			soundVolume*=2;
		if(weapondef->type=="DGun")
			soundVolume*=0.15;
		weapondef->soundhit.volume=soundVolume;
	}
	weapon->fireSoundId = weapondef->firesound.id;
	weapon->fireSoundVolume=weapondef->firesound.volume;

	weapon->onlyForward=weapondef->onlyForward;
	if(owner->unitDef->type=="Fighter" && !owner->unitDef->hoverAttack)		//fighter aircrafts have too big tolerance in ta
		weapon->maxAngleDif=cos(weapondef->maxAngle*0.4/180*PI);
	else
		weapon->maxAngleDif=cos(weapondef->maxAngle/180*PI);

	weapon->weaponNum=owner->weapons.size();

	weapon->badTargetCategory=udw->badTargetCat;
	weapon->onlyTargetCategory=weapondef->onlyTargetCategory & udw->onlyTargetCat;

	if(udw->slavedTo)
		weapon->slavedTo=owner->weapons[udw->slavedTo-1];

	weapon->mainDir=udw->mainDir;
	weapon->maxMainDirAngleDif=udw->maxAngleDif;

	weapon->Init();

	return weapon;
}
