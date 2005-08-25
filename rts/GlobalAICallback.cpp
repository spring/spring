#include "StdAfx.h"
#include "GlobalAICallback.h"
#include "Net.h"
#include "GlobalAI.h"
#include "ReadMap.h"
#include "LosHandler.h"
#include "InfoConsole.h"
#include "Group.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "Team.h"
//#include "multipath.h"
//#include "PathHandler.h"
#include "QuadField.h"
#include "SelectedUnits.h"
#include "GeometricObjects.h"
#include "CommandAI.h"
#include "GameHelper.h"
#include "UnitDefHandler.h"
#include "GroupHandler.h"
#include "GlobalAIHandler.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "IGroupAI.h"
#include "PathManager.h"
#include "AICheats.h"
#include "GameSetup.h"
#include "SmfReadMap.h"
#include "Wind.h"
#include "UnitDrawer.h"
//#include "mmgr.h"

CGlobalAICallback::CGlobalAICallback(CGlobalAI* ai)
: ai(ai),
	cheats(0), 
	noMessages(false)
{
}

CGlobalAICallback::~CGlobalAICallback(void)
{
	delete cheats;
}

IAICheats* CGlobalAICallback::GetCheatInterface()
{
	if(cheats)
		return cheats;

	if(!gs->cheatEnabled)
		return 0;

	cheats=new CAICheats(ai);
	return cheats;
}

void CGlobalAICallback::SendTextMsg(const char* text,int priority)
{
	//todo: fix priority
	info->AddLine("GlobalAI%i: %s",ai->team,text);
}

int CGlobalAICallback::GetCurrentFrame()
{
	return gs->frameNum;
}

int CGlobalAICallback::GetMyTeam()
{
	return ai->team;
}

int CGlobalAICallback::GetMyAllyTeam()
{
	return gs->team2allyteam[ai->team];
}

void* CGlobalAICallback::CreateSharedMemArea(char* name, int size)
{
	return globalAI->GetAIBuffer(ai->team,name,size);
}

void CGlobalAICallback::ReleasedSharedMemArea(char* name)
{
	globalAI->ReleaseAIBuffer(ai->team,name);
}

int CGlobalAICallback::CreateGroup(char* dll)
{
	CGroup* g=ai->gh->CreateNewGroup(dll);
	return g->id;
}

void CGlobalAICallback::EraseGroup(int groupid)
{
	if(ai->gh->groups[groupid])
		ai->gh->RemoveGroup(ai->gh->groups[groupid]);

}

bool CGlobalAICallback::AddUnitToGroup(int unitid,int groupid)
{
	CUnit* u=uh->units[unitid];
	if(u && u->team==ai->team && ai->gh->groups[groupid]){
		return u->SetGroup(ai->gh->groups[groupid]);
	}
	return false;
}

bool CGlobalAICallback::RemoveUnitFromGroup(int unitid)
{
	CUnit* u=uh->units[unitid];
	if(u && u->team==ai->team){
		u->SetGroup(0);
		return true;
	}
	return false;
}

int CGlobalAICallback::GetUnitGroup(int unitid)
{
	if(gs->teams[ai->team]->units.find(uh->units[unitid])!=gs->teams[ai->team]->units.end()){
		CGroup* g=uh->units[unitid]->group;
		if(g)
			return g->id;
	}
	return -1;
}

const std::vector<CommandDescription>* CGlobalAICallback::GetGroupCommands(int groupid)
{
	static std::vector<CommandDescription> tempcmds;

	if(ai->gh->groups[groupid] && ai->gh->groups[groupid]->ai)
		return &ai->gh->groups[groupid]->ai->GetPossibleCommands();

	return &tempcmds;
}

int CGlobalAICallback::GiveGroupOrder(int groupid, Command* c)
{
	if(ai->gh->groups[groupid] && ai->gh->groups[groupid]->ai)
		ai->gh->groups[groupid]->ai->GiveCommand(c);

	return 0;
}
	
int CGlobalAICallback::GiveOrder(int unitid,Command* c)
{
	if (noMessages)
		return -1;

	if(gs->teams[ai->team]->units.find(uh->units[unitid])==gs->teams[ai->team]->units.end())
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

const vector<CommandDescription>* CGlobalAICallback::GetUnitCommands(int unitid)
{
	if(gs->teams[ai->team]->units.find(uh->units[unitid])!=gs->teams[ai->team]->units.end()){
		return &(uh->units[unitid]->commandAI->possibleCommands);
	}
	return 0;
}

const deque<Command>* CGlobalAICallback::GetCurrentUnitCommands(int unitid)
{
	if(gs->teams[ai->team]->units.find(uh->units[unitid])!=gs->teams[ai->team]->units.end()){
		return &(uh->units[unitid]->commandAI->commandQue);
	}
	return 0;
}

int CGlobalAICallback::GetUnitAiHint(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->aihint;
	}
	return 0;
}

