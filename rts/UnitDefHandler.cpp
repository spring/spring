#include "StdAfx.h"
#include "UnitDefHandler.h"
#include "myGL.h"
#include "SunParser.h"
#include <algorithm>
#include <locale>
#include <cctype>
#include "FileHandler.h"
#include "Bitmap.h"
#include "InfoConsole.h"
#include "CategoryHandler.h"
#include "Sound.h"
#include "WeaponDefHandler.h"
#include "DamageArrayHandler.h"
#include "UnitDef.h"
#include "ReadMap.h"
#include "GameSetup.h"
//#include "mmgr.h"


const char YARDMAP_CHAR = 'c';		//Need to be low case.


CUnitDefHandler* unitDefHandler;

CUnitDefHandler::CUnitDefHandler(void)
{
	noCost=false;
      	std::string dir = "units/";

	PrintLoadMsg("Loading units and weapons");

	weaponDefHandler=new CWeaponDefHandler();

	numUnits = 0;
	
	std::vector<std::string> tafiles = CFileHandler::FindFiles("units/*.fbi");
	std::vector<std::string> tafiles2 = CFileHandler::FindFiles("units/*.swu");
	while(!tafiles2.empty()){
		tafiles.push_back(tafiles2.back());
		tafiles2.pop_back();
	}

      	soundcategory.LoadFile("gamedata/SOUND.TDF");
	
	numUnits = tafiles.size();

	// This could be wasteful if there is a lot of restricted units, but that is not that likely
	unitDefs = new UnitDef[numUnits+1];
	
	unsigned int i = 1;		// Start at unit id 1
	for(unsigned int a = 0; a < tafiles.size(); ++a)	
	{
		// Determine the name (in lowercase) first
		int len = tafiles[a].find_last_of("/")+1;
		std::string unitname = tafiles[a].substr(len, tafiles[a].size()-4-len);
		std::transform(unitname.begin(), unitname.end(), unitname.begin(), (int (*)(int))std::tolower);

		// Restrictions may tell us not to use this unit at all
		if (gameSetup)
			if (gameSetup->restrictedUnits.find(unitname) != gameSetup->restrictedUnits.end()) {
				//info->AddLine("Not using unit %s", unitname.c_str());
				continue;
			}

		// Seems ok, load it
		unitDefs[i].loaded = false;
		unitDefs[i].filename = tafiles[a];
		unitDefs[i].id = i;			
		unitDefs[i].yardmap = 0;
		unitDefs[i].name = unitname;
		unitID[unitname] = i;

		// Increase index for next unit
		i++;
	}

	FindTABuildOpt();
}

CUnitDefHandler::~CUnitDefHandler(void)
{
	//Deleting eventual yeardmaps.
	for(int i = 1; i <= numUnits; i++){
		if(unitDefs[i].yardmap != 0) {
			delete[] unitDefs[i].yardmap;
			unitDefs[i].yardmap = 0;
		}
		glDeleteTextures(1,&unitDefs[i].unitimage);
	}
	delete[] unitDefs;
	delete weaponDefHandler;
}

