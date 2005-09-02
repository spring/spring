// GroupAICallback.cpp: implementation of the CGroupAICallback class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAiCallback.h"
#include "Net.h"
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
#include "GuiHandler.h"		//todo: fix some switch for new gui
#include "NewGuiDefine.h"
#include "GUIcontroller.h"
#include "GroupHandler.h"
#include "GlobalAIHandler.h"
#include "Feature.h"
#include "FeatureHandler.h"
#include "PathManager.h"
#include "UnitDrawer.h"
#include "Player.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace std;

CGroupAICallback::CGroupAICallback(CGroup* group)
: group(group)
{

}

CGroupAICallback::~CGroupAICallback()
{

}

void CGroupAICallback::SendTextMsg(const char* text,int priority)
{
	//todo: fix priority
	info->AddLine("Group%i: %s",group->id,text);
}

int CGroupAICallback::GetCurrentFrame()
{
	return gs->frameNum;
}

int CGroupAICallback::GetMyTeam()
{
	return group->handler->team;
}

int CGroupAICallback::GetMyAllyTeam()
{
	return gs->team2allyteam[group->handler->team];
}

int CGroupAICallback::GetPlayerTeam(int player)
{
	CPlayer *pl = gs->players [player];
	if (pl->spectator)
		return -1;
	return pl->team;
}

void* CGroupAICallback::CreateSharedMemArea(char* name, int size)
{
	return globalAI->GetAIBuffer(group->handler->team,name,size);
}

void CGroupAICallback::ReleasedSharedMemArea(char* name)
{
	globalAI->ReleaseAIBuffer(group->handler->team,name);
}

int CGroupAICallback::GiveOrder(int unitid,Command* c)
{
	if(group->units.find(uh->units[unitid])==group->units.end())
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
void CGroupAICallback::UpdateIcons()
{
	selectedUnits.PossibleCommandChange(0);
}

const Command* CGroupAICallback::GetOrderPreview()
{
	static Command tempcmd;
	//todo: need to add support for new gui
#ifdef NEW_GUI
	tempcmd=guicontroller->GetOrderPreview();
#else
	tempcmd=guihandler->GetOrderPreview();
#endif
	return &tempcmd;
}

bool CGroupAICallback::IsSelected()
{
	return selectedUnits.selectedGroup==group->id;
}

int CGroupAICallback::GetUnitLastUserOrder(int unitid)
{
	if(group->units.find(uh->units[unitid])!=group->units.end()){
		return uh->units[unitid]->commandAI->lastUserCommand;
	}
	return 0;
}

const vector<CommandDescription>* CGroupAICallback::GetUnitCommands(int unitid)
{
	if(group->units.find(uh->units[unitid])!=group->units.end()){
		return &(uh->units[unitid]->commandAI->possibleCommands);
	}
	return 0;
}

const deque<Command>* CGroupAICallback::GetCurrentUnitCommands(int unitid)
{
	if(group->units.find(uh->units[unitid])!=group->units.end()){
		return &(uh->units[unitid]->commandAI->commandQue);
	}
	return 0;
}

int CGroupAICallback::GetUnitAihint(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->aihint;
	}
	return 0;
}

int CGroupAICallback::GetUnitTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->team;
	}
	return 0;	
}

int CGroupAICallback::GetUnitAllyTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->allyteam;
	}
	return 0;	
}

float CGroupAICallback::GetUnitHealth(int unitid)			//the units current health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->health;
	}
	return 0;
}

float CGroupAICallback::GetUnitMaxHealth(int unitid)		//the units max health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->maxHealth;
	}
	return 0;
}

float CGroupAICallback::GetUnitSpeed(int unitid)				//the units max speed
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->unitDef->speed;
	}
	return 0;
}

float CGroupAICallback::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->power;
	}
	return 0;
}

float CGroupAICallback::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->experience;
	}
	return 0;
}

float CGroupAICallback::GetUnitMaxRange(int unitid)		//the furthest any weapon of the unit can fire
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->maxRange;
	}
	return 0;
}

const UnitDef* CGroupAICallback::GetUnitDef(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->unitDef;
	}
	return 0;
}

const UnitDef* CGroupAICallback::GetUnitDef(const char* unitName)
{
	return unitDefHandler->GetUnitByName(unitName);
}

float3 CGroupAICallback::GetUnitPos(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & (LOS_INLOS|LOS_INRADAR))){
		return helper->GetUnitErrorPos(unit,gs->team2allyteam[group->handler->team]);
	}
	return ZeroVector;
}


int CGroupAICallback::InitPath(float3 start,float3 end,int pathType)
{
	return pathManager->RequestPath(moveinfo->moveData.at(pathType),start,end);
}

float3 CGroupAICallback::GetNextWaypoint(int pathid)
{
	return pathManager->NextWaypoint(pathid,ZeroVector);
}

void CGroupAICallback::FreePath(int pathid)
{
	pathManager->DeletePath(pathid);
}

float CGroupAICallback::GetPathLength(float3 start,float3 end,int pathType)
{
//	return pathfinder->GetPathLength(start,end,pathType);
	return 0;
}

