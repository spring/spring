#include "StdAfx.h"
#include "UnitDefHandler.h"
#include "Rendering/GL/myGL.h"
#include "TdfParser.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <locale>
#include <cctype>
#include "FileSystem/FileHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "LogOutput.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sound.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "UnitDef.h"
#include "Map/ReadMap.h"
#include "Game/GameSetup.h"
#include "Rendering/GroundDecalHandler.h"
#include "Platform/ConfigHandler.h"
#include "Game/Team.h"
#include "mmgr.h"
#include "Sim/Misc/SensorHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"

CR_BIND(UnitDef, );

const char YARDMAP_CHAR = 'c';		//Need to be low case.

CUnitDefHandler* unitDefHandler;

CUnitDefHandler::CUnitDefHandler(void)
{
	noCost=false;
	std::string dir = "units/";

	PrintLoadMsg("Loading units and weapons");

	weaponDefHandler = SAFE_NEW CWeaponDefHandler();

	numUnits = 0;

	std::vector<std::string> tafiles = CFileHandler::FindFiles("units/", "*.fbi");
	std::vector<std::string> tafiles2 = CFileHandler::FindFiles("units/", "*.swu");
	while (!tafiles2.empty()) {
		tafiles.push_back(tafiles2.back());
		tafiles2.pop_back();
	}

	soundcategory.LoadFile("gamedata/SOUND.TDF");

	numUnits = tafiles.size();
	if (gameSetup) {
		numUnits -= gameSetup->restrictedUnits.size();
	}

	// This could be wasteful if there is a lot of restricted units, but that is not that likely
	unitDefs = SAFE_NEW UnitDef[numUnits + 1];

	unsigned int id = 1;  // Start at unit id 1

	for(unsigned int a = 0; a < tafiles.size(); ++a)
	{
		// Determine the name (in lowercase) first
		const int len = tafiles[a].find_last_of("/") + 1;
		std::string unitname = StringToLower(tafiles[a].substr(len, tafiles[a].size() - 4 - len));

		// Restrictions may tell us not to use this unit at all
		if (gameSetup) {
			const std::map<std::string, int>& resUnits = gameSetup->restrictedUnits;
		  if ((resUnits.find(unitname) != resUnits.end()) &&
			    (resUnits.find(unitname)->second == 0)) {
				continue;
			}
		}

		// Seems ok, load it
		unitDefs[id].valid = false;
		unitDefs[id].filename = tafiles[a];
		unitDefs[id].name = unitname;
		unitDefs[id].id = id;
		unitDefs[id].buildangle = 0;
		unitDefs[id].unitimage = 0;
		unitDefs[id].imageSizeX = -1;
		unitDefs[id].imageSizeY = -1;
		unitDefs[id].techLevel = -1;
		unitDefs[id].decoyDef = NULL;
		unitID[unitname] = id;
		for (int ym = 0; ym < 4; ym++) {
			unitDefs[id].yardmaps[ym] = 0;
		}

		// parse the TDF data (but don't load buildpics, etc...)
		ParseUnit(unitDefs[id].filename, id);

		// Increase index for next unit
		id++;
	}

	// set the real number of unitdefs
	numUnits = (id - 1);

	FindTABuildOpt();

	ProcessDecoys();

	AssignTechLevels();
}


CUnitDefHandler::~CUnitDefHandler(void)
{
	//Deleting eventual yeardmaps.
	for(int i = 1; i <= numUnits; i++){
		for (int u = 0; u < 4; u++)
			delete[] unitDefs[i].yardmaps[u];

		if (unitDefs[i].unitimage) {
			glDeleteTextures(1,&unitDefs[i].unitimage);
			unitDefs[i].unitimage = 0;
		}
	}
	delete[] unitDefs;
	delete weaponDefHandler;
}


void CUnitDefHandler::ProcessDecoys()
{
	// assign the decoy pointers, and build the decoy map
	map<string, string>::const_iterator mit;
	for (mit = decoyNameMap.begin(); mit != decoyNameMap.end(); ++mit) {
		map<string, int>::iterator fakeIt, realIt;
		fakeIt = unitID.find(mit->first);
		realIt = unitID.find(mit->second);
		if ((fakeIt != unitID.end()) && (realIt != unitID.end())) {
			UnitDef* fake = &unitDefs[fakeIt->second];
			UnitDef* real = &unitDefs[realIt->second];
			fake->decoyDef = real;
			decoyMap[real].insert(fake);
		}
	}
	decoyNameMap.clear();
}