void CUnitDefHandler::FindTABuildOpt()
{
	CSunParser sunparser;
      	sunparser.LoadFile("gamedata/SIDEDATA.TDF");

	std::vector<std::string> sideunits = sunparser.GetSectionList("CANBUILD");
	for(unsigned int i=0; i<sideunits.size(); i++)
	{
		std::map<std::string, std::string>::iterator it;

		UnitDef *builder=NULL;
		std::transform(sideunits[i].begin(), sideunits[i].end(), sideunits[i].begin(), (int (*)(int))std::tolower);
		std::map<std::string, int>::iterator it1 = unitID.find(sideunits[i]);
		if(it1!= unitID.end())
			builder = &unitDefs[it1->second];

		if(builder)
		{
			std::map<std::string, std::string> buildoptlist = sunparser.GetAllValues("CANBUILD\\" + sideunits[i]);
			for(it=buildoptlist.begin(); it!=buildoptlist.end(); it++)
			{
				UnitDef *buildopt=0;
				std::transform(it->second.begin(),it->second.end(), it->second.begin(), (int (*)(int))std::tolower);

				if(unitID.find(it->second)!= unitID.end()){
					int num=atoi(it->first.substr(8).c_str());
					builder->buildOptions[num]=it->second;
				}
			}
		}
	}

	std::vector<std::string> files = CFileHandler::FindFiles("download/*.tdf");
	for(unsigned int i=0; i<files.size(); i++)
	{
		CSunParser dparser;
		dparser.LoadFile(files[i]);

		std::vector<std::string> sectionlist = dparser.GetSectionList("");

		for(unsigned int j=0; j<sectionlist.size(); j++)
		{
			UnitDef *builder=NULL;
			std::string un1 = dparser.SGetValueDef("", sectionlist[j] + "\\UNITMENU");
			std::transform(un1.begin(), un1.end(), un1.begin(), (int (*)(int))std::tolower);
			std::map<std::string, int>::iterator it1 = unitID.find(un1);
			if(it1!= unitID.end())
				builder = &unitDefs[it1->second];

			if(builder)
			{
				UnitDef *buildopt=NULL;
				string un2 = dparser.SGetValueDef("", sectionlist[j] + "\\UNITNAME");
				std::transform(un2.begin(), un2.end(), un2.begin(), (int (*)(int))std::tolower);

				if(unitID.find(un2)!= unitID.end()){
					int menu=atoi(dparser.SGetValueDef("", sectionlist[j] + "\\MENU").c_str());
					int button=atoi(dparser.SGetValueDef("", sectionlist[j] + "\\BUTTON").c_str());
					int num=(menu-2)*6+button+1;
					builder->buildOptions[num]=un2;
				}
			}
		}

	}
}

