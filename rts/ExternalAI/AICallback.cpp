// Generalized callback interface - shared between global AI and group AI
// by Zaphod
#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "Net.h"
#include "GlobalAI.h"
#include "Sim/Map/ReadMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Group.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Game/Team.h"
#include "Sim/Misc/QuadField.h"
#include "Game/SelectedUnits.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Units/CommandAI/CommandAI.h"
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
#include "Sim/Map/SmfReadMap.h"
#include "Sim/Misc/Wind.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Game/Player.h"
#include "Rendering/InMapDraw.h"
#include "FileSystem/FileHandler.h"
#include "Rendering/InMapDraw.h"
#include "FileSystem/FileHandler.h"
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
		info->AddLine(priority, "Group%i: %s",group->id,text);
	else
        info->AddLine(priority, "GlobalAI%i: %s",team,text);
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

int CAICallback::CreateGroup(char* dll)
{
	CGroup* g=gh->CreateNewGroup(dll);
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
	if (!CHECK_UNITID(unitid) || c == NULL)
		return -1;

	if (noMessages)
		return -1;

	CUnit *unit = uh->units[unitid];

	if (group && unit->group != group)
		return -1;

	if (unit->team != team)
		return -1;

	netbuf[0]=NETMSG_AICOMMAND;
	*((short int*)&netbuf[1])=c->params.size()*4+11;
	netbuf[3]=gu->myPlayerNum;
	*((short int*)&netbuf[4])=unitid;
	*((int*)&netbuf[6])=c->id;
	netbuf[10]=c->options;
	int a=0;
	vector<float>::iterator oi;
	for(oi=c->params.begin();oi!=c->params.end();++oi){
		*((float*)&netbuf[11+a*4])=(*oi);			
		a++;
	}
	net->SendData(netbuf,c->params.size()*4+11);
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
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & (LOS_INLOS|LOS_INRADAR))){
			return helper->GetUnitErrorPos(unit,gs->AllyTeam(team));
		}
	}
	return ZeroVector;
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

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(!gs->Ally((*ui)->allyteam,gs->AllyTeam(team)) && ((*ui)->losStatus[gs->AllyTeam(team)] & (LOS_INLOS|LOS_INRADAR))){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}


int CAICallback::GetEnemyUnits(int *units,const float3& pos,float radius)
{
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

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(gs->Ally((*ui)->allyteam,gs->AllyTeam(team))){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CAICallback::GetFriendlyUnits(int *units,const float3& pos,float radius)
{
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

extern string stupidGlobalModName;
const char* CAICallback::GetModName()
{
	return stupidGlobalModName.c_str();
}

float CAICallback::GetMaxMetal()
{
	return ((CSmfReadMap*)readmap)->maxMetal;
}

float CAICallback::GetExtractorRadius()
{
	return ((CSmfReadMap*)readmap)->extractorRadius;
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
	return ((CSmfReadMap*)readmap)->tidalStrength;
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

void CAICallback::DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder)
{
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitByName(name);
	if(!tdu.unitdef){
		info->AddLine("Uknown unit in CAICallback::DrawUnit %s",name);
		return;
	}
	tdu.pos=pos;
	tdu.rot=rotation;
	tdu.team=team;
	tdu.drawBorder=drawBorder;
	std::pair<int,CUnitDrawer::TempDrawUnit> tp(gs->frameNum+lifetime,tdu);
	if(transparent)
		unitDrawer->tempTransperentDrawUnits.insert(tp);
	else
		unitDrawer->tempDrawUnits.insert(tp);
}

bool CAICallback::CanBuildAt(const UnitDef* unitDef,float3 pos)
{
	CFeature* f;
	pos=helper->Pos2BuildPos (pos, unitDef);
	return !!uh->TestUnitBuildSquare(pos,unitDef->name,f);
}

float3 CAICallback::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist)
{
	//todo fix so you cant detect enemy buildings with it
	float bestDist=searchRadius;
	float3 bestPos(-1,0,0);

	CFeature* feature;
	int allyteam=gs->AllyTeam(team);

	for(float z=pos.z-searchRadius;z<pos.z+searchRadius;z+=SQUARE_SIZE*2){
		for(float x=pos.x-searchRadius;x<pos.x+searchRadius;x+=SQUARE_SIZE*2){
			float3 p(x,0,z);
			p = helper->Pos2BuildPos (p, unitdef);
			float dist=pos.distance2D(p);
			if(dist<bestDist && uh->TestUnitBuildSquare(p,unitdef,feature) && (!feature || feature->allyteam!=allyteam)){
				int xs=(int)(x/SQUARE_SIZE);
				int zs=(int)(z/SQUARE_SIZE);
				bool good=true;
				for(int z2=max(0,zs-unitdef->ysize/2-minDist);z2<min(gs->mapy,zs+unitdef->ysize+minDist);++z2){
					for(int x2=max(0,xs-unitdef->xsize/2-minDist);x2<min(gs->mapx,xs+unitdef->xsize+minDist);++x2){
						CSolidObject* so=readmap->groundBlockingObjectMap[z2*gs->mapx+x2];
						if(so && so->immobile && !dynamic_cast<CFeature*>(so)){
							good=false;
							break;
						}
					}
				}
				if(good){
					bestDist=dist;
					bestPos=p;
				}
			}
		}
	}

	return bestPos;	
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
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS))
			return unit->activated;
	}
	return false;
}

bool CAICallback::UnitBeingBuilt (int unitid)
{
	if (CHECK_UNITID(unitid)) {
		CUnit* unit=uh->units[unitid];
		if(unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS))
			return unit->beingBuilt;
	}
	return false;
}

int CAICallback::GetFeatures (int *features, int max)
{
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
	return false;
}

int CAICallback::HandleCommand (void *data)
{
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

bool CAICallback::GetProperty(int id, int property, void *data)
{
	if (CHECK_UNITID(id)) {
		switch (property) {
		case AIVAL_UNITDEF:{
			CUnit *unit = uh->units[id];
			if (unit && (unit->losStatus[gs->AllyTeam(team)] & LOS_INLOS)) {
				(*(const UnitDef**)data) = unit->unitDef;
				return true;
			}
			break;}
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
	int a=0;
	if (gu->myAllyTeam == gs->AllyTeam(team)) {
		for(set<CUnit*>::iterator ui=selectedUnits.selectedUnits.begin();ui!=selectedUnits.selectedUnits.end();++ui)
			units[a++]=(*ui)->id;
	}
	return a;
}


float3 CAICallback::GetMousePos() {
	if (gu->myAllyTeam == gs->AllyTeam(team))
		return inMapDrawer->GetMouseMapPos();
	else
		return ZeroVector;
}


int CAICallback::GetMapPoints(PointMarker *pm, int maxPoints)
{
	int a=0;

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