void CUnitDefHandler::FindTABuildOpt()
{
	TdfParser tdfparser;
	tdfparser.LoadFile("gamedata/SIDEDATA.TDF");

	// get the commander IDs while SIDEDATA.TDF is open
	commanderIDs.clear();
	std::vector<std::string> sides = tdfparser.GetSectionList("");
	for (unsigned int i=0; i<sides.size(); i++){
		const std::string& section = sides[i];
		if ((section.find("side") == 0) &&
		    (section.find_first_not_of("0123456789", 4) == std::string::npos)) {
			string commUnit = tdfparser.SGetValueDef("", section + "\\COMMANDER");
			StringToLowerInPlace(commUnit);
			if (!commUnit.empty()) {
				std::map<std::string, int>::iterator it = unitID.find(commUnit);
				if (it != unitID.end()) {
					commanderIDs.insert(it->second);
				}
			}
		}
	}

	std::vector<std::string> sideunits = tdfparser.GetSectionList("CANBUILD");
	for(unsigned int i=0; i<sideunits.size(); i++)
	{
		std::map<std::string, std::string>::const_iterator it;

		UnitDef *builder=NULL;
		StringToLowerInPlace(sideunits[i]);
		std::map<std::string, int>::iterator it1 = unitID.find(sideunits[i]);
		if(it1!= unitID.end())
			builder = &unitDefs[it1->second];

		if(builder)
		{
			const std::map<std::string, std::string>& buildoptlist = tdfparser.GetAllValues("CANBUILD\\" + sideunits[i]);
			for(it=buildoptlist.begin(); it!=buildoptlist.end(); it++)
			{
				UnitDef *buildopt=0;
				std::string opt = StringToLower(it->second);

				if(unitID.find(opt)!= unitID.end()){
					int num=atoi(it->first.substr(8).c_str());
					builder->buildOptions[num]=opt;
				}
			}
		}
	}

	std::vector<std::string> files = CFileHandler::FindFiles("download/", "*.tdf");
	for(unsigned int i=0; i<files.size(); i++)
	{
		TdfParser dparser(files[i]);

		std::vector<std::string> sectionlist = dparser.GetSectionList("");

		for(unsigned int j=0; j<sectionlist.size(); j++)
		{
			UnitDef *builder=NULL;
			std::string un1 = StringToLower(dparser.SGetValueDef("", sectionlist[j] + "\\UNITMENU"));
			std::map<std::string, int>::iterator it1 = unitID.find(un1);
			if(it1!= unitID.end())
				builder = &unitDefs[it1->second];

			if(builder)
			{
				UnitDef *buildopt=NULL;
				string un2 = StringToLower(dparser.SGetValueDef("", sectionlist[j] + "\\UNITNAME"));

				if(unitID.find(un2)!= unitID.end()){
					int menu=atoi(dparser.SGetValueDef("", sectionlist[j] + "\\MENU").c_str());
					int button=atoi(dparser.SGetValueDef("", sectionlist[j] + "\\BUTTON").c_str());
					int num=(menu-2)*6+button+1;
					builder->buildOptions[num]=un2;
				} else {
					logOutput.Print("couldnt find unit %s",un2.c_str());
				}
			}
		}

	}
}


