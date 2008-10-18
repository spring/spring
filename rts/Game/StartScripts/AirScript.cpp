#include "StdAfx.h"
#include "mmgr.h"

#include "AirScript.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Game/Camera.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Platform/ConfigHandler.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "System/GlobalUnsynced.h"



const char* armhawk_name[] = { "arm_hawk", "ARMHAWK", NULL };
const char* armfig_name [] = { "arm_freedom_fighter", "ARMFIG", NULL };
const char* corvamp_name[] = { "core_vamp", "CORVAMP", NULL };
const char* corape_name [] = { "core_rapier", "CORAPE", NULL };

CAirScript::CAirScript()
: CScript(std::string("Air combat test")),
	tcp(500,100,500),
	tcf(0,0,1),
	lastUpdateTime(0),
	timeOut(15),
	oldCamPos(500,500,500),
	oldCamDir(1,0,0)
{
	curPlane=0;
	for(int a=0;a<32;++a)
		oldUp[a]=UpVector;

	doRoll=false;
}

CAirScript::~CAirScript()
{
}

static const char* FindUnit(const char** name)
{
	for (int i = 0; name[i]; ++i) {
		if (unitDefHandler->GetUnitByName(name[i]))
			return name[i];
	}
	return name[0];
}

void CAirScript::GameStart()
{
	doRoll=!configHandler.GetInt("ReflectiveWater",1);

	ENTER_MIXED;
	tcp=camera->pos;
	tcf=camera->forward;
	ENTER_SYNCED;
	for(int a=0;a<10;++a){
		CUnit* u;
		if(gs->randFloat()<0.5f)
			u=unitLoader.LoadUnit(FindUnit(armhawk_name),float3(1650,300,2100+a*150),0,false,0,NULL);
		else
			u=unitLoader.LoadUnit(FindUnit(armfig_name),float3(1650,300,2100+a*150),0,false,0,NULL);
		u->pos.y=350;
		u->experience=0.3f;
		((CAirMoveType*)u->moveType)->SetState(AAirMoveType::AIRCRAFT_FLYING);
		planes.push_back(u->id);
		Command c2;
		c2.id=CMD_MOVE_STATE;
		c2.params.push_back(1);
		u->commandAI->GiveCommand(c2);

		Command c;
		c.id=CMD_PATROL;
		c.options=0;
		c.params.push_back(6570);
		c.params.push_back(0);
		c.params.push_back(2560);
		u->commandAI->GiveCommand(c);

		if(gs->randFloat()<0.5f){
			u=unitLoader.LoadUnit(FindUnit(corvamp_name),float3(3880,300,2100+a*150),1,false,0,NULL);
			((CAirMoveType*)u->moveType)->SetState(AAirMoveType::AIRCRAFT_FLYING);
		}else{
			u=unitLoader.LoadUnit(FindUnit(corape_name),float3(3880,300,2100+a*150),1,false,0,NULL);
		}
		u->pos.y=350;
		u->experience=0.3f;
		planes.push_back(u->id);
		u->commandAI->GiveCommand(c2);
		c.params[0]=500;
		u->commandAI->GiveCommand(c);
	}
}

void CAirScript::Update()
{
	std::deque<int>::iterator pi;
	int num=0;
	for(pi=planes.begin();pi!=planes.end();++pi){
		if(uh->units[*pi]==0){
			CUnit* u;
			if(!(num&1))
				u=unitLoader.LoadUnit(FindUnit(armhawk_name),float3(1000+(num&1)*5000,500,2100+num*120),(num&1),false,0,NULL);
			else
				u=unitLoader.LoadUnit(FindUnit(corvamp_name),float3(1000+(num&1)*5000,500,2100+num*120),(num&1),false,0,NULL);
			u->pos.y=ground->GetHeight(1000+(num&1)*5000,2100+num*120)+350;
			u->experience=0.3f;
			u->speed.x=2.8f;
			((CAirMoveType*)u->moveType)->SetState(AAirMoveType::AIRCRAFT_FLYING);
			*pi=u->id;

			Command c;
			c.id=CMD_PATROL;
			c.options=0;
			c.params.push_back(7000-(num&1)*6500);
			c.params.push_back(200);
			c.params.push_back(2500+num*60);

			Command c2;
			c2.id=CMD_MOVE_STATE;
			c2.params.push_back(1);
			u->commandAI->GiveCommand(c2);
			u->commandAI->GiveCommand(c);
		}
		num++;
	}
}

void CAirScript::SetCamera(void)
{
	if(curPlane==0 || uh->units[curPlane]==0){
		std::deque<int>::iterator pi;
		for(pi=planes.begin();pi!=planes.end();++pi){
			if(uh->units[*pi]!=0){
				curPlane=*pi;
				break;
			}
		}
		timeOut=std::min(1,timeOut+1);
	}
	if(timeOut>0){
		timeOut++;
		camera->forward=oldCamDir;
		camera->pos=oldCamPos;
		if(timeOut>15)
			timeOut=0;
		return;
	}

	if(uh->units[curPlane]==0)
		return;

	//CAircraft* u=(CAircraft*)uh->units[curPlane];
	CUnit *u = uh->units[curPlane];

	float deltaTime=gs->frameNum+gu->timeOffset-lastUpdateTime;
	lastUpdateTime=gs->frameNum+gu->timeOffset;

	float3 modPlanePos=u->pos-u->frontdir*25+u->speed*gu->timeOffset;
	tcp+=(modPlanePos-tcp)*(1-pow(0.95f,deltaTime));
	tcf+=(u->frontdir-tcf)*(1-pow(0.9f,deltaTime));
	tcf.Normalize();

	camera->pos=tcp;
	//camera->forward=((u->pos+u->speed*gu->timeOffset-camera->pos).Normalize()+u->frontdir).Normalize();
	camera->forward=((u->pos+u->speed*gu->timeOffset-camera->pos).Normalize()+tcf.Normalize());
	camera->forward.Normalize();

	if(doRoll){
		oldUp[gs->frameNum%32]=u->updir;
		float3 up(0,0,0);
		for(int a=0;a<32;++a)
			up+=oldUp[a];
		camera->up=up;
	} else {
		camera->up=UpVector;
	}
	oldCamDir=camera->forward;
	oldCamPos=camera->pos;
}
