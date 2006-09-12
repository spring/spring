// Generalized callback interface - shared between global AI and group AI
#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "Net.h"
#include "GlobalAI.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/LosHandler.h"
#include "LogOutput.h"
#include "Game/UI/MiniMap.h"
#include "Group.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Game/Team.h"
#include "Sim/Misc/QuadField.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Game/GameHelper.h"
#include "Sim/Units/UnitDefHandler.h"
#include "GroupHandler.h"
#include "GlobalAIHandler.h"
#include "Sim/Misc/Feature.h"
#include "Sim/Misc/FeatureHandler.h"
#include "IGroupAI.h"
#include "Sim/Path/PathManager.h"
#include "AICheats.h"
#include "Game/GameSetup.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/Wind.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Game/Player.h"
#include "Rendering/InMapDraw.h"
#include "FileSystem/FileHandler.h"
#include "Rendering/InMapDraw.h"
#include "FileSystem/FileHandler.h"
#include "Platform/ConfigHandler.h"
#include "Platform/errorhandler.h"
#include "Game/UI/MouseHandler.h"
#include "Game/Camera.h"
#include "Game/CameraController.h"
#include "Sim/ModInfo.h"
#include "Platform/FileSystem.h"
#include "mmgr.h"

/* Cast id to unsigned to catch negative ids in the same operations,
cast MAX_* to unsigned to suppress GCC comparison between signed/unsigned warning. */
#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)MAX_UNITS)
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())
#define CHECK_FEATURE(id) ((unsigned)(id) < (unsigned)MAX_FEATURES)
/* With some hacking you can raise an abort (assert) instead of ignoring the id, */
//#define CHECK_UNITID(id) (assert(id > 0 && id < MAX_UNITS), true)
/* ...or disable the check altogether for release... */
//#define CHECK_UNITID(id) true

CAICallback::CAICallback(int Team, CGroupHandler *ghandler)
: team(Team), noMessages(false), gh(ghandler), group (0)
{}

CAICallback::~CAICallback(void)
{}

void CAICallback::SendTextMsg(const char* text,int priority)
{
	if (group)
		logOutput.Print(priority, "Group%i: %s",group->id,text);
	else
        logOutput.Print(priority, "GlobalAI%i: %s",team,text);
}
void CAICallback::SetLastMsgPos(float3 pos)
{
	logOutput.SetLastMsgPos(pos);
}

void CAICallback::AddNotification(float3 pos, float3 color, float alpha)
{
	minimap->AddNotification(pos,color,alpha);
}
bool CAICallback::PosInCamera(float3 pos, float radius)
{
	return camera->InView(pos,radius);
}

// see if the AI hasn't modified any parts of this callback
// (still completely insecure ofcourse, but it filters out the easiest way of cheating)
void CAICallback::verify()
{
	CGlobalAI *gai = globalAI->ais [team];
	if (gai && (((group && group->handler != gai->gh && group->handler != grouphandler) || gai->team != team))) {
		handleerror (0, "AI has modified spring components(possible cheat)", "Spring is closing:", MBF_OK | MBF_EXCL);
		exit (-1);
	}
}

int CAICallback::GetCurrentFrame()
{
	return gs->frameNum;
}

int CAICallback::GetMyTeam()
{
	return team;
}

int CAICallback::GetMyAllyTeam()
{
	return gs->AllyTeam(team);
}

int CAICallback::GetPlayerTeam(int player)
{
	CPlayer *pl = gs->players [player];
	if (pl->spectator)
		return -1;
	return pl->team;
}

void* CAICallback::CreateSharedMemArea(char* name, int size)
{
	return globalAI->GetAIBuffer(team,name,size);
}

void CAICallback::ReleasedSharedMemArea(char* name)
{
	globalAI->ReleaseAIBuffer(team,name);
}

int CAICallback::CreateGroup(char* dll, unsigned aiNumber)
{
	AIKey key;
	key.dllName=dll;
	key.aiNumber=aiNumber;
	CGroup* g=gh->CreateNewGroup(key);
	return g->id;
}

void CAICallback::EraseGroup(int groupid)
{
	if (CHECK_GROUPID(groupid)) {
		if(gh->groups[groupid])
			gh->RemoveGroup(gh->groups[groupid]);
	}
}