void CUnitDefHandler::ParseTAUnit(std::string file, int id)
{
	TdfParser tdfparser(file);

	UnitDef& ud = unitDefs[id];

	const string decoy = tdfparser.SGetValueDef("", "UNITINFO\\DecoyFor");
	if (!decoy.empty()) {
		decoyNameMap[ud.name] = StringToLower(decoy);
	}

	ud.name = tdfparser.SGetValueMSG("UNITINFO\\UnitName");
	ud.humanName = tdfparser.SGetValueMSG("UNITINFO\\name");

	ud.gaia = tdfparser.SGetValueDef("", "UNITINFO\\gaia");

	tdfparser.GetDef(ud.extractsMetal, "0", "UNITINFO\\ExtractsMetal");
	tdfparser.GetDef(ud.windGenerator, "0", "UNITINFO\\WindGenerator");
	tdfparser.GetDef(ud.tidalGenerator, "0", "UNITINFO\\TidalGenerator");

	ud.health=atof(tdfparser.SGetValueDef("0", "UNITINFO\\MaxDamage").c_str());
	ud.metalUpkeep=atof(tdfparser.SGetValueDef("0", "UNITINFO\\MetalUse").c_str());
	ud.energyUpkeep=atof(tdfparser.SGetValueDef("0", "UNITINFO\\EnergyUse").c_str());
	ud.metalMake=atof(tdfparser.SGetValueDef("0", "UNITINFO\\MetalMake").c_str());
	ud.makesMetal=atof(tdfparser.SGetValueDef("0", "UNITINFO\\MakesMetal").c_str());
	ud.energyMake=atof(tdfparser.SGetValueDef("0", "UNITINFO\\EnergyMake").c_str());
	ud.metalStorage=atof(tdfparser.SGetValueDef("0", "UNITINFO\\MetalStorage").c_str());
	ud.energyStorage=atof(tdfparser.SGetValueDef("0", "UNITINFO\\EnergyStorage").c_str());

	ud.autoHeal=atof(tdfparser.SGetValueDef("0", "UNITINFO\\AutoHeal").c_str())*(16.0f/30.0f);
	ud.idleAutoHeal=atof(tdfparser.SGetValueDef("10", "UNITINFO\\IdleAutoHeal").c_str())*(16.0f/30.0f);
	ud.idleTime=atoi(tdfparser.SGetValueDef("600", "UNITINFO\\IdleTime").c_str());

	ud.buildangle=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\buildangle").c_str());

	ud.isMetalMaker=(ud.makesMetal>=1 && ud.energyUpkeep>ud.makesMetal*40);

	ud.controlRadius=32;
	ud.losHeight=20;
	ud.metalCost=atof(tdfparser.SGetValueDef("0", "UNITINFO\\BuildCostMetal").c_str());
	if(ud.metalCost<1)		//avoid some nasty divide by 0 etc
		ud.metalCost=1;
	ud.mass=atof(tdfparser.SGetValueDef("0", "UNITINFO\\Mass").c_str());
	if(ud.mass<=0)
		ud.mass=ud.metalCost;
	ud.energyCost=atof(tdfparser.SGetValueDef("0", "UNITINFO\\BuildCostEnergy").c_str());
	ud.buildTime=atof(tdfparser.SGetValueDef("0", "UNITINFO\\BuildTime").c_str());
	if(ud.buildTime<1)		//avoid some nasty divide by 0 etc
		ud.buildTime=1;
	ud.aihint=id;		//fix
	ud.losRadius=atof(tdfparser.SGetValueDef("0", "UNITINFO\\SightDistance").c_str())*sensorHandler->losMul/(SQUARE_SIZE*(1<<sensorHandler->losMipLevel));
	ud.airLosRadius=atof(tdfparser.SGetValueDef("-1", "UNITINFO\\AirSightDistance").c_str());
	if(ud.airLosRadius==-1)
		ud.airLosRadius=atof(tdfparser.SGetValueDef("0", "UNITINFO\\SightDistance").c_str())*sensorHandler->airLosMul*1.5f/(SQUARE_SIZE*(1<<sensorHandler->airMipLevel));
	else
		ud.airLosRadius = ud.airLosRadius*sensorHandler->airLosMul/(SQUARE_SIZE*(1<<sensorHandler->airMipLevel));

	ud.tooltip=tdfparser.SGetValueDef(ud.name,"UNITINFO\\Description");
	ud.moveType=0;

	tdfparser.GetDef(ud.canSubmerge, "0", "UNITINFO\\canSubmerge");
	tdfparser.GetDef(ud.canfly, "0", "UNITINFO\\canfly");
	tdfparser.GetDef(ud.canmove, "0", "UNITINFO\\canmove");
	tdfparser.GetDef(ud.reclaimable, "1", "UNITINFO\\reclaimable");
	tdfparser.GetDef(ud.noAutoFire,"0","UNITINFO\\noautofire");
	tdfparser.GetDef(ud.canAttack,"1","UNITINFO\\canattack");
	tdfparser.GetDef(ud.canFight,"1","UNITINFO\\canfight");
	tdfparser.GetDef(ud.canPatrol,"1","UNITINFO\\canpatrol");
	tdfparser.GetDef(ud.canGuard,"1","UNITINFO\\canguard");
	tdfparser.GetDef(ud.canRepeat,"1","UNITINFO\\canrepeat");

	tdfparser.GetDef(ud.builder, "0", "UNITINFO\\Builder");
	string x = "0";
	if(ud.builder) x = "1";

	tdfparser.GetDef(ud.canRestore,x,"UNITINFO\\canrestore");
	tdfparser.GetDef(ud.canRepair,x,"UNITINFO\\canrepair");
	tdfparser.GetDef(ud.canReclaim,x,"UNITINFO\\canreclaim");
	tdfparser.GetDef(ud.canBuild,x,"UNITINFO\\canbuild");
	tdfparser.GetDef(ud.canAssist,x,"UNITINFO\\canassist");

	tdfparser.GetDef(ud.upright, "0", "UNITINFO\\Upright");
	tdfparser.GetDef(ud.onoffable, "0", "UNITINFO\\onoffable");

	tdfparser.GetDef(ud.maxSlope, "0", "UNITINFO\\MaxSlope");
	ud.maxHeightDif=40*tan(ud.maxSlope*(PI/180));
	ud.maxSlope = cos(ud.maxSlope*(PI/180));
	tdfparser.GetDef(ud.minWaterDepth, "-10e6", "UNITINFO\\MinWaterDepth");
	tdfparser.GetDef(ud.maxWaterDepth, "10e6", "UNITINFO\\MaxWaterDepth");
	tdfparser.GetDef(ud.minCollisionSpeed, "1.0", "UNITINFO\\MinCollisionSpeed");
	tdfparser.GetDef(ud.slideTolerance, "2.0", "UNITINFO\\SlideTolerance");

	std::string value;
	ud.floater = tdfparser.SGetValue(value, "UNITINFO\\Waterline");

	tdfparser.GetDef(ud.waterline, "0", "UNITINFO\\Waterline");
	if(ud.waterline>8 && ud.canmove)
		ud.waterline+=5;		//make subs travel at somewhat larger depths to reduce vulnerability to surface weapons

	tdfparser.GetDef(ud.selfDCountdown, "5", "UNITINFO\\selfdestructcountdown");

	ud.speed=atof(tdfparser.SGetValueDef("0", "UNITINFO\\MaxVelocity").c_str())*30;
	ud.maxAcc=atof(tdfparser.SGetValueDef("0.5", "UNITINFO\\acceleration").c_str());
	ud.maxDec=atof(tdfparser.SGetValueDef("0.5", "UNITINFO\\BrakeRate").c_str())*0.1f;
	ud.turnRate=atof(tdfparser.SGetValueDef("0", "UNITINFO\\TurnRate").c_str());
	ud.buildSpeed=atof(tdfparser.SGetValueDef("0", "UNITINFO\\WorkerTime").c_str());
	ud.buildDistance=atof(tdfparser.SGetValueDef("64", "UNITINFO\\Builddistance").c_str());
	ud.buildDistance=std::max(128.f,ud.buildDistance);
	ud.armoredMultiple=atof(tdfparser.SGetValueDef("1", "UNITINFO\\DamageModifier").c_str());
	ud.armorType=damageArrayHandler->GetTypeFromName(ud.name);
//	logOutput.Print("unit %s has armor %i",ud.name.c_str(),ud.armorType);

	ud.radarRadius=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\RadarDistance").c_str());
	ud.sonarRadius=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\SonarDistance").c_str());
	ud.jammerRadius=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\RadarDistanceJam").c_str());
	ud.sonarJamRadius=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\SonarDistanceJam").c_str());
	ud.stealth=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\Stealth").c_str());
	ud.targfac=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\istargetingupgrade").c_str());
	ud.isFeature=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\IsFeature").c_str());
	ud.canResurrect=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\canResurrect").c_str());
	ud.canCapture=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\canCapture").c_str());
	ud.hideDamage=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\HideDamage").c_str());
	ud.isCommander=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\commander").c_str());
	ud.showPlayerName=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\showplayername").c_str());

	ud.cloakCost=atof(tdfparser.SGetValueDef("-1", "UNITINFO\\CloakCost").c_str());
	ud.cloakCostMoving=atof(tdfparser.SGetValueDef("-1", "UNITINFO\\CloakCostMoving").c_str());
	if(ud.cloakCostMoving<0)
		ud.cloakCostMoving=ud.cloakCost;
	if(ud.cloakCost>=0)
		ud.canCloak=true;
	else
		ud.canCloak=false;
	ud.startCloaked=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\init_cloaked").c_str());
	ud.decloakDistance=atof(tdfparser.SGetValueDef("-1", "UNITINFO\\mincloakdistance").c_str());

	ud.highTrajectoryType=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\HighTrajectory").c_str());

	ud.canKamikaze=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\kamikaze").c_str());
	ud.kamikazeDist=atof(tdfparser.SGetValueDef("-25", "UNITINFO\\kamikazedistance").c_str())+25; //we count 3d distance while ta count 2d distance so increase slightly

	tdfparser.GetDef(ud.showNanoFrame, "1", "UNITINFO\\shownanoframe");
	tdfparser.GetDef(ud.showNanoSpray, "1", "UNITINFO\\shownanospray");
	ud.nanoColor=tdfparser.GetFloat3(float3(0.2f,0.7f,0.2f),"UNITINFO\\nanocolor");

	tdfparser.GetDef(ud.canhover, "0", "UNITINFO\\canhover");

	if(tdfparser.SGetValue(value, "UNITINFO\\floater"))
		tdfparser.GetDef(ud.floater, "0", "UNITINFO\\floater");

	tdfparser.GetDef(ud.builder, "0", "UNITINFO\\Builder");
	if(ud.builder && !ud.buildSpeed)		//core anti is flagged as builder for some reason
		ud.builder=false;

	ud.wantedHeight=atof(tdfparser.SGetValueDef("0", "UNITINFO\\cruisealt").c_str());
	tdfparser.GetDef(ud.hoverAttack,"0", "UNITINFO\\hoverattack");
	tdfparser.GetDef(ud.airStrafe,"1", "UNITINFO\\airStrafe");
	ud.dlHoverFactor = atof(tdfparser.SGetValueDef("-1", "UNITINFO\\airhoverfactor").c_str());

	tdfparser.GetDef(ud.transportSize, "0", "UNITINFO\\transportsize");
	tdfparser.GetDef(ud.transportCapacity, "0", "UNITINFO\\transportcapacity");
	ud.isfireplatform=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\isfireplatform").c_str());
	ud.isAirBase=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\isAirBase").c_str());
	ud.loadingRadius=atof(tdfparser.SGetValueDef("220", "UNITINFO\\loadingradius").c_str());
	tdfparser.GetDef(ud.transportMass, "100000", "UNITINFO\\TransportMass");
	tdfparser.GetDef(ud.holdSteady, "1", "UNITINFO\\HoldSteady");

	tdfparser.GetDef(ud.wingDrag, "0.07", "UNITINFO\\WingDrag");				//drag caused by wings
	tdfparser.GetDef(ud.wingAngle, "0.08", "UNITINFO\\WingAngle");			//angle between front and the wing plane
	tdfparser.GetDef(ud.drag, "0.005", "UNITINFO\\Drag");								//how fast the aircraft loses speed (see also below)
	tdfparser.GetDef(ud.frontToSpeed, "0.1", "UNITINFO\\frontToSpeed");	//fudge factor for lining up speed and front of plane
	tdfparser.GetDef(ud.speedToFront, "0.07", "UNITINFO\\speedToFront");//fudge factor for lining up speed and front of plane
	tdfparser.GetDef(ud.myGravity, "0.4", "UNITINFO\\myGravity");				//planes are slower than real airplanes so lower gravity to compensate

	tdfparser.GetDef(ud.maxBank, "0.8", "UNITINFO\\maxBank");						//max roll
	tdfparser.GetDef(ud.maxPitch, "0.45", "UNITINFO\\maxPitch");				//max pitch this plane tries to keep
	tdfparser.GetDef(ud.turnRadius, "500", "UNITINFO\\turnRadius");			//hint to the ai about how large turn radius this plane needs

	tdfparser.GetDef(ud.maxAileron, "0.015", "UNITINFO\\maxAileron");		//turn speed around roll axis
	tdfparser.GetDef(ud.maxElevator, "0.01", "UNITINFO\\maxElevator");	//turn speed around pitch axis
	tdfparser.GetDef(ud.maxRudder, "0.004", "UNITINFO\\maxRudder");			//turn speed around yaw axis

	tdfparser.GetDef(ud.maxFuel, "0", "UNITINFO\\maxfuel");						//max flight time in seconds before aircraft must return to base
	tdfparser.GetDef(ud.refuelTime, "5", "UNITINFO\\refueltime");
	tdfparser.GetDef(ud.minAirBasePower, "0", "UNITINFO\\minairbasepower");

	tdfparser.GetTDef(ud.maxThisUnit, MAX_UNITS, "UNITINFO\\UnitRestricted");
	if (gameSetup)
	{
		string lname = StringToLower(ud.name);
		if (gameSetup->restrictedUnits.find(lname) != gameSetup->restrictedUnits.end()) {
			ud.maxThisUnit = min(ud.maxThisUnit, gameSetup->restrictedUnits.find(lname)->second);
		}
	}

	ud.categoryString=tdfparser.SGetValueDef("", "UNITINFO\\Category");
	ud.category=CCategoryHandler::Instance()->GetCategories(tdfparser.SGetValueDef("", "UNITINFO\\Category"));
	ud.noChaseCategory=CCategoryHandler::Instance()->GetCategories(tdfparser.SGetValueDef("", "UNITINFO\\NoChaseCategory"));
//	logOutput.Print("Unit %s has cat %i",ud.humanName.c_str(),ud.category);

	ud.iconType=tdfparser.SGetValueDef("default", "UNITINFO\\iconType");

	ud.shieldWeaponDef = NULL;
	ud.stockpileWeaponDef = NULL;

	ud.maxWeaponRange = 0.0f;

	for(int a=0;a<16;++a){
		char c[50];
		sprintf(c,"%i",a+1);

		string name;
		tdfparser.GetDef(name, "", std::string("UNITINFO\\")+"weapon"+c);
		WeaponDef *wd = weaponDefHandler->GetWeapon(name);

		if(!wd){
			if(a>2)	//allow empty weapons among the first 3
				break;
			else
				continue;
		}

		while(ud.weapons.size()<a){
			if (!weaponDefHandler->GetWeapon("NOWEAPON")) {
				logOutput.Print("Error: Spring requires a NOWEAPON weapon type to be present as a placeholder for missing weapons");
				break;
			} else {
				ud.weapons.push_back(UnitDef::UnitDefWeapon("NOWEAPON",weaponDefHandler->GetWeapon("NOWEAPON"),0,float3(0,0,1),-1,0,0,0));
			}
		}

		string badTarget;
		tdfparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "badTargetCategory"+c);
		unsigned int btc=CCategoryHandler::Instance()->GetCategories(badTarget);
		if(a<3){
			switch(a){
				case 0:
					tdfparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "wpri_badTargetCategory");
					break;
				case 1:
					tdfparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "wsec_badTargetCategory");
					break;
				case 2:
					tdfparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "wspe_badTargetCategory");
					break;
			}
			btc|=CCategoryHandler::Instance()->GetCategories(badTarget);
		}

		string onlyTarget;
		tdfparser.GetDef(onlyTarget, "", std::string("UNITINFO\\") + "onlyTargetCategory"+c);
		unsigned int otc;
		if(!onlyTarget.empty())
			otc=CCategoryHandler::Instance()->GetCategories(onlyTarget);
		else
			otc=0xffffffff;

		unsigned int slaveTo=atoi(tdfparser.SGetValueDef("0", string("UNITINFO\\WeaponSlaveTo")+c).c_str());

		float3 mainDir=tdfparser.GetFloat3(float3(1,0,0),string("UNITINFO\\WeaponMainDir")+c);
		mainDir.Normalize();

		float angleDif=cos((float)atof(tdfparser.SGetValueDef("360", string("UNITINFO\\MaxAngleDif")+c).c_str())*PI/360);

		float fuelUse=atof(tdfparser.SGetValueDef("0", string("UNITINFO\\WeaponFuelUsage")+c).c_str());

		ud.weapons.push_back(UnitDef::UnitDefWeapon(name,wd,slaveTo,mainDir,angleDif,btc,otc,fuelUse));

		if (wd->range > ud.maxWeaponRange) {
			ud.maxWeaponRange = wd->range;
		}
		if (wd->isShield) {
			if (!ud.shieldWeaponDef || // use the biggest shield
			    (ud.shieldWeaponDef->shieldRadius < wd->shieldRadius)) {
				ud.shieldWeaponDef = wd;
			}
		}
		if (wd->stockpile) {
			// interceptors have priority
			if (wd->interceptor        ||
			    !ud.stockpileWeaponDef ||
			    !ud.stockpileWeaponDef->interceptor) {
				ud.stockpileWeaponDef = wd;
			}
		}
	}

	tdfparser.GetDef(ud.canDGun, "0", "UNITINFO\\candgun");

	string TEDClass=tdfparser.SGetValueDef("0", "UNITINFO\\TEDClass").c_str();
	ud.TEDClassString=TEDClass;
	ud.extractRange=0;

	if(ud.extractsMetal) {
		ud.extractRange = readmap->extractorRadius;
		ud.type = "MetalExtractor";
	}
	else if(ud.transportCapacity)
	{
		ud.type = "Transport";
	}
	else if(ud.builder)
	{
		if(TEDClass!="PLANT")
			ud.type = "Builder";
		else
			ud.type = "Factory";
	}
	else if(ud.canfly && !ud.hoverAttack)
	{
		if(!ud.weapons.empty() && ud.weapons[0].def!=0 && (ud.weapons[0].def->type=="AircraftBomb" || ud.weapons[0].def->type=="TorpedoLauncher")){
			ud.type = "Bomber";
			if(ud.turnRadius==500)	//only reset it if user hasnt set it explicitly
				ud.turnRadius=1000;			//hint to the ai about how large turn radius this plane needs

		} else {
			ud.type = "Fighter";
		}
		tdfparser.GetDef(ud.maxAcc, "0.065", "UNITINFO\\maxAcc");						//engine power
	}
	else if(ud.canmove)
	{
		ud.type = "GroundUnit";
	}
	else
	{
		ud.type = "Building";
	}

	ud.movedata=0;
	if(ud.canmove && !ud.canfly && ud.type!="Factory"){
		string moveclass=tdfparser.SGetValueDef("", "UNITINFO\\MovementClass");
		ud.movedata=moveinfo->GetMoveDataFromName(moveclass);
		if (ud.movedata->moveType==MoveData::Hover_Move ||
		    ud.movedata->moveType==MoveData::Ship_Move) {
			ud.upright=true;
		}
		if (ud.canhover) {
			if (ud.movedata->moveType!=MoveData::Hover_Move) {
				logOutput.Print("Inconsistant move data hover %i %s %s",
				                ud.movedata->pathType,ud.humanName.c_str(),
				                moveclass.c_str());
			}
		} else if(ud.floater){
			if (ud.movedata->moveType!=MoveData::Ship_Move) {
				logOutput.Print("Inconsistant move data ship %i %s %s",
				                ud.movedata->pathType,ud.humanName.c_str(),
				                moveclass.c_str());
			}
		} else {
			if (ud.movedata->moveType!=MoveData::Ground_Move) {
				logOutput.Print("Inconsistant move data ground %i %s %s",
				                 ud.movedata->pathType,ud.humanName.c_str(),
				                 moveclass.c_str());
			}
		}
//		logOutput.Print("%s uses movetype %i",ud.humanName.c_str(),ud.movedata->pathType);
	}

	if(ud.maxAcc!=0 && ud.speed!=0)
		ud.drag=1.0f/(ud.speed/GAME_SPEED*1.1f/ud.maxAcc)-ud.wingAngle*ud.wingAngle*ud.wingDrag;		//meant to set the drag such that the maxspeed becomes what it should be

	std::string objectname;
	tdfparser.GetDef(objectname, "", "UNITINFO\\Objectname");
	ud.model.modelpath = "objects3d/" + objectname;
	ud.model.modelname = objectname;

	tdfparser.GetDef(ud.wreckName, "", "UNITINFO\\Corpse");
	tdfparser.GetDef(ud.deathExplosion, "", "UNITINFO\\ExplodeAs");
	tdfparser.GetDef(ud.selfDExplosion, "", "UNITINFO\\SelfDestructAs");

	tdfparser.GetDef(ud.buildpicname, "", "UNITINFO\\BuildPic");

	//ud.power = (ud.metalCost + ud.energyCost/60.0f);
	tdfparser.GetTDef(ud.power, (ud.metalCost + ud.energyCost/60.0f), "UNITINFO\\power");

	// Prevent a division by zero in experience calculations.
	if(ud.power<1e-3f){
		logOutput.Print("Unit %s is really cheap? %f",ud.humanName.c_str(),ud.power);
		logOutput.Print("This can cause a division by zero in experience calculations.");
		ud.power=1e-3f;
	}

	tdfparser.GetDef(ud.activateWhenBuilt, "0", "UNITINFO\\ActivateWhenBuilt");

	ud.xsize=atoi(tdfparser.SGetValueDef("1", "UNITINFO\\FootprintX").c_str())*2;//ta has only half our res so multiply size with 2
	ud.ysize=atoi(tdfparser.SGetValueDef("1", "UNITINFO\\FootprintZ").c_str())*2;

	ud.needGeo=false;
	if(ud.type=="Building" || ud.type=="Factory"){
		CreateYardMap(&ud, tdfparser.SGetValueDef("c", "UNITINFO\\YardMap"));
	} else {
		for (int u=0;u<4;u++)
			ud.yardmaps[u] = 0;
	}

	ud.leaveTracks=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\LeaveTracks").c_str());
	ud.trackWidth=atof(tdfparser.SGetValueDef("32", "UNITINFO\\TrackWidth").c_str());
	ud.trackOffset=atof(tdfparser.SGetValueDef("0", "UNITINFO\\TrackOffset").c_str());
	ud.trackStrength=atof(tdfparser.SGetValueDef("0", "UNITINFO\\TrackStrength").c_str());
	ud.trackStretch=atof(tdfparser.SGetValueDef("1", "UNITINFO\\TrackStretch").c_str());
	if(ud.leaveTracks && groundDecals)
		ud.trackType=groundDecals->GetTrackType(tdfparser.SGetValueDef("StdTank", "UNITINFO\\TrackType"));

	ud.useBuildingGroundDecal=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\UseBuildingGroundDecal").c_str());
	ud.buildingDecalSizeX=atoi(tdfparser.SGetValueDef("4", "UNITINFO\\BuildingGroundDecalSizeX").c_str());
	ud.buildingDecalSizeY=atoi(tdfparser.SGetValueDef("4", "UNITINFO\\BuildingGroundDecalSizeY").c_str());
	ud.buildingDecalDecaySpeed=atof(tdfparser.SGetValueDef("0.1", "UNITINFO\\BuildingGroundDecalDecaySpeed").c_str());
	if(ud.useBuildingGroundDecal && groundDecals)
		ud.buildingDecalType=groundDecals->GetBuildingDecalType(tdfparser.SGetValueDef("", "UNITINFO\\BuildingGroundDecalType"));

	ud.canDropFlare=!!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\CanDropFlare").c_str());
	ud.flareReloadTime=atof(tdfparser.SGetValueDef("5", "UNITINFO\\FlareReload").c_str());
	ud.flareEfficieny=atof(tdfparser.SGetValueDef("0.5", "UNITINFO\\FlareEfficiency").c_str());
	ud.flareDelay=atof(tdfparser.SGetValueDef("0.3", "UNITINFO\\FlareDelay").c_str());
	ud.flareDropVector=tdfparser.GetFloat3(ZeroVector,"UNITINFO\\FlareDropVector");
	ud.flareTime=atoi(tdfparser.SGetValueDef("3", "UNITINFO\\FlareTime").c_str())*30;
	ud.flareSalvoSize=atoi(tdfparser.SGetValueDef("4", "UNITINFO\\FlareSalvoSize").c_str());
	ud.flareSalvoDelay=atoi(tdfparser.SGetValueDef("0.1", "UNITINFO\\FlareSalvoDelay").c_str())*30;

	ud.smoothAnim = !!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\SmoothAnim").c_str());
	ud.canLoopbackAttack = !!atoi(tdfparser.SGetValueDef("0", "UNITINFO\\CanLoopbackAttack").c_str());
	ud.levelGround = !!atoi(tdfparser.SGetValueDef("1", "UNITINFO\\LevelGround").c_str());

	// aircraft collision sizes default to half their visual size, to
	// make them less likely to collide or get hit by nontracking weapons
	const char* defScale = ud.canfly ? "0.5" : "1";
	ud.collisionSphereScale = atof(tdfparser.SGetValueDef(defScale, "UNITINFO\\CollisionSphereScale").c_str());

	float3 cso = ZeroVector;
	const char* strCSOffset = tdfparser.SGetValueDef("default", "UNITINFO\\CollisionSphereOffset").c_str();
	if (sscanf(strCSOffset, "%f %f %f", &cso.x, &cso.y, &cso.z) == 3) {
		ud.useCSOffset = true;
		ud.collisionSphereOffset = cso;
	}
	else {
		ud.useCSOffset = false;
	}

	ud.seismicRadius=atoi(tdfparser.SGetValueDef("0", "UNITINFO\\seismicDistance").c_str());
	ud.seismicSignature=atoi(tdfparser.SGetValueDef("-1", "UNITINFO\\seismicSignature").c_str());
	if(ud.seismicSignature==-1)
	{
		if(!ud.floater && !ud.canhover && !ud.canfly)
			ud.seismicSignature = sqrt(ud.mass/(float)100);
		else
			ud.seismicSignature = 0;
	}

	if(tdfparser.SectionExist("UNITINFO\\SFXTypes"))
	{
		int num=0;
		while(num!=-1)
		{
			char cnum[16];
			SNPRINTF(cnum, sizeof(cnum), "%i", num);
			std::string expsfx = tdfparser.SGetValueDef("", "UNITINFO\\SFXTypes\\explosiongenerator" + std::string(cnum));
			if(expsfx!="")
			{
				ud.sfxExplGens.push_back(explGenHandler->LoadGenerator(expsfx));
				num++;
			}
			else
				num=-1;
		}

	}

	LoadSound(tdfparser, ud.sounds.ok, "ok1");
	LoadSound(tdfparser, ud.sounds.select, "select1");
	LoadSound(tdfparser, ud.sounds.arrived, "arrived1");
	LoadSound(tdfparser, ud.sounds.build, "build");
	LoadSound(tdfparser, ud.sounds.activate, "activate");
	LoadSound(tdfparser, ud.sounds.deactivate, "deactivate");
	LoadSound(tdfparser, ud.sounds.cant, "cant");
	LoadSound(tdfparser, ud.sounds.underattack, "underattack");
}


