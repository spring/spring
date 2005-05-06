// UnitLoader.cpp: implementation of the CUnitLoader class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "StdAfx.h"
#include "UnitLoader.h"
#include "UnitParser.h"
#include "Unit.h"
#include "InfoConsole.h"
#include "RocketLauncher.h"
#include "Cannon.h"
#include "bombdropper.h"
#include "FlameThrower.h"
#include "Rifle.h"
#include "Ground.h"
#include "Factory.h"
#include "Builder.h"
#include "MeleeWeapon.h"
#include "Sound.h"
#include "SunParser.h"
#include "UnitDefHandler.h"
#include "3DOParser.h"
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


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitLoader unitLoader;

CUnitLoader::CUnitLoader()
{
	parser=new CUnitParser;
}

CUnitLoader::~CUnitLoader()
{
	delete parser;
}

CUnit* CUnitLoader::LoadUnit(const string& name, float3 pos, int side, bool build)
{
	CUnit* unit;
START_TIME_PROFILE;

	UnitDef* ud= unitDefHandler->GetUnitByName(name);
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
	unit->controlRadius=ud->controlRadius/SQUARE_SIZE;
	unit->losHeight=ud->losHeight;
	unit->metalCost=ud->metalCost;
	unit->energyCost=ud->energyCost;
	unit->buildTime=ud->buildTime;
	unit->aihint=ud->aihint;
	unit->tooltip=ud->humanName + " " + ud->tooltip;
	unit->armoredMultiple=max(0.0001f,ud->armoredMultiple);		//armored multiple of 0 will crash spring
	unit->wreckName=ud->wreckName;
	unit->realLosRadius=ud->losRadius/(SQUARE_SIZE*2);
	unit->realAirLosRadius=ud->airLosRadius/(SQUARE_SIZE*4);
	unit->upright=ud->upright;
	unit->xsize=ud->xsize;
	unit->ysize=ud->ysize;
	unit->radarRadius=ud->radarRadius/(SQUARE_SIZE*8);
	unit->sonarRadius=ud->sonarRadius/(SQUARE_SIZE*8);
	unit->jammerRadius=ud->jammerRadius/(SQUARE_SIZE*8);
	unit->stealth=ud->stealth;
	unit->category=ud->category;
	unit->armorType=ud->armorType;

	if(build){
		unit->ChangeLos(1,1);
		unit->health=0.1;
	} else {
		unit->ChangeLos(ud->losRadius/SQUARE_SIZE,ud->airLosRadius/(SQUARE_SIZE*2));
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
		unit->mobility->maxTurnRate = ud->turnRate;
		unit->mobility->moveData = ud->movedata;

		//Unit parameters
		unit->floatOnWater = (ud->movedata->moveType == MoveData::Hover_Move) || (ud->movedata->moveType == MoveData::Ship_Move);
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
		unit->mobility->maxTurnRate = ud->turnRate;
		unit->mobility->moveData = ud->movedata;

		unit->mass=ud->mass;

		if ((type == "Builder") || ud->hoverAttack || ud->transportCapacity) {
			CTAAirMoveType *mt = new CTAAirMoveType(unit);

			mt->turnRate = ud->turnRate;
			mt->maxSpeed = ud->speed / GAME_SPEED;
			mt->accRate = ud->maxAcc;
			mt->decRate = ud->maxDec;
			mt->wantedHeight = ud->wantedHeight;

			unit->moveType = mt;
		}
		else {
			CAirMoveType *mt = new CAirMoveType(unit);
			//mt->useHeading = false;
		
			mt->wingAngle = ud->wingAngle;
			mt->invDrag = 1 - ud->drag;
			mt->frontToSpeed = ud->frontToSpeed;
			mt->speedToFront = ud->speedToFront;
			mt->myGravity = ud->myGravity;

			mt->maxBank = ud->maxBank;
			mt->maxPitch = ud->maxPitch;
			mt->turnRadius = ud->turnRadius;
			mt->wantedHeight = ud->wantedHeight;

			mt->maxAcc = ud->maxAcc;
			mt->maxAileron = ud->maxAileron;
			mt->maxElevator = ud->maxElevator;
			mt->maxRudder = ud->maxRudder;

			if(type=="Fighter")
				mt->isFighter=true;

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
	unit->model = unit3doparser->Load3DO((ud->model.modelpath).c_str(),ud->canfly?0.5:1,side); 	//this is a hack to make aircrafts less likely to collide and get hit by nontracking weapons
	unit->SetRadius(unit->model->radius);

	if(ud->floater)
		unit->pos.y = max(-ud->waterline,ground->GetHeight2(unit->pos.x,unit->pos.z));
	else
		unit->pos.y=ground->GetHeight2(unit->pos.x,unit->pos.z);
	//unit->pos.y=ground->GetHeight(unit->pos.x,unit->pos.z);

	unit->cob = new CCobInstance(GCobEngine.GetCobFile("scripts\\" + ud->model.modelname+".cob"), unit);
	unit->cob->Call(COBFN_Create);

	unit->localmodel = unit3doparser->CreateLocalModel(unit->model, &unit->cob->pieces);

	for(unsigned int i=0; i< /*(ud->type=="Bomber" ? 1:*/ud->weapons.size(); i++)
		unit->weapons.push_back(LoadWeapon(ud->weapons[i],unit));
	
	unit->Init();

	if(!build)
		unit->FinishedBuilding();

END_TIME_PROFILE("Unit loader");
	return unit;
}

CWeapon* CUnitLoader::LoadWeapon(WeaponDef *weapondef, CUnit* owner)
{

	CWeapon* weapon;

	if(!weapondef){
		weapon=new CNoWeapon(owner);
		weapon->weaponDef=weaponDefHandler->GetWeapon("NoWeapon");
		return weapon;
	} else if(weapondef->type=="Rocket"){
		weapon=new CRocketLauncher(owner);
	} else if(weapondef->type=="Cannon"){
		weapon=new CCannon(owner);
		((CCannon*)weapon)->highTrajectory=weapondef->highTrajectory;
		((CCannon*)weapon)->selfExplode=weapondef->movement.selfExplode;
	} else if(weapondef->type=="Rifle"){
		weapon=new CRifle(owner);
	} else if(weapondef->type=="Melee"){
		weapon=new CMeleeWeapon(owner);
	} else if(weapondef->type=="AircraftBomb"){
		weapon=new CBombDropper(owner);
	} else if(weapondef->type=="flame"){
		weapon=new CFlameThrower(owner);
	} else if(weapondef->type=="MissileLauncher"){
		weapon=new CMissileLauncher(owner);
		((CMissileLauncher*)weapon)->tracking=weapondef->movement.tracking;
	} else if(weapondef->type=="TorpedoLauncher"){
		weapon=new CTorpedoLauncher(owner);
		((CTorpedoLauncher*)weapon)->tracking=weapondef->movement.tracking;
	} else if(weapondef->type=="LaserCannon"){
		weapon=new CLaserCannon(owner);
		((CLaserCannon*)weapon)->color=weapondef->visuals.color;
	} else if(weapondef->type=="LightingCannon"){
		weapon=new CLightingCannon(owner);
		((CLightingCannon*)weapon)->color=weapondef->visuals.color;
	} else if(weapondef->type=="EmgCannon"){
		weapon=new CEmgCannon(owner);
	} else if(weapondef->type=="DGun"){
		weapon=new CDGunWeapon(owner);
	} else if(weapondef->type=="StarburstLauncher"){
		weapon=new CStarburstLauncher(owner);
		((CStarburstLauncher*)weapon)->tracking=weapondef->movement.tracking;
		((CStarburstLauncher*)weapon)->uptime=weapondef->uptime*30;
	}else {
		(*info) << "Unknown weapon type " << weapondef->type.c_str() << "\n";
		return 0;
	}
	weapon->weaponDef = weapondef;

	weapon->reloadTime=weapondef->reload*GAME_SPEED;
	weapon->range=weapondef->range;
//	weapon->baseRange=weapondef->range;
	weapon->heightMod=weapondef->heightmod;
	weapon->projectileSpeed=weapondef->movement.projectilespeed;
//	weapon->baseProjectileSpeed=weapondef->projectilespeed/GAME_SPEED;

	weapon->damages = weapondef->damages; //not needed anymore, to be removed

	weapon->areaOfEffect=weapondef->areaOfEffect;
	weapon->accuracy=weapondef->accuracy;
	weapon->sprayangle=weapondef->sprayangle;

	weapon->salvoSize=weapondef->salvosize;
	weapon->salvoDelay=weapondef->salvodelay*GAME_SPEED;

	weapon->metalFireCost=weapondef->metalcost;
	weapon->energyFireCost=weapondef->energycost;

	/*CFileHandler ffile("sounds\\"+weapondef->sfiresound);
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
		weapondef->firesound.volume=soundVolume;
		if(weapon->areaOfEffect>8)
			soundVolume*=2;
		weapondef->soundhit.volume=soundVolume;
	}
	weapon->fireSoundId = weapondef->firesound.id;
	weapon->fireSoundVolume=weapondef->firesound.volume;

	weapon->onlyForward=weapondef->onlyForward;
	if(owner->unitDef->type=="Fighter" && !owner->unitDef->hoverAttack)		//fighter aircrafts have to big tolerance in ta
		weapon->maxAngleDif=cos(weapondef->maxAngle*0.4/180*PI);
	else
		weapon->maxAngleDif=cos(weapondef->maxAngle/180*PI);

	weapon->weaponNum=owner->weapons.size();

	weapon->badTargetCategory=weapondef->badTargetCategory;
	weapon->onlyTargetCategory=weapondef->onlyTargetCategory;

	weapon->Init();

	return weapon;
}

bool CUnitLoader::CanBuildUnit(string name, int team)
{
	return true;
}

int CUnitLoader::GetTechLevel(string type)
{
	//CUnitParser::UnitInfo* unitinfo=parser->ParseUnit(type);
	//return atoi(unitinfo->info["techlevel"].c_str());
	return 0;
//	int id = unitDefHandler.unitID[type];
//	return unitDefHandler.unitDefs[id].techlevel;
}

string CUnitLoader::GetName(string name)
{
	return name;

	/*string s3;
	int lastdot=name.find_last_of('.');
	for(int a=0;a<lastdot;a++)
		s3+=name[a];
	return s3;*/
}