bool CAICallback::AddUnitToGroup(int unitid,int groupid)
{
	if (CHECK_UNITID(unitid) && CHECK_GROUPID(groupid)) {
		CUnit* u=uh->units[unitid];
		if(u && u->team==team && gh->groups[groupid]){
			return u->SetGroup(gh->groups[groupid]);
		}
	}
	return false;
}

bool CAICallback::RemoveUnitFromGroup(int unitid)
{
	if (CHECK_UNITID(unitid)) {
		CUnit* u=uh->units[unitid];
		if(u && u->team==team){
			u->SetGroup(0);
			return true;
		}
	}
	return false;
}

int CAICallback::GetUnitGroup(int unitid)
{
	if (CHECK_UNITID(unitid)) {
		CUnit *unit = uh->units[unitid];
		if (unit && unit->team == team) {
			CGroup* g=uh->units[unitid]->group;
			if(g)
				return g->id;
		}
	}
	return -1;
}

const std::vector<CommandDescription>* CAICallback::GetGroupCommands(int groupid)
{
	static std::vector<CommandDescription> tempcmds;

	if (CHECK_GROUPID(groupid)) {
		if(gh->groups[groupid] && gh->groups[groupid]->ai)
			return &gh->groups[groupid]->ai->GetPossibleCommands();
	}
	return &tempcmds;
}

int CAICallback::GiveGroupOrder(int groupid, Command* c)
{
	if (CHECK_GROUPID(groupid) && c != NULL) {
		if(gh->groups[groupid] && gh->groups[groupid]->ai)
			gh->groups[groupid]->ai->GiveCommand(c);
	}
	return 0;
}

int CAICallback::GiveOrder(int unitid, Command* c)
{
	verify ();

	if (!CHECK_UNITID(unitid) || c == NULL)
		return -1;

	if (noMessages)
		return -1;

	CUnit *unit = uh->units[unitid];

	if (!unit)
		return -1;

	if (group && unit->group != group)
		return -1;

	if (unit->team != team)
		return -1;

	net->SendSTLData<unsigned char, short, int, unsigned char, std::vector<float> >(
			NETMSG_AICOMMAND, gu->myPlayerNum, unitid, c->id, c->options, c->params);
	return 0;
}

const vector<CommandDescription>* CAICallback::GetUnitCommands(int unitid)
{
	if (CHECK_UNITID(unitid)) {
		CUnit *unit = uh->units[unitid];
		if (unit && unit->team == team)
			return &unit->commandAI->possibleCommands;
	}
	return 0;
}

const deque<Command>* CAICallback::GetCurrentUnitCommands(int unitid)
{
	if (CHECK_UNITID(unitid)) {
		CUnit *unit = uh->units[unitid];
		if (unit && unit->team == team)
			return &unit->commandAI->commandQue;
	}
	return 0;
}

int CAICallback::GetUnitAiHint(int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->aihint;
		}
	}
	return 0;
}

int CAICallback::GetUnitTeam(int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->team;
		}
	}
	return 0;	
}

int CAICallback::GetUnitAllyTeam(int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->allyteam;
		}
	}
	return 0;	
}

float CAICallback::GetUnitHealth(int unitid)			//the units current health
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->health;
		}
	}
	return 0;
}

float CAICallback::GetUnitMaxHealth(int unitid)		//the units max health
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->maxHealth;
		}
	}
	return 0;
}

float CAICallback::GetUnitSpeed(int unitid)				//the units max speed
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->unitDef->speed;
		}
	}
	return 0;
}

float CAICallback::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->power;
		}
	}
	return 0;
}

float CAICallback::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->experience;
		}
	}
	return 0;
}

float CAICallback::GetUnitMaxRange(int unitid)		//the furthest any weapon of the unit can fire
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->maxRange;
		}
	}
	return 0;
}

const UnitDef* CAICallback::GetUnitDef(int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->unitDef;
		}
	}
	return 0;
}

const UnitDef* CAICallback::GetUnitDef(const char* unitName)
{
	return unitDefHandler->GetUnitByName(unitName);
}