void CUnitDefHandler::LoadSound(TdfParser &tdfparser, GuiSound &gsound, std::string sunname)
{
	soundcategory.GetDef(gsound.name, "", tdfparser.SGetValueDef("", "UNITINFO\\SoundCategory")+"\\"+sunname);
	if(gsound.name.compare("")==0)
		gsound.id = 0;
	else
	{
		const string soundFile = "sounds/" + gsound.name + ".wav";
		CFileHandler file(soundFile);
		if(file.FileExists()) {
			PUSH_CODE_MODE;
			ENTER_UNSYNCED;
			int id = sound->GetWaveId(soundFile);
			POP_CODE_MODE;
			gsound.id = id;
		} else
			gsound.id = 0;
	}
	gsound.volume = 5.0f;
}


void CUnitDefHandler::ParseUnit(std::string file, int id)
{
  try {
    ParseTAUnit(file, id);
  } catch (content_error const& e) {
    std::cout << e.what() << std::endl;
    return;
  }

	unitDefs[id].valid = true;

	if (noCost) {
		unitDefs[id].metalCost = 1;
		unitDefs[id].energyCost = 1;
		unitDefs[id].buildTime = 10;
		unitDefs[id].metalUpkeep = 0;
		unitDefs[id].energyUpkeep = 0;
	}
}