void CUnitDefHandler::ParseTAUnit(std::string file, int id)
{
	CSunParser sunparser;
	sunparser.LoadFile(file);

	UnitDef& ud=unitDefs[id];

	ud.name = sunparser.SGetValueMSG("UNITINFO\\UnitName");
	ud.humanName = sunparser.SGetValueMSG("UNITINFO\\name");

	sunparser.GetDef(ud.extractsMetal, "0", "UNITINFO\\ExtractsMetal");
	sunparser.GetDef(ud.windGenerator, "0", "UNITINFO\\WindGenerator");
	sunparser.GetDef(ud.tidalGenerator, "0", "UNITINFO\\TidalGenerator");

	ud.health=atof(sunparser.SGetValueDef("0", "UNITINFO\\MaxDamage").c_str());
	ud.metalUpkeep=atof(sunparser.SGetValueDef("0", "UNITINFO\\MetalUse").c_str());
	ud.energyUpkeep=atof(sunparser.SGetValueDef("0", "UNITINFO\\EnergyUse").c_str());
	ud.metalMake=atof(sunparser.SGetValueDef("0", "UNITINFO\\MetalMake").c_str());
	ud.makesMetal=atof(sunparser.SGetValueDef("0", "UNITINFO\\MakesMetal").c_str());
	ud.energyMake=atof(sunparser.SGetValueDef("0", "UNITINFO\\EnergyMake").c_str());
	ud.metalStorage=atof(sunparser.SGetValueDef("0", "UNITINFO\\MetalStorage").c_str());
	ud.energyStorage=atof(sunparser.SGetValueDef("0", "UNITINFO\\EnergyStorage").c_str());

	ud.controlRadius=32;
	ud.losHeight=20;
	ud.metalCost=atof(sunparser.SGetValueDef("0", "UNITINFO\\BuildCostMetal").c_str());
	ud.mass=ud.metalCost;
	ud.energyCost=atof(sunparser.SGetValueDef("0", "UNITINFO\\BuildCostEnergy").c_str());
	ud.buildTime=atof(sunparser.SGetValueDef("0", "UNITINFO\\BuildTime").c_str());
	ud.aihint=id;		//fix
	ud.losRadius=atof(sunparser.SGetValueDef("0", "UNITINFO\\SightDistance").c_str());
	ud.airLosRadius=atof(sunparser.SGetValueDef("0", "UNITINFO\\SightDistance").c_str())*2;
	ud.tooltip=sunparser.SGetValueDef(ud.name,"UNITINFO\\Description");
	ud.moveType=0;
	
	sunparser.GetDef(ud.canfly, "0", "UNITINFO\\canfly");
	sunparser.GetDef(ud.canmove, "0", "UNITINFO\\canmove");
	sunparser.GetDef(ud.builder, "0", "UNITINFO\\Builder");
	sunparser.GetDef(ud.upright, "0", "UNITINFO\\Upright");
	sunparser.GetDef(ud.onoffable, "0", "UNITINFO\\onoffable");

	sunparser.GetDef(ud.maxSlope, "0", "UNITINFO\\MaxSlope");
	ud.maxHeightDif=40*tan(ud.maxSlope*(PI/180));
	ud.maxSlope = cos(ud.maxSlope*(PI/180));
	sunparser.GetDef(ud.minWaterDepth, "-10e6", "UNITINFO\\MinWaterDepth");
	sunparser.GetDef(ud.maxWaterDepth, "10e6", "UNITINFO\\MaxWaterDepth");
	std::string value;
	ud.floater = sunparser.SGetValue(value, "UNITINFO\\Waterline");
	sunparser.GetDef(ud.waterline, "0", "UNITINFO\\Waterline");

	sunparser.GetDef(ud.selfDCountdown, "5", "UNITINFO\\selfdestructcountdown");

	ud.speed=atof(sunparser.SGetValueDef("0", "UNITINFO\\MaxVelocity").c_str())*30;
	ud.maxAcc=atof(sunparser.SGetValueDef("0.5", "UNITINFO\\acceleration").c_str())*0.1;
	ud.maxDec=atof(sunparser.SGetValueDef("0.5", "UNITINFO\\BrakeRate").c_str())*0.1;
	ud.turnRate=atof(sunparser.SGetValueDef("0", "UNITINFO\\TurnRate").c_str());
	ud.buildSpeed=atof(sunparser.SGetValueDef("0", "UNITINFO\\WorkerTime").c_str());
	ud.buildDistance=atof(sunparser.SGetValueDef("64", "UNITINFO\\Builddistance").c_str());
	ud.buildDistance=max(128.f,ud.buildDistance);
	ud.armoredMultiple=atof(sunparser.SGetValueDef("0.0001", "UNITINFO\\DamageModifier").c_str());
	ud.armorType=damageArrayHandler->GetTypeFromName(ud.name);
//	info->AddLine("unit %s has armor %i",ud.name.c_str(),ud.armorType);

	ud.radarRadius=atoi(sunparser.SGetValueDef("0", "UNITINFO\\RadarDistance").c_str());
	ud.sonarRadius=atoi(sunparser.SGetValueDef("0", "UNITINFO\\SonarDistance").c_str());
	ud.jammerRadius=atoi(sunparser.SGetValueDef("0", "UNITINFO\\RadarDistanceJam").c_str());
	ud.stealth=!!atoi(sunparser.SGetValueDef("0", "UNITINFO\\Stealth").c_str());
	ud.targfac=!!atoi(sunparser.SGetValueDef("0", "UNITINFO\\istargetingupgrade").c_str());
	ud.isFeature=!!atoi(sunparser.SGetValueDef("0", "UNITINFO\\IsFeature").c_str());

	ud.cloakCost=atof(sunparser.SGetValueDef("-1", "UNITINFO\\CloakCost").c_str());
	ud.cloakCostMoving=atof(sunparser.SGetValueDef("-1", "UNITINFO\\CloakCostMoving").c_str());
	if(ud.cloakCostMoving<0)
		ud.cloakCostMoving=ud.cloakCost;
	if(ud.cloakCost>=0)
		ud.canCloak=true;
	else
		ud.canCloak=false;
	ud.startCloaked=!!atoi(sunparser.SGetValueDef("0", "UNITINFO\\init_cloaked").c_str());
	ud.decloakDistance=atof(sunparser.SGetValueDef("-1", "UNITINFO\\mincloakdistance").c_str());
	
	ud.canKamikaze=!!atoi(sunparser.SGetValueDef("0", "UNITINFO\\kamikaze").c_str());
	ud.kamikazeDist=atof(sunparser.SGetValueDef("0", "UNITINFO\\kamikazedistance").c_str())+25; //we count 3d distance while ta count 2d distance so increase slightly

	sunparser.GetDef(ud.canfly, "0", "UNITINFO\\canfly");
	sunparser.GetDef(ud.canmove, "0", "UNITINFO\\canmove");
	sunparser.GetDef(ud.canhover, "0", "UNITINFO\\canhover");
	if(sunparser.SGetValue(value, "UNITINFO\\floater"))
		sunparser.GetDef(ud.floater, "0", "UNITINFO\\floater");
	sunparser.GetDef(ud.builder, "0", "UNITINFO\\Builder");

	if(ud.builder && !ud.buildSpeed)		//core anti is flagged as builder for some reason
		ud.builder=false;

	sunparser.GetDef(ud.transportSize, "0", "UNITINFO\\transportsize");
	sunparser.GetDef(ud.transportCapacity, "0", "UNITINFO\\transportcapacity");
	ud.loadingRadius=220;

	ud.wingDrag=0.07;			//drag caused by wings
	ud.wingAngle=0.08;			//angle between front and the wing plane
	ud.drag=0.005;					//how fast the aircraft loses speed;
	ud.frontToSpeed=0.1;	//fudge factor for lining up speed and front of plane
	ud.speedToFront=0.07;	//fudge factor for lining up speed and front of plane
	ud.myGravity=0.4;		//planes are slower than real airplanes so lower gravity to compensate

	ud.maxBank=0.8;				//max roll
	ud.maxPitch=0.45;			//max pitch this plane tries to keep
	ud.turnRadius=500;			//hint to the ai about how large turn radius this plane needs
//	ud.wantedHeight=400;		//height about ground this plane tries to keep

	ud.maxAcc=0.075;				//engine power
	ud.maxAileron=0.015;		//turn speed around roll axis
	ud.maxElevator=0.01;		//turn speed around pitch axis
	ud.maxRudder=0.004;			//turn speed around yaw axis


	ud.category=CCategoryHandler::Instance()->GetCategories(sunparser.SGetValueDef("", "UNITINFO\\Category"));
	ud.noChaseCategory=CCategoryHandler::Instance()->GetCategories(sunparser.SGetValueDef("", "UNITINFO\\NoChaseCategory"));
//	info->AddLine("Unit %s has cat %i",ud.humanName.c_str(),ud.category);

	std::string weap1;
	sunparser.GetDef(weap1, "", std::string("UNITINFO\\Weapon1"));
	std::string weap2;
	sunparser.GetDef(weap2, "", std::string("UNITINFO\\Weapon2"));
	std::string weap3;
	sunparser.GetDef(weap3, "", std::string("UNITINFO\\Weapon3"));

	WeaponDef *wd1 = weaponDefHandler->GetWeapon(weap1);
	WeaponDef *wd2 = weaponDefHandler->GetWeapon(weap2);
	WeaponDef *wd3 = weaponDefHandler->GetWeapon(weap3);

	if(wd1)
	{
		wd1->badTargetCategory=0;
		string badTarget;
		sunparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "wpri_badTargetCategory");
		if(!badTarget.empty())
			wd1->badTargetCategory=CCategoryHandler::Instance()->GetCategories(badTarget);

		ud.sweapons.push_back(weap1);
		ud.weapons.push_back(wd1);
	}
	else
	{
		if(wd2||wd3)
		{
			ud.sweapons.push_back("");
			ud.weapons.push_back(NULL);
		}

	}
	if(wd2)
	{
		wd2->badTargetCategory=0;
		string badTarget;
		sunparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "wsec_badTargetCategory");
		if(!badTarget.empty())
			wd2->badTargetCategory=CCategoryHandler::Instance()->GetCategories(badTarget);

		ud.sweapons.push_back(weap2);
		ud.weapons.push_back(wd2);
	}
	else
	{
		if(wd3)
		{
			ud.sweapons.push_back("");
			ud.weapons.push_back(NULL);
		}
	}
	if(wd3)
	{
		wd3->badTargetCategory=0;
		string badTarget;
		sunparser.GetDef(badTarget, "", std::string("UNITINFO\\") + "wspe_badTargetCategory");
		if(!badTarget.empty())
			wd3->badTargetCategory=CCategoryHandler::Instance()->GetCategories(badTarget);

		ud.sweapons.push_back(weap3);
		ud.weapons.push_back(wd3);
	}

	sunparser.GetDef(ud.canDGun, "0", "UNITINFO\\candgun");

	ud.wantedHeight=atof(sunparser.SGetValueDef("0", "UNITINFO\\cruisealt").c_str())*1.5;
	ud.hoverAttack = !!atoi(sunparser.SGetValueDef("0", "UNITINFO\\hoverattack").c_str());

	string TEDClass=sunparser.SGetValueDef("0", "UNITINFO\\TEDClass").c_str();
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
	else if(ud.canfly)
	{
		if(!ud.weapons.empty() && ud.weapons[0]!=0 && (ud.weapons[0]->type=="AircraftBomb" || ud.weapons[0]->type=="TorpedoLauncher")){
			ud.type = "Bomber";			
			ud.turnRadius=800;			//hint to the ai about how large turn radius this plane needs

		} else {
			ud.type = "Fighter";
		}
		ud.maxAcc=max(ud.maxAcc*0.6,0.065);
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
		string moveclass=sunparser.SGetValueDef("", "UNITINFO\\MovementClass");
		ud.movedata=moveinfo->GetMoveDataFromName(moveclass);
		if(ud.movedata->moveType==MoveData::Hover_Move || ud.movedata->moveType==MoveData::Ship_Move){
			ud.upright=true;
		}
		if(ud.canhover){
			if(ud.movedata->moveType!=MoveData::Hover_Move){
				info->AddLine("Inconsistant move data hover %i %s %s",ud.movedata->pathType,ud.humanName.c_str(),moveclass.c_str());
			}
		} else if(ud.floater){
			if(ud.movedata->moveType!=MoveData::Ship_Move){
				info->AddLine("Inconsistant move data ship %i %s %s",ud.movedata->pathType,ud.humanName.c_str(),moveclass.c_str());
			}
		} else {
			if(ud.movedata->moveType!=MoveData::Ground_Move){
				info->AddLine("Inconsistant move data ground %i %s %s",ud.movedata->pathType,ud.humanName.c_str(),moveclass.c_str());
			}
		}
//		info->AddLine("%s uses movetype %i",ud.humanName.c_str(),ud.movedata->pathType);
	}

	if(ud.maxAcc!=0 && ud.speed!=0)
		ud.drag=1.0/(ud.speed/GAME_SPEED*1.1/ud.maxAcc)-ud.wingAngle*ud.wingAngle*ud.wingDrag;

	std::string objectname;
	sunparser.GetDef(objectname, "", "UNITINFO\\Objectname");
	ud.model.modelpath = "objects3d/" + objectname + ".3do";
	ud.model.modelname = objectname;

	sunparser.GetDef(ud.wreckName, "", "UNITINFO\\Corpse");
	sunparser.GetDef(ud.deathExplosion, "", "UNITINFO\\ExplodeAs");
	sunparser.GetDef(ud.selfDExplosion, "", "UNITINFO\\SelfDestructAs");

	CBitmap bitmap("unitpics/" + ud.name + ".pcx");
	ud.unitimage = bitmap.CreateTexture(false);

	ud.power = (ud.metalCost + ud.energyCost/60.0f);

	sunparser.GetDef(ud.activateWhenBuilt, "0", "UNITINFO\\ActivateWhenBuilt");
	
	ud.xsize=atoi(sunparser.SGetValueDef("1", "UNITINFO\\FootprintX").c_str())*2;//ta has only half our res so multiply size with 2
	ud.ysize=atoi(sunparser.SGetValueDef("1", "UNITINFO\\FootprintZ").c_str())*2;

	ud.needGeo=false;
	if(ud.type=="Building" || ud.type=="Factory"){
//		CreateBlockingLevels(&ud,sunparser.SGetValueDef("c", "UNITINFO\\YardMap"));
		CreateYardMap(&ud, sunparser.SGetValueDef("c", "UNITINFO\\YardMap"));
	} else {
		ud.yardmap = 0;
	}


	LoadSound(sunparser, ud.sounds.ok, "ok1");
	LoadSound(sunparser, ud.sounds.select, "select1");
	LoadSound(sunparser, ud.sounds.arrived, "arrived1");
	LoadSound(sunparser, ud.sounds.build, "build");
	LoadSound(sunparser, ud.sounds.activate, "activate");
	LoadSound(sunparser, ud.sounds.deactivate, "deactivate");
	LoadSound(sunparser, ud.sounds.cant, "cant");
}