float3 CAICallback::GetUnitPos(int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & (LOS_INLOS|LOS_INRADAR))){
			return helper->GetUnitErrorPos(unit,gs->AllyTeam(team));
		}
	}
	return ZeroVector;
}

int CAICallback::GetBuildingFacing(int unitid) {
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->buildFacing;
		}
	}
	return 0;
}

bool CAICallback::IsUnitCloaked(int unitid) {
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->isCloaked;
		}
	}
	return false;
}

bool CAICallback::IsUnitParalyzed(int unitid){
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			return unit->stunned;
		}
	}
	return 0;
} 

int CAICallback::InitPath(float3 start,float3 end,int pathType)
{
	return pathManager->RequestPath(moveinfo->moveData.at(pathType),start,end);
}

float3 CAICallback::GetNextWaypoint(int pathid)
{
	return pathManager->NextWaypoint(pathid,ZeroVector);
}

void CAICallback::FreePath(int pathid)
{
	pathManager->DeletePath(pathid);
}

float CAICallback::GetPathLength(float3 start,float3 end,int pathType)
{
//	return pathfinder->GetPathLength(start,end,pathType);
	return 0;
}

int CAICallback::GetEnemyUnits(int *units)
{
	list<CUnit*>::iterator ui;
	int a=0;

	verify ();
	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(!gs->Ally((*ui)->allyteam,gs->AllyTeam(team)) && ((*ui)->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}


int CAICallback::GetEnemyUnitsInRadarAndLos(int *units)
{
	list<CUnit*>::iterator ui;
	int a=0;

	verify ();
	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(!gs->Ally((*ui)->allyteam,gs->AllyTeam(team)) && ((*ui)->losStatus[gs->AllyTeam(team)] & (LOS_INLOS|LOS_INRADAR))){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}


int CAICallback::GetEnemyUnits(int *units,const float3& pos,float radius)
{
	verify ();
	vector<CUnit*> unit=qf->GetUnitsExact(pos,radius);

	vector<CUnit*>::iterator ui;
	int a=0;

	for(ui=unit.begin();ui!=unit.end();++ui){
		if(!gs->Ally((*ui)->allyteam,gs->AllyTeam(team)) && ((*ui)->losStatus[gs->AllyTeam(team)] & LOS_INLOS)){
			units[a]=(*ui)->id;
			++a;
		}
	}
	return a;
}

int CAICallback::GetFriendlyUnits(int *units)
{
	int a=0;

	verify ();
	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(gs->Ally((*ui)->allyteam,gs->AllyTeam(team))){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CAICallback::GetFriendlyUnits(int *units,const float3& pos,float radius)
{
	verify ();
	vector<CUnit*> unit=qf->GetUnitsExact(pos,radius);

	vector<CUnit*>::iterator ui;
	int a=0;

	for(ui=unit.begin();ui!=unit.end();++ui){
		if((*ui)->team==gu->myTeam){
			units[a]=(*ui)->id;
			++a;
		}
	}
	return a;
}


int CAICallback::GetMapWidth()
{
	return gs->mapx;
}

int CAICallback::GetMapHeight()
{
	return gs->mapy;
}

extern string stupidGlobalMapname;
const char* CAICallback::GetMapName ()
{
	return stupidGlobalMapname.c_str();
}

const char* CAICallback::GetModName()
{
	return modInfo->name.c_str();
}

float CAICallback::GetMaxMetal()
{
	return readmap->maxMetal;
}

float CAICallback::GetExtractorRadius()
{
	return readmap->extractorRadius;
}

float CAICallback::GetMinWind()
{
	return wind.minWind;
}

float CAICallback::GetMaxWind()
{
	return wind.maxWind;
}

float CAICallback::GetTidalStrength()
{
	return readmap->tidalStrength;
}

float CAICallback::GetGravity()
{
	return gs->gravity;
}

/*const unsigned char* CAICallback::GetSupplyMap()
{
	return supplyhandler->supplyLevel[gu->myTeam];
}

const unsigned char* CAICallback::GetTeamMap()
{
	return readmap->teammap;
}*/

const float* CAICallback::GetHeightMap()
{
	return readmap->centerheightmap;
}

float CAICallback::GetMinHeight()
{
	return readmap->minheight;
}

float CAICallback::GetMaxHeight()
{
	return readmap->maxheight;
}

const float* CAICallback::GetSlopeMap()
{
	return readmap->slopemap;
}

const unsigned short* CAICallback::GetLosMap()
{
	return loshandler->losMap[gs->AllyTeam(team)];
}

const unsigned short* CAICallback::GetRadarMap()
{
	return radarhandler->radarMaps[gs->AllyTeam(team)];
}

const unsigned short* CAICallback::GetJammerMap()
{
	return radarhandler->jammerMaps[gs->AllyTeam(team)];
}

const unsigned char* CAICallback::GetMetalMap()
{
	return readmap->metalMap->metalMap;
}

float CAICallback::GetElevation(float x,float z)
{
	return ground->GetHeight(x,z);
}

int CAICallback::CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddSpline(pos1,pos2,pos3,pos4,width,arrow,lifetime,group);
}

int CAICallback::CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddLine(pos1,pos2,width,arrow,lifetime,group);
}

void CAICallback::SetFigureColor(int group,float red,float green,float blue,float alpha)
{
	geometricObjects->SetColor(group,red,green,blue,alpha);
}

void CAICallback::DeleteFigureGroup(int group)
{
	geometricObjects->DeleteGroup(group);
}

void CAICallback::DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder,int facing)
{
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitByName(name);
	if(!tdu.unitdef){
		logOutput.Print("Uknown unit in CAICallback::DrawUnit %s",name);
		return;
	}
	tdu.pos=pos;
	tdu.rotation=rotation;
	tdu.team=team;
	tdu.drawBorder=drawBorder;
	tdu.facing=facing;
	std::pair<int,CUnitDrawer::TempDrawUnit> tp(gs->frameNum+lifetime,tdu);
	if(transparent)
		unitDrawer->tempTransparentDrawUnits.insert(tp);
	else
		unitDrawer->tempDrawUnits.insert(tp);
}

bool CAICallback::CanBuildAt(const UnitDef* unitDef,float3 pos, int facing)
{
	CFeature* f;
	BuildInfo bi(unitDef, pos, facing);
	bi.pos=helper->Pos2BuildPos (bi);
	return !!uh->TestUnitBuildSquare(bi,f,gs->AllyTeam(team));
}


struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};


bool SearchOffsetComparator (const SearchOffset& a, const SearchOffset& b)
{
	return a.qdist < b.qdist;
}

const vector<SearchOffset>& GetSearchOffsetTable (int radius)
{
	static vector <SearchOffset> searchOffsets;
	int size = radius*radius*4;
	if (size > searchOffsets.size()) {
		searchOffsets.resize (size);

		for (int y=0;y<radius*2;y++)
			for (int x=0;x<radius*2;x++)
			{
				SearchOffset& i = searchOffsets[y*radius*2+x];

				i.dx = x-radius;
				i.dy = y-radius;
				i.qdist = i.dx*i.dx+i.dy*i.dy;
			}

		std::sort (searchOffsets.begin(), searchOffsets.end(), SearchOffsetComparator);
	}

	return searchOffsets;
}

float3 CAICallback::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist, int facing)
{
	CFeature* feature;
	int allyteam=gs->AllyTeam(team);

	int endr = (int)(searchRadius / (SQUARE_SIZE*2));
	const vector<SearchOffset>& ofs = GetSearchOffsetTable (endr);

	for(int so=0;so<endr*endr*4;so++) {
		float x = pos.x+ofs[so].dx*SQUARE_SIZE*2;
		float z = pos.z+ofs[so].dy*SQUARE_SIZE*2;
		BuildInfo bi(unitdef, float3(x,0,z), facing);
		bi.pos = helper->Pos2BuildPos (bi);
		if(uh->TestUnitBuildSquare(bi,feature,allyteam) && (!feature || feature->allyteam!=allyteam))
		{
			int xs=(int)(x/SQUARE_SIZE);
			int zs=(int)(z/SQUARE_SIZE);
			bool good=true;
			int xsize=bi.GetXSize();
			int ysize=bi.GetYSize();
			for(int z2=max(0,zs-ysize/2-minDist);z2<min(gs->mapy,zs+ysize+minDist);++z2){
				for(int x2=max(0,xs-xsize/2-minDist);x2<min(gs->mapx,xs+xsize+minDist);++x2){
					CSolidObject* so=readmap->groundBlockingObjectMap[z2*gs->mapx+x2];
					if(so && so->immobile && !dynamic_cast<CFeature*>(so)){
						good=false;
						break;
					}
				}
			}
			if(good) 
				return bi.pos;
		}
	}

	return float3(-1.0f,0.0f,0.0f);	
}


