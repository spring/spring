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
#include "guicontroller.h"
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
	info->AddLine("Group%i: %s",group->id,text);
}

int CGroupAiCallback::GetCurrentFrame()
{
	return gs->frameNum;
}

int CGroupAiCallback::GetMyTeam()
{
	return gu->myTeam;
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

Command CGroupAiCallback::GetOrderPreview()
{
	//todo: need to add support for new gui
#ifdef NEW_GUI
	return guicontroller->GetOrderPreview();
#else
	return guihandler->GetOrderPreview();
#endif
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

		
//todo: include proper testing of los rules

int CGroupAiCallback::GetUnitAihint(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && loshandler->InLos(unit,gu->myAllyTeam)){
		return unit->aihint;
	}
	return 0;
}

int CGroupAiCallback::GetUnitTeam(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && loshandler->InLos(unit,gu->myAllyTeam)){
		return unit->team;
	}
	return 0;	
}

float CGroupAiCallback::GetUnitHealth(int unitid)			//the units current health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
		return unit->health;
	}
	return 0;
}

float CGroupAiCallback::GetUnitMaxHealth(int unitid)		//the units max health
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
		return unit->maxHealth;
	}
	return 0;
}

float CGroupAiCallback::GetUnitSpeed(int unitid)				//the units max speed
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
		return unit->unitDef->speed;
	}
	return 0;
}

float CGroupAiCallback::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
		return unit->power;
	}
	return 0;
}

float CGroupAiCallback::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
		return unit->experience;
	}
	return 0;
}

float CGroupAiCallback::GetUnitMaxRange(int unitid)		//the furthest any weapon of the unit can fire
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
		return unit->maxRange;
	}
	return 0;
}

const UnitDef* CGroupAiCallback::GetUnitDef(int unitid)
{
	CUnit* unit=uh->units[unitid];
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam))){
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
	if(unit && (gs->allies[gu->myAllyTeam][unit->allyteam] || loshandler->InLos(unit,gu->myAllyTeam) || radarhandler->InRadar(unit,gu->myAllyTeam))){
		return helper->GetUnitErrorPos(unit,gu->myAllyTeam);
	}
	return ZeroVector;
}


int CGroupAiCallback::InitPath(float3 start,float3 end,int pathType)
{
//	return pathHandler->RequestPath(start,end,pathType,4,0);
	return 0;
}

float3 CGroupAiCallback::GetNextWaypoint(int pathid)
{
//	return pathfinder->GetNextWaypoint(pathid);
	return float3(0, 0, 0);
}

void CGroupAiCallback::FreePath(int pathid)
{
//	pathfinder->DeletePath(pathid);
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
		if(!gs->allies[(*ui)->allyteam][gu->myAllyTeam] && loshandler->InLos((*ui),gu->myAllyTeam)){
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
		if((*ui)->team!=gu->myTeam && loshandler->InLos(*ui,gu->myTeam)){
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
		if(gs->allies[(*ui)->allyteam][gu->myAllyTeam]){
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
	return loshandler->losMap[gu->myAllyTeam];
}

const unsigned short* CGroupAiCallback::GetRadarMap()
{
	return radarhandler->radarMaps[gu->myAllyTeam];
}

const unsigned short* CGroupAiCallback::GetJammerMap()
{
	return radarhandler->jammerMaps[gu->myAllyTeam];
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
	CUnitHandler::TempDrawUnit tdu;
	tdu.unitdef=unitDefHandler->GetUnitByName(name);
	if(!tdu.unitdef){
		info->AddLine("Uknown unit in CGroupAiCallback::DrawUnit %s",name);
		return;
	}
	tdu.pos=pos;
	tdu.rot=rotation;
	tdu.team=team;
	tdu.drawBorder=drawBorder;
	std::pair<int,CUnitHandler::TempDrawUnit> tp(gs->frameNum+lifetime,tdu);
	if(transparent)
		uh->tempTransperentDrawUnits.insert(tp);
	else
		uh->tempDrawUnits.insert(tp);
}

bool CGroupAiCallback::CanBuildAt(const UnitDef* unitDef,float3 pos)
{
	return true;
}

float CGroupAiCallback::GetMetal()
{
	return gs->teams[gu->myTeam]->metal;
}

float CGroupAiCallback::GetEnergy()
{
	return gs->teams[gu->myTeam]->energy;
}

float CGroupAiCallback::GetMetalStorage()
{
	return gs->teams[gu->myTeam]->metalStorage;
}

float CGroupAiCallback::GetEnergyStorage()
{
	return gs->teams[gu->myTeam]->energyStorage;
}