void CUnitDefHandler::LoadSound(CSunParser &sunparser, GuiSound &gsound, std::string sunname)
{
	soundcategory.GetDef(gsound.name, "", sunparser.SGetValueDef("", "UNITINFO\\SoundCategory")+"\\"+sunname);
	if(gsound.name.compare("")==0)
		gsound.id = 0;
	else
	{
		CFileHandler file("sounds/"+gsound.name+".wav");
		if(file.FileExists())
			gsound.id = sound->GetWaveId(gsound.name+".wav");
		else
			gsound.id = 0;
	}
	gsound.volume = 5.0f;
}

void CUnitDefHandler::ParseUnit(std::string file, int id)
{
/*	switch(unitDefs[id].unitsourcetype)
	{
	case TA_UNIT:*/
		ParseTAUnit(file, id);
/*		break;
	}
*/
	unitDefs[id].loaded = true;

	if(noCost){
		unitDefs[id].metalCost = 1;
		unitDefs[id].energyCost = 1;
		unitDefs[id].buildTime = 10;
		unitDefs[id].metalUpkeep = 0;
		unitDefs[id].energyUpkeep = 0;
	}
}

UnitDef *CUnitDefHandler::GetUnitByName(std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), (int (*)(int))std::tolower);

	std::map<std::string, int>::iterator it = unitID.find(name);
	if(it == unitID.end())
		return NULL;

	int id = it->second;
	if(!unitDefs[id].loaded)
		ParseUnit(unitDefs[id].filename, id);

	return &unitDefs[id];
}