float CAICallback::GetMetal()
{
	return gs->Team(team)->metal;
}

float CAICallback::GetMetalIncome()
{
	return gs->Team(team)->oldMetalIncome;
}

float CAICallback::GetMetalUsage()
{
	return gs->Team(team)->oldMetalExpense;	
}

float CAICallback::GetMetalStorage()
{
	return gs->Team(team)->metalStorage;
}

float CAICallback::GetEnergy()
{
	return gs->Team(team)->energy;
}

float CAICallback::GetEnergyIncome()
{
	return gs->Team(team)->oldEnergyIncome;
}

float CAICallback::GetEnergyUsage()
{
	return gs->Team(team)->oldEnergyExpense;	
}

float CAICallback::GetEnergyStorage()
{
	return gs->Team(team)->energyStorage;
}

bool CAICallback::GetUnitResourceInfo (int unitid, UnitResourceInfo *i)
{
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS))
		{
			i->energyMake = unit->energyMake;
			i->energyUse = unit->energyUse;
			i->metalMake = unit->metalMake;
			i->metalUse = unit->metalUse;
			return true;
		}
	}
	return false;
}

bool CAICallback::IsUnitActivated (int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS))
			return unit->activated;
	}
	return false;
}

bool CAICallback::UnitBeingBuilt (int unitid)
{
	verify ();
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS))
			return unit->beingBuilt;
	}
	return false;
}