UnitDef *CUnitDefHandler::GetUnitByName(std::string name)
{
	StringToLowerInPlace(name);

	std::map<std::string, int>::iterator it = unitID.find(name);
	if (it == unitID.end()) {
		return NULL;
	}

	const int id = it->second;
	if (!unitDefs[id].valid) {
		return NULL; // should not happen
	}

	return &unitDefs[id];
}


UnitDef *CUnitDefHandler::GetUnitByID(int id)
{
	if ((id <= 0) || (id > numUnits)) {
		return NULL;
	}
	UnitDef* ud = &unitDefs[id];
	if (!ud->valid) {
		return NULL;
	}
	return ud;
}

/*
Creates a open ground blocking map, called yardmap.
When sat != 0, is used instead of normal all-over-blocking.
*/
void CUnitDefHandler::CreateYardMap(UnitDef *def, std::string yardmapStr) {
	//Force string to lower case.
	StringToLowerInPlace(yardmapStr);

	//Creates the map.
	for (int u=0;u<4;u++)
		def->yardmaps[u] = SAFE_NEW unsigned char[def->xsize * def->ysize];

	unsigned char *originalMap = SAFE_NEW unsigned char[def->xsize * def->ysize / 4];		//TAS resolution is double of TA resolution.
	memset(originalMap, 1, def->xsize * def->ysize / 4);

	if(!yardmapStr.empty()){
		std::string::iterator si = yardmapStr.begin();
		int x, y;
		for(y = 0; y < def->ysize / 2; y++) {
			for(x = 0; x < def->xsize / 2; x++) {
				if(*si == 'g')
					def->needGeo=true;
				if(*si == YARDMAP_CHAR)
					originalMap[x + y*def->xsize/2] = 0;
				do {
					si++;
				} while(si != yardmapStr.end() && *si == ' ');
				if(si == yardmapStr.end())
					break;
			}
			if(si == yardmapStr.end())
				break;
		}
	}
	for(int y = 0; y < def->ysize; y++)
		for(int x = 0; x < def->xsize; x++){
			def->yardmaps[0][x + y*def->xsize] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[1][(def->ysize*def->xsize)-(def->ysize*(x+1)-(y+1)+1)] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[2][(def->ysize*def->xsize)-(x + y*def->xsize+1)] = originalMap[x/2 + y/2*def->xsize/2];
			def->yardmaps[3][def->ysize*(x+1)-(y+1)] = originalMap[x/2 + y/2*def->xsize/2];
		}
	delete[] originalMap;
}


