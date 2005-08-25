// GroupAiCallback.cpp: implementation of the CGroupAiCallback class.
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
#include "PathManager.h"
#include "UnitDrawer.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace std;

CGroupAiCallback::CGroupAiCallback(CGroup* group)
: group(group)
{

}

CGroupAiCallback::~CGroupAiCallback()
{

}

void CGroupAiCallback::SendTextMsg(const char* text,int priority)
{
	//todo: fix priority
	info->AddLine("Group%i: %s",group->id,text);
}

int CGroupAiCallback::GetCurrentFrame()
{
	return gs->frameNum;
}

int CGroupAiCallback::GetMyTeam()
{
	return group->handler->team;
}

int CGroupAiCallback::GetMyAllyTeam()
{
	return gs->team2allyteam[group->handler->team];
}

void* CGroupAiCallback::CreateSharedMemArea(char* name, int size)
{
	return globalAI->GetAIBuffer(group->handler->team,name,size);
}

void CGroupAiCallback::ReleasedSharedMemArea(char* name)
{
	globalAI->ReleaseAIBuffer(group->handler->team,name);
}

int CGroupAiCallback::GiveOrder(int unitid,Command* c)
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
void CGroupAiCallback::UpdateIcons()
{
	selectedUnits.PossibleCommandChange(0);
}

const Command* CGroupAiCallback::GetOrderPreview()
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

bool CGroupAiCallback::IsSelected()
{
	return selectedUnits.selectedGroup==group->id;
}

int CGroupAiCallback::GetUnitLastUserOrder(int unitid)
{
	if(group->units.find(uh->units[unitid])!=group->units.end()){
		return uh->units[unitid]->commandAI->lastUserCommand;
	}
	return 0;
}

const vector<CommandDescription>* CGroupAiCallback::GetUnitCommands(int unitid)
{
	if(group->units.find(uh->units[unitid])!=group->units.end()){
		return &(uh->units[unitid]->commandAI->possibleCommands);
	}
	return 0;
}

const deque<Command>* CGroupAiCallback::GetCurrentUnitCommands(int unitid)
{
	if(group->units.find(uh->units[unitid])!=group->units.end()){
		return &(uh->units[unitid]->commandAI->commandQue);
	}
	return 0;
}

int CGroupAiCallback::GetUnitAihint(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->aihint;
	}
	return 0;
}

int CGroupAiCallback::GetUnitTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->team;
	}
	return 0;	
}

int CGroupAiCallback::GetUnitAllyTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->allyteam;
	}
	return 0;	
}

float CGroupAiCallback::GetUnitHealth(int unitid)			//the units current health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->health;
	}
	return 0;
}

float CGroupAiCallback::GetUnitMaxHealth(int unitid)		//the units max health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->maxHealth;
	}
	return 0;
}

float CGroupAiCallback::GetUnitSpeed(int unitid)				//the units max speed
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->unitDef->speed;
	}
	return 0;
}

float CGroupAiCallback::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->power;
	}
	return 0;
}

float CGroupAiCallback::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->experience;
	}
	return 0;
}

float CGroupAiCallback::GetUnitMaxRange(int unitid)		//the furthest any weapon of the unit can fire
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->maxRange;
	}
	return 0;
}

const UnitDef* CGroupAiCallback::GetUnitDef(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & LOS_INLOS)){
		return unit->unitDef;
	}
	return 0;
}

const UnitDef* CGroupAiCallback::GetUnitDef(const char* unitName)
{
	return unitDefHandler->GetUnitByName(unitName);
}

float3 CGroupAiCallback::GetUnitPos(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (unit->losStatus[gs->team2allyteam[group->handler->team]] & (LOS_INLOS|LOS_INRADAR))){
		return helper->GetUnitErrorPos(unit,gs->team2allyteam[group->handler->team]);
	}
	return ZeroVector;
}


int CGroupAiCallback::InitPath(float3 start,float3 end,int pathType)
{
	return pathManager->RequestPath(moveinfo->moveData.at(pathType),start,end);
}

float3 CGroupAiCallback::GetNextWaypoint(int pathid)
{
	return pathManager->NextWaypoint(pathid,ZeroVector);
}

void CGroupAiCallback::FreePath(int pathid)
{
	pathManager->DeletePath(pathid);
}

float CGroupAiCallback::GetPathLength(float3 start,float3 end,int pathType)
{
//	return pathfinder->GetPathLength(start,end,pathType);
	return 0;
}