int CAICallback::GetFeatures (int *features, int max)
{
	verify ();
	int i = 0;
	int allyteam = gs->AllyTeam(team);

	for (int a=0;a<MAX_FEATURES;a++) {
		if (featureHandler->features [a]) {
			CFeature *f = featureHandler->features[a];

			if (f->allyteam >= 0 && f->allyteam!=allyteam && !loshandler->InLos(f->pos,allyteam))
				continue;

			features [i++] = f->id;

			if (i == max) 
				break;
		}
	}
	return i;
}

int CAICallback::GetFeatures (int *features, int maxids, const float3& pos, float radius)
{
	verify ();
	vector<CFeature*> ft = qf->GetFeaturesExact (pos, radius);
	int allyteam = gs->AllyTeam(team);
	int n = 0;
	
	for (int a=0;a<ft.size();a++)
	{
		CFeature *f = ft[a];

		if (f->allyteam >= 0 && f->allyteam!=allyteam && !loshandler->InLos(f->pos,allyteam))
			continue;

		features [n++] = f->id;
		if (maxids == n)
			break;
	}

	return n;
}

FeatureDef* CAICallback::GetFeatureDef (int feature)
{
	verify ();
	if (CHECK_FEATURE(feature)) {
		CFeature *f = featureHandler->features [feature];
		int allyteam = gs->AllyTeam(team);
		if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
			return f->def;
	}
	
	return 0;
}

float CAICallback::GetFeatureHealth (int feature)
{
	verify ();
	if (CHECK_FEATURE(feature)) {
		CFeature *f = featureHandler->features [feature];
		int allyteam = gs->AllyTeam(team);
		if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
			return f->health;
	}
	return 0.0f;
}

float CAICallback::GetFeatureReclaimLeft (int feature)
{
	verify ();
	if (CHECK_FEATURE(feature)) {
		CFeature *f = featureHandler->features [feature];
		int allyteam = gs->AllyTeam(team);
		if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
			return f->reclaimLeft;
	}
	return 0.0f;
}