int CGlobalAICallback::GetUnitTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->team;
	}
	return 0;	
}

int CGlobalAICallback::GetUnitAllyTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->allyteam;
	}
	return 0;	
}

float CGlobalAICallback::GetUnitHealth(int unitid)			//the units current health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->health;
	}
	return 0;
}

float CGlobalAICallback::GetUnitMaxHealth(int unitid)		//the units max health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->maxHealth;
	}
	return 0;
}

float CGlobalAICallback::GetUnitSpeed(int unitid)				//the units max speed
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->unitDef->speed;
	}
	return 0;
}

float CGlobalAICallback::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->power;
	}
	return 0;
}

float CGlobalAICallback::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->experience;
	}
	return 0;
}

float CGlobalAICallback::GetUnitMaxRange(int unitid)		//the furthest any weapon of the unit can fire
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->maxRange;
	}
	return 0;
}

const UnitDef* CGlobalAICallback::GetUnitDef(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
		return unit->unitDef;
	}
	return 0;
}

const UnitDef* CGlobalAICallback::GetUnitDef(const char* unitName)
{
	return unitDefHandler->GetUnitByName(unitName);
}

float3 CGlobalAICallback::GetUnitPos(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & (LOS_INLOS|LOS_INRADAR))){
		return helper->GetUnitErrorPos(unit,gs->team2allyteam[ai->team]);
	}
	return ZeroVector;
}


int CGlobalAICallback::InitPath(float3 start,float3 end,int pathType)
{
	return pathManager->RequestPath(moveinfo->moveData.at(pathType),start,end);
}

float3 CGlobalAICallback::GetNextWaypoint(int pathid)
{
	return pathManager->NextWaypoint(pathid,ZeroVector);
}

void CGlobalAICallback::FreePath(int pathid)
{
	pathManager->DeletePath(pathid);
}

float CGlobalAICallback::GetPathLength(float3 start,float3 end,int pathType)
{
//	return pathfinder->GetPathLength(start,end,pathType);
	return 0;
}