UnitDef *CUnitDefHandler::GetUnitByID(int id)
{	
	if(!unitDefs[id].loaded)
		ParseUnit(unitDefs[id].filename, id);

	return &unitDefs[id];
}

/*
void CUnitDefHandler::CreateBlockingLevels(UnitDef *def,std::string yardmap)
{
	std::transform(yardmap.begin(),yardmap.end(), yardmap.begin(), (int (*)(int))std::tolower);
	
	def->yardmapLevels[0]=new unsigned char[(def->xsize)*(def->ysize)];
	for(int a=1;a<=4;++a){
		def->yardmapLevels[a]=new unsigned char[(def->xsize+a*2-1)*(def->ysize+a*2-1)];
	}
	memset(def->yardmapLevels[0],1,def->xsize*def->ysize);

	unsigned char* orgLevel=new unsigned char[def->xsize*def->ysize/4];
	memset(orgLevel,1,def->xsize*def->ysize/4);

	std::string::iterator si=yardmap.begin();
	for(int y=0;y<def->ysize/2;++y){
		for(int x=0;x<def->xsize/2;++x){
			if(*si=='c'){
				orgLevel[y*def->xsize/2+x]=0;
			}
			++si;
			if(si==yardmap.end())
				break;
			if(*si==' ')
				si++;
			if(si==yardmap.end())
				break;
		}
		if(si==yardmap.end())
			break;
	}
	for(int y=0;y<def->ysize;++y){
		for(int x=0;x<def->xsize;++x){
			def->yardmapLevels[0][y*def->xsize+x]=orgLevel[y/2*def->xsize/2+x/2];
		}
	}

	for(int a=1;a<=4;++a){
		int step=2;
		if(a==1)
			step=1;
		for(int y=0;y<def->ysize+a*2-1;++y){
			int maxPrevY=def->ysize+a*2-1-step;
			for(int x=0;x<def->xsize+a*2-1;++x){
				int maxPrevX=def->xsize+a*2-1-step;
				if(def->yardmapLevels[a-1][min(maxPrevY-1,y)*maxPrevX+min(maxPrevX-1,x)] || 
					 def->yardmapLevels[a-1][min(maxPrevY-1,y)*maxPrevX+max(0,x-step)] || 
					 def->yardmapLevels[a-1][max(0,y-step)*maxPrevX+min(maxPrevX-1,x)] || 
					 def->yardmapLevels[a-1][max(0,y-step)*maxPrevX+max(0,x-step)]){
						def->yardmapLevels[a][y*(def->xsize+a*2-1)+x]=1;
					 } else {
						def->yardmapLevels[a][y*(def->xsize+a*2-1)+x]=0;
					 }
			}
		}
	}
}
*/

/*
Creates a open ground blocking map, called yardmap.
When sat != 0, is used instead of normal all-over-blocking.
*/
void CUnitDefHandler::CreateYardMap(UnitDef *def, std::string yardmapStr) {
	//Force string to lower case.
	std::transform(yardmapStr.begin(), yardmapStr.end(), yardmapStr.begin(), (int(*)(int))std::tolower);

	//Creates the map.
	def->yardmap = new unsigned char[def->xsize * def->ysize];
	memset(def->yardmap, 1, def->xsize * def->ysize);	//Q: Needed?

	unsigned char *originalMap = new unsigned char[def->xsize * def->ysize / 4];		//TAS resolution is double of TA resolution.
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
		for(int x = 0; x < def->xsize; x++)
			def->yardmap[x + y*def->xsize] = originalMap[x/2 + y/2*def->xsize/2];

	delete[] originalMap;
}