float3 CAICallback::GetFeaturePos (int feature)
{
	verify ();
	if (CHECK_FEATURE(feature)) {
		CFeature *f = featureHandler->features [feature];
		int allyteam = gs->AllyTeam(team);
		if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
			return f->pos;
	}
	return ZeroVector;
}

bool CAICallback::GetValue(int id, void *data)
{
	verify();
	switch (id) {
		case AIVAL_NUMDAMAGETYPES:{
			*((int*)data) = DamageArray::numTypes;
			return true;
		}case AIVAL_EXCEPTION_HANDLING:{
			*(bool*)data = CGlobalAIHandler::CatchException();
			return true;
		}case AIVAL_MAP_CHECKSUM:{
			*(unsigned int*)data = readmap->mapChecksum;
			return true;
		}case AIVAL_DEBUG_MODE:{
			*(bool*)data = gu->drawdebug;
			return true;
		}case AIVAL_GAME_MODE:{
			*(int*)data = gs->gameMode;
			return true;
		}case AIVAL_GAME_PAUSED:{
			*(bool*)data = gs->paused;
			return true;
		}case AIVAL_GAME_SPEED_FACTOR:{
			*(float*)data = gs->speedFactor;
			return true;
		}case AIVAL_GUI_VIEW_RANGE:{
			*(float*)data = gu->viewRange;
			return true;
		}case AIVAL_GUI_SCREENX:{
			*(float*)data = gu->screenx;
			return true;
		}case AIVAL_GUI_SCREENY:{
			*(float*)data = gu->screeny;
			return true;
		}case AIVAL_GUI_CAMERA_DIR:{
			*(float3*)data = mouse->currentCamController->GetDir();
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			*(float3*)data = mouse->currentCamController->GetPos();
			return true;
		}case AIVAL_SCRIPT_FILENAME_DEPRECATED:{
			// Passing container objects across DLL boundaries is not safe on windows,
			// So this AIVAL is deprecated in favour of AIVAL_SCRIPT_FILENAME_CSTR
// 			if (gameSetup) *((std::string*)data) = gameSetup->setupFileName;
// 			else *((std::string*)data) = "";
			return false;
		}case AIVAL_SCRIPT_FILENAME_CSTR:{
			strcpy((char*) data, gameSetup ? gameSetup->setupFileName.c_str() : "");
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			std::string f((char*) data);
			f = filesystem.LocateFile(f);
			strcpy((char*) data, f.c_str());
			return true;
		}case AIVAL_LOCATE_FILE_W:{
			std::string f((char*) data);
			f = filesystem.LocateFile(f, FileSystem::WRITE | FileSystem::CREATE_DIRS);
			strcpy((char*) data, f.c_str());
			return true;
		}
		default:
			return false;
	}
}

int CAICallback::HandleCommand (int commandId, void *data)
{
	switch (commandId)
	{
	case AIHCQuerySubVersionId:
		return 1; // current version of Handle Command interface
	case AIHCAddMapPointId:
		net->SendSTLData<unsigned char, unsigned char, short, short, std::string>(
			NETMSG_MAPDRAW, team, CInMapDraw::NET_POINT,
			(short)((AIHCAddMapPoint *)data)->pos.x, (short)((AIHCAddMapPoint *)data)->pos.z,
			std::string(((AIHCAddMapPoint *)data)->label));
		return 1;
	case AIHCAddMapLineId:
		net->SendData<unsigned char, unsigned char, unsigned char, short, short, short, short>(
			NETMSG_MAPDRAW, 12 /*message size*/, team, CInMapDraw::NET_LINE,
			(short)((AIHCAddMapLine *)data)->posfrom.x, (short)((AIHCAddMapLine *)data)->posfrom.z,
			(short)((AIHCAddMapLine *)data)->posto.x, (short)((AIHCAddMapLine *)data)->posto.z);
		return 1;
	case AIHCRemoveMapPointId:
		net->SendData<unsigned char, unsigned char, unsigned char, short, short>(
			NETMSG_MAPDRAW, 8 /*message size*/, team, CInMapDraw::NET_ERASE,
			(short)((AIHCRemoveMapPoint *)data)->pos.x, (short)((AIHCRemoveMapPoint *)data)->pos.z);
		return 1;
	}
	return 0;
}