unsigned int CUnitDefHandler::GetUnitImage(UnitDef *unitdef)
{
	if (unitdef->unitimage != 0) {
		return unitdef->unitimage;
	}

	CBitmap bitmap;
	if (!unitdef->buildpicname.empty()) {
		bitmap.Load("unitpics/" + unitdef->buildpicname);
	}
	else {
		//try pcx first and then bmp if no pcx exist
		CFileHandler bfile("unitpics/" + unitdef->name + ".pcx");
		if (bfile.FileExists()) {
			bitmap.Load("unitpics/" + unitdef->name + ".pcx");
		}
		else {
			CFileHandler bfile("unitpics/" + unitdef->name + ".bmp");
			if (bfile.FileExists()){
				bitmap.Load("unitpics/" + unitdef->name + ".bmp");
			} else {
				bitmap.Alloc(1, 1); // last resort
			}
		}
	}

	const unsigned int texID = bitmap.CreateTexture(false);
	PUSH_CODE_MODE;
	ENTER_SYNCED;
	unitdef->unitimage  = texID;
	unitdef->imageSizeX = bitmap.xsize;
	unitdef->imageSizeY = bitmap.ysize;
	POP_CODE_MODE;

	return unitdef->unitimage;
}


void CUnitDefHandler::AssignTechLevels()
{
	set<int>::iterator it;
	for (it = commanderIDs.begin(); it != commanderIDs.end(); ++it) {
		AssignTechLevel(unitDefs[*it], 0);
	}
}