int CGlobalAICallback::GetEnemyUnits(int *units)
{
	list<CUnit*>::iterator ui;
	int a=0;

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(!gs->allies[(*ui)->allyteam][gs->team2allyteam[ai->team]] && ((*ui)->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CGlobalAICallback::GetEnemyUnits(int *units,const float3& pos,float radius)
{
	vector<CUnit*> unit=qf->GetUnitsExact(pos,radius);

	vector<CUnit*>::iterator ui;
	int a=0;

	for(ui=unit.begin();ui!=unit.end();++ui){
		if(!gs->allies[(*ui)->allyteam][gs->team2allyteam[ai->team]] && ((*ui)->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS)){
			units[a]=(*ui)->id;
			++a;
		}
	}
	return a;
}

int CGlobalAICallback::GetFriendlyUnits(int *units)
{
	int a=0;

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(gs->allies[(*ui)->allyteam][gs->team2allyteam[ai->team]]){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CGlobalAICallback::GetFriendlyUnits(int *units,const float3& pos,float radius)
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


int CGlobalAICallback::GetMapWidth()
{
	return gs->mapx;
}

int CGlobalAICallback::GetMapHeight()
{
	return gs->mapy;
}

extern string stupidGlobalMapname;
const char* CGlobalAICallback::GetMapName ()
{
	return stupidGlobalMapname.c_str();
}

extern string stupidGlobalModName;
const char* CGlobalAICallback::GetModName()
{
	return stupidGlobalModName.c_str();
}

float CGlobalAICallback::GetMaxMetal()
{
	return ((CSmfReadMap*)readmap)->maxMetal;
}

float CGlobalAICallback::GetExtractorRadius()
{
	return ((CSmfReadMap*)readmap)->extractorRadius;
}

float CGlobalAICallback::GetMinWind()
{
	return wind.minWind;
}

float CGlobalAICallback::GetMaxWind()
{
	return wind.maxWind;
}

float CGlobalAICallback::GetTidalStrength()
{
	return ((CSmfReadMap*)readmap)->tidalStrength;
}

/*const unsigned char* CGlobalAICallback::GetSupplyMap()
{
	return supplyhandler->supplyLevel[gu->myTeam];
}

const unsigned char* CGlobalAICallback::GetTeamMap()
{
	return readmap->teammap;
}*/

const float* CGlobalAICallback::GetHeightMap()
{
	return readmap->centerheightmap;
}

const unsigned short* CGlobalAICallback::GetLosMap()
{
	return loshandler->losMap[gs->team2allyteam[ai->team]];
}

const unsigned short* CGlobalAICallback::GetRadarMap()
{
	return radarhandler->radarMaps[gs->team2allyteam[ai->team]];
}

const unsigned short* CGlobalAICallback::GetJammerMap()
{
	return radarhandler->jammerMaps[gs->team2allyteam[ai->team]];
}

const unsigned char* CGlobalAICallback::GetMetalMap()
{
	return readmap->metalMap->metalMap;
}

float CGlobalAICallback::GetElevation(float x,float z)
{
	return ground->GetHeight(x,z);
}

int CGlobalAICallback::CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddSpline(pos1,pos2,pos3,pos4,width,arrow,lifetime,group);
}

int CGlobalAICallback::CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddLine(pos1,pos2,width,arrow,lifetime,group);
}

void CGlobalAICallback::SetFigureColor(int group,float red,float green,float blue,float alpha)
{
	geometricObjects->SetColor(group,red,green,blue,alpha);
}

void CGlobalAICallback::DeleteFigureGroup(int group)
{
	geometricObjects->DeleteGroup(group);
}

void CGlobalAICallback::DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder)
{
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitByName(name);
	if(!tdu.unitdef){
		info->AddLine("Uknown unit in CGlobalAICallback::DrawUnit %s",name);
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

bool CGlobalAICallback::CanBuildAt(const UnitDef* unitDef,float3 pos)
{
	CFeature* f;
	pos=helper->Pos2BuildPos (pos, unitDef);
	return !!uh->TestUnitBuildSquare(pos,unitDef->name,f);
}

float3 CGlobalAICallback::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist)
{
	//todo fix so you cant detect enemy buildings with it
	float bestDist=searchRadius;
	float3 bestPos(-1,0,0);

	CFeature* feature;
	int allyteam=gs->team2allyteam[ai->team];

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


float CGlobalAICallback::GetMetal()
{
	return gs->teams[gu->myTeam]->metal;
}

float CGlobalAICallback::GetMetalIncome()
{
	return gs->teams[gu->myTeam]->oldMetalIncome;
}

float CGlobalAICallback::GetMetalUsage()
{
	return gs->teams[gu->myTeam]->oldMetalExpense;	
}

float CGlobalAICallback::GetMetalStorage()
{
	return gs->teams[gu->myTeam]->metalStorage;
}

float CGlobalAICallback::GetEnergy()
{
	return gs->teams[gu->myTeam]->energy;
}

float CGlobalAICallback::GetEnergyIncome()
{
	return gs->teams[gu->myTeam]->oldEnergyIncome;
}

float CGlobalAICallback::GetEnergyUsage()
{
	return gs->teams[gu->myTeam]->oldEnergyExpense;	
}

float CGlobalAICallback::GetEnergyStorage()
{
	return gs->teams[gu->myTeam]->energyStorage;
}

bool CGlobalAICallback::GetUnitResourceInfo (int unitid, UnitResourceInfo *i)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS))
	{
		i->energyMake = unit->energyMake;
		i->energyUse = unit->energyUse;
		i->metalMake = unit->metalMake;
		i->metalUse = unit->metalUse;
		return true;
	}
	return false;
}

bool CGlobalAICallback::IsUnitActivated (int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS))
		return unit->activated;
	return false;
}

bool CGlobalAICallback::UnitBeingBuilt (int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[ai->team]] & LOS_INLOS))
		return unit->beingBuilt;
	return false;
}

int CGlobalAICallback::GetFeatures (int *features, int max)
{
	int i = 0;
	int allyteam = gs->team2allyteam[ai->team];

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

int CGlobalAICallback::GetFeatures (int *features, int maxids, const float3& pos, float radius)
{
	vector<CFeature*> ft = qf->GetFeaturesExact (pos, radius);
	int allyteam = gs->team2allyteam[ai->team];
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

FeatureDef* CGlobalAICallback::GetFeatureDef (int feature)
{
	CFeature *f = featureHandler->features [feature];
	int allyteam = gs->team2allyteam[ai->team];
	if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
		return f->def;
	
	return 0;
}

float CGlobalAICallback::GetFeatureHealth (int feature)
{
	CFeature *f = featureHandler->features [feature];
	int allyteam = gs->team2allyteam[ai->team];
	if (f->allyteam < 0 || f->allyteam == gs->team2allyteam[ai->team] || loshandler->InLos(f->pos,allyteam))
		return f->health;
	return 0.0f;
}

float CGlobalAICallback::GetFeatureReclaimLeft (int feature)
{
	CFeature *f = featureHandler->features [feature];
	int allyteam = gs->team2allyteam[ai->team];
	if (f->allyteam < 0 || f->allyteam == gs->team2allyteam[ai->team] || loshandler->InLos(f->pos,allyteam))
		return f->reclaimLeft;
	return 0.0f;
}