int CAICallback::GetNumUnitDefs ()
{
	return unitDefHandler->numUnits;
}

void CAICallback::GetUnitDefList (const UnitDef** list)
{
	for (int a=0;a<unitDefHandler->numUnits;a++)
		list [a] = unitDefHandler->GetUnitByID (a+1);
}


float CAICallback::GetUnitDefRadius(int def)
{
	UnitDef *ud = &unitDefHandler->unitDefs[def];
	S3DOModel* mdl = modelParser->Load3DO(ud->model.modelpath,ud->canfly?0.5f:1,0);
	return mdl->radius;
}

float CAICallback::GetUnitDefHeight(int def)
{
	UnitDef *ud = &unitDefHandler->unitDefs[def];
	S3DOModel* mdl = modelParser->Load3DO(ud->model.modelpath,ud->canfly?0.5f:1,0);
	return mdl->height;
}


bool CAICallback::GetProperty(int id, int property, void *data)
{
	verify ();
	if (CHECK_UNITID(id)) {
		switch (property) {
		case AIVAL_UNITDEF:{
			CUnit *unit = uh->units[id];
			if (unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)) {
				(*(const UnitDef**)data) = unit->unitDef;
				return true;
			}
			break;
		}
		}
	}
	return false;
}

int CAICallback::GetFileSize (const char *name)
{
	CFileHandler fh (name);

	if (!fh.FileExists ())
		return -1;

	return fh.FileSize();
}

bool CAICallback::ReadFile (const char *name, void *buffer, int bufferLength)
{
	CFileHandler fh (name);
	int fs;
	if (!fh.FileExists() || bufferLength < (fs = fh.FileSize()))
		return false;

	fh.Read (buffer, fs);
	return true;
}


// Additions to the interface by Alik
int CAICallback::GetSelectedUnits(int *units)
{
	verify ();
	int a=0;
	if (gu->myAllyTeam == gs->AllyTeam(team)) {
		for(set<CUnit*>::iterator ui=selectedUnits.selectedUnits.begin();ui!=selectedUnits.selectedUnits.end();++ui)
			units[a++]=(*ui)->id;
	}
	return a;
}


float3 CAICallback::GetMousePos() {
	verify ();
	if (gu->myAllyTeam == gs->AllyTeam(team))
		return inMapDrawer->GetMouseMapPos();
	else
		return ZeroVector;
}


int CAICallback::GetMapPoints(PointMarker *pm, int maxPoints)
{
	int a=0;
	verify ();

	if (gu->myAllyTeam != gs->AllyTeam(team))
		return 0;

	for (int i=0;i<inMapDrawer->numQuads;i++){
		if(!inMapDrawer->drawQuads[i].points.empty()){
			for(list<CInMapDraw::MapPoint>::iterator mp=inMapDrawer->drawQuads[i].points.begin();mp!=inMapDrawer->drawQuads[i].points.end();++mp){
				if(mp->color==gs->Team(team)->color) { //Maybe add so that markers of your ally team would be also found?
					pm[a].pos=mp->pos;
					pm[a].color=mp->color;
					pm[a].label=mp->label.c_str();
					if (++a == maxPoints) return a;
				}
				else{ continue; }
			}
		}
	}
	return a;
}

int CAICallback::GetMapLines(LineMarker *lm, int maxLines) 
{
	int a=0;
	verify ();

	if (gu->myAllyTeam != gs->AllyTeam(team))
		return 0;

	for (int i=0;i<inMapDrawer->numQuads;i++){
		if(!inMapDrawer->drawQuads[i].points.empty()){
			for(list<CInMapDraw::MapLine>::iterator ml=inMapDrawer->drawQuads[i].lines.begin();ml!=inMapDrawer->drawQuads[i].lines.end();++ml){
				if(ml->color==gs->Team(team)->color){ //Maybe add so that markers of your ally team would be also found?
					lm[a].pos=ml->pos;
					lm[a].color=ml->color;
					lm[a].pos2=ml->pos2;
					if (++a == maxLines) return a;
				}
				else {continue;}
			}
		}
	}
	return a;
}

IMPLEMENT_PURE_VIRTUAL(IAICallback::~IAICallback())