void CUnitDefHandler::AssignTechLevel(UnitDef& ud, int level)
{
	if ((ud.techLevel >= 0) && (ud.techLevel <= level)) {
		return;
	}

	ud.techLevel = level;

	level++;

	map<int, std::string>::const_iterator bo_it;
	for (bo_it = ud.buildOptions.begin(); bo_it != ud.buildOptions.end(); ++bo_it) {
		std::map<std::string, int>::const_iterator ud_it = unitID.find(bo_it->second);
		if (ud_it != unitID.end()) {
			AssignTechLevel(unitDefs[ud_it->second], level);
		}
	}
}


bool CUnitDefHandler::SaveTechLevels(const std::string& filename,
                                     const std::string& modname)
{
	FILE* f;
	if (filename.empty()) {
		f = stdout;
	} else {
		f = fopen(filename.c_str(), "wt");
		if (f == NULL) {
			return false;
		}
	}

	fprintf(f, "\nTech Levels for \"%s\"\n", modname.c_str());
	multimap<int, string> entries;
	std::map<std::string, int>::const_iterator uit;
	for (uit = unitID.begin(); uit != unitID.end(); uit++) {
		const string& unitName = uit->first;
		const UnitDef* ud = GetUnitByName(unitName);
		if (ud) {
			char buf[256];
			SNPRINTF(buf, sizeof(buf), " %3i:  %-15s  // %s :: %s\n",
							 ud->techLevel, unitName.c_str(),
							 ud->humanName.c_str(), ud->tooltip.c_str());
			entries.insert(pair<int, string>(ud->techLevel, buf));
		}
	}
	int prevLevel = -2;
	multimap<int, string>::iterator eit;
	for (eit = entries.begin(); eit != entries.end(); ++eit) {
		if (eit->first != prevLevel) {
			fprintf(f, "\n");
			prevLevel = eit->first;
		}
		fprintf(f, "%s", eit->second.c_str());
	}

	if (f != stdout) {
		fclose(f);
	}
	return true;
}


UnitDef::~UnitDef()
{
}