int CGroupAiCallback::GetEnemyUnits(int *units)
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

int CGroupAiCallback::GetEnemyUnits(int *units,const float3& pos,float radius)
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

int CGroupAiCallback::GetFriendlyUnits(int *units)
{
	int a=0;

	for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
		if(gs->allies[(*ui)->allyteam][gs->team2allyteam[group->handler->team]]){
			units[a++]=(*ui)->id;
		}
	}
	return a;
}

int CGroupAiCallback::GetFriendlyUnits(int *units,const float3& pos,float radius)
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


int CGroupAiCallback::GetMapWidth()
{
	return gs->mapx;
}

int CGroupAiCallback::GetMapHeight()
{
	return gs->mapy;
}

/*const unsigned char* CGroupAiCallback::GetSupplyMap()
{
	return supplyhandler->supplyLevel[gu->myTeam];
}

const unsigned char* CGroupAiCallback::GetTeamMap()
{
	return readmap->teammap;
}*/

const float* CGroupAiCallback::GetHeightMap()
{
	return readmap->centerheightmap;
}

const unsigned short* CGroupAiCallback::GetLosMap()
{
	return loshandler->losMap[gs->team2allyteam[group->handler->team]];
}

const unsigned short* CGroupAiCallback::GetRadarMap()
{
	return radarhandler->radarMaps[gs->team2allyteam[group->handler->team]];
}

const unsigned short* CGroupAiCallback::GetJammerMap()
{
	return radarhandler->jammerMaps[gs->team2allyteam[group->handler->team]];
}

const unsigned char* CGroupAiCallback::GetMetalMap()
{
	return readmap->metalMap->metalMap;
}

float CGroupAiCallback::GetElevation(float x,float z)
{
	return ground->GetHeight(x,z);
}

int CGroupAiCallback::CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddSpline(pos1,pos2,pos3,pos4,width,arrow,lifetime,group);
}

int CGroupAiCallback::CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group)
{
	return geometricObjects->AddLine(pos1,pos2,width,arrow,lifetime,group);
}

void CGroupAiCallback::SetFigureColor(int group,float red,float green,float blue,float alpha)
{
	geometricObjects->SetColor(group,red,green,blue,alpha);
}

void CGroupAiCallback::DeleteFigureGroup(int group)
{
	geometricObjects->DeleteGroup(group);
}

void CGroupAiCallback::DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder)
{
	CUnitDrawer::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitByName(name);
	if(!tdu.unitdef){
		info->AddLine("Uknown unit in CGroupAiCallback::DrawUnit %s",name);
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

bool CGroupAiCallback::CanBuildAt(const UnitDef* unitDef,float3 pos)
{
	//todo fix so you cant detect enemy buildings etc with this
	CFeature* f;
	return !!uh->TestUnitBuildSquare(pos,unitDef,f);
}

float3 CGroupAiCallback::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist)
{
	//todo fix so you cant detect enemy buildings with it
	float bestDist=searchRadius;
	float3 bestPos(-1,0,0);

	CFeature* feature;
	int allyteam=gs->team2allyteam[group->handler->team];

	for(float z=pos.z-searchRadius;z<pos.z+searchRadius;z+=SQUARE_SIZE*2){
		for(float x=pos.x-searchRadius;x<pos.x+searchRadius;x+=SQUARE_SIZE*2){
			float3 p(x,0,z);
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

float CGroupAiCallback::GetMetal()
{
	return gs->teams[gu->myTeam]->metal;
}

float CGroupAiCallback::GetMetalIncome()
{
	return gs->teams[gu->myTeam]->oldMetalIncome;
}

float CGroupAiCallback::GetMetalUsage()
{
	return gs->teams[gu->myTeam]->oldMetalExpense;	
}

float CGroupAiCallback::GetMetalStorage()
{
	return gs->teams[gu->myTeam]->metalStorage;
}

float CGroupAiCallback::GetEnergy()
{
	return gs->teams[gu->myTeam]->energy;
}

float CGroupAiCallback::GetEnergyIncome()
{
	return gs->teams[gu->myTeam]->oldEnergyIncome;
}

float CGroupAiCallback::GetEnergyUsage()
{
	return gs->teams[gu->myTeam]->oldEnergyExpense;	
}

float CGroupAiCallback::GetEnergyStorage()
{
	return gs->teams[gu->myTeam]->energyStorage;
}