int CGroupAICallback::GetEnemyUnits(int *units)
{
	list<CUnit*>::iterator ui;
	int a=0;

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(!gs->allies[(*ui)->allyteam][gs->team2allyteam[group->handler->team]] && ((*ui)->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CGroupAICallback::GetEnemyUnits(int *units,const float3& pos,float radius)
{
	vector<CUnit*> unit=qf->GetUnitsExact(pos,radius);

	vector<CUnit*>::iterator ui;
	int a=0;

	for(ui=unit.begin();ui!=unit.end();++ui){
		if(!gs->allies[(*ui)->allyteam][gs->team2allyteam[group->handler->team]] && ((*ui)->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
			units[a]=(*ui)->id;
			++a;
		}
	}
	return a;
}

int CGroupAICallback::GetFriendlyUnits(int *units)
{
	int a=0;

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(gs->allies[(*ui)->allyteam][gs->team2allyteam[group->handler->team]]){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CGroupAICallback::GetFriendlyUnits(int *units,const float3& pos,float radius)
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


int CGroupAICallback::GetMapWidth()
{
	return gs->mapx;
}

int CGroupAICallback::GetMapHeight()
{
	return gs->mapy;
}

extern string stupidGlobalMapname;
const char* CGroupAICallback::GetMapName ()
{
	return stupidGlobalMapname.c_str();
}

extern string stupidGlobalModName;
const char* CGroupAICallback::GetModName()
{
	return stupidGlobalModName.c_str();
}


/*const unsigned char* CGroupAICallback::GetSupplyMap()
{
	return supplyhandler->supplyLevel[gu->myTeam];
}

const unsigned char* CGroupAICallback::GetTeamMap()
{
	return readmap->teammap;
}*/

const float* CGroupAICallback::GetHeightMap()
{
	return readmap->centerheightmap;
}

const unsigned short* CGroupAICallback::GetLosMap()
{
	return loshandler->losMap[gs->team2allyteam[group->handler->team]];
}

const unsigned short* CGroupAICallback::GetRadarMap()
{
	return radarhandler->radarMaps[gs->team2allyteam[group->handler->team]];
}

const unsigned short* CGroupAICallback::GetJammerMap()
{
	return radarhandler->jammerMaps[gs->team2allyteam[group->handler->team]];
}

const unsigned char* CGroupAICallback::GetMetalMap()
{
	return readmap->metalMap->metalMap;
}

float CGroupAICallback::GetElevation(float x,float z)
{
	return ground->GetHeight(x,z);
}

int CGroupAICallback::CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddSpline(pos1,pos2,pos3,pos4,width,arrow,lifetime,group);
}

int CGroupAICallback::CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddLine(pos1,pos2,width,arrow,lifetime,group);
}

void CGroupAICallback::SetFigureColor(int group,float red,float green,float blue,float alpha)
{
	geometricObjects->SetColor(group,red,green,blue,alpha);
}

void CGroupAICallback::DeleteFigureGroup(int group)
{
	geometricObjects->DeleteGroup(group);
}

void CGroupAICallback::DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder)
{
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitByName(name);
	if(!tdu.unitdef){
		info->AddLine("Uknown unit in CGroupAICallback::DrawUnit %s",name);
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

bool CGroupAICallback::CanBuildAt(const UnitDef* unitDef,float3 pos)
{
	//todo fix so you cant detect enemy buildings etc with this
	CFeature* f;
	pos=helper->Pos2BuildPos(pos,unitDef);
	return !!uh->TestUnitBuildSquare(pos,unitDef,f);
}

float3 CGroupAICallback::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist)
{
	//todo fix so you cant detect enemy buildings with it
	float bestDist=searchRadius;
	float3 bestPos(-1,0,0);

	CFeature* feature;
	int allyteam=gs->team2allyteam[group->handler->team];

	for(float z=pos.z-searchRadius;z<pos.z+searchRadius;z+=SQUARE_SIZE*2){
		for(float x=pos.x-searchRadius;x<pos.x+searchRadius;x+=SQUARE_SIZE*2){
			float3 p(x,0,z);
			p=helper->Pos2BuildPos(p,unitdef);
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

float CGroupAICallback::GetMetal()
{
	return gs->teams[gu->myTeam]->metal;
}

float CGroupAICallback::GetMetalIncome()
{
	return gs->teams[gu->myTeam]->oldMetalIncome;
}

float CGroupAICallback::GetMetalUsage()
{
	return gs->teams[gu->myTeam]->oldMetalExpense;	
}

float CGroupAICallback::GetMetalStorage()
{
	return gs->teams[gu->myTeam]->metalStorage;
}

float CGroupAICallback::GetEnergy()
{
	return gs->teams[gu->myTeam]->energy;
}

float CGroupAICallback::GetEnergyIncome()
{
	return gs->teams[gu->myTeam]->oldEnergyIncome;
}

float CGroupAICallback::GetEnergyUsage()
{
	return gs->teams[gu->myTeam]->oldEnergyExpense;	
}

float CGroupAICallback::GetEnergyStorage()
{
	return gs->teams[gu->myTeam]->energyStorage;
}

int CGroupAICallback::GetFeatures (int *features, int max)
{
	int i = 0;

	int allyteam = gs->team2allyteam[group->handler->team];

	for (int a=0;a<MAX_FEATURES;a++)
	{
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

int CGroupAICallback::GetFeatures (int *features, int maxids, const float3& pos, float radius)
{
	vector<CFeature*> ft = qf->GetFeaturesExact (pos, radius);
	int allyteam = gs->team2allyteam[group->handler->team];
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

FeatureDef* CGroupAICallback::GetFeatureDef (int feature)
{
	CFeature *f = featureHandler->features [feature];
	int allyteam = gs->team2allyteam[group->handler->team];
	if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
		return f->def;
 	
 	return 0;
}

float CGroupAICallback::GetFeatureHealth (int feature)
{
	CFeature *f = featureHandler->features [feature];
	int allyteam = gs->team2allyteam[group->handler->team];
	if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
		return f->health;
	return 0.0f;
}

float CGroupAICallback::GetFeatureReclaimLeft (int feature)
{
	CFeature *f = featureHandler->features [feature];
	int allyteam = gs->team2allyteam[group->handler->team];
	if (f->allyteam < 0 || f->allyteam == allyteam || loshandler->InLos(f->pos,allyteam))
		return f->reclaimLeft;
	return 0.0f;
}

