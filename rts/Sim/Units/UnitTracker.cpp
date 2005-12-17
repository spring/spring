#include "StdAfx.h"
#include "UnitTracker.h"
#include "Game/SelectedUnits.h"
#include "Game/Camera.h"
#include "math.h"
#include "Unit.h"
#include "Game/UI/MouseHandler.h"
#include "UnitHandler.h"
#include "Platform/ConfigHandler.h"
#include "Sim/Map/Ground.h"
#include "Game/CameraController.h"
//#include "mmgr.h"

CUnitTracker unitTracker;

CUnitTracker::CUnitTracker(void)
:	tcp(500,100,500),
	tcf(0,0,1),
	lastUpdateTime(0),
	timeOut(15),
	oldCamPos(500,500,500),
	oldCamDir(1,0,0),
	lastFollowUnit(0)
{
	for(int a=0;a<32;++a)
		oldUp[a]=UpVector;

	doRoll=false;
	firstUpdate=true;
}

CUnitTracker::~CUnitTracker(void)
{
}

void CUnitTracker::SetCam(void)
{
	if(firstUpdate){
		firstUpdate=false;
		doRoll=!configHandler.GetInt("ReflectiveWater",1);
	}
	if(lastFollowUnit!=0 && uh->units[lastFollowUnit]==0){
		timeOut=1;
		lastFollowUnit=0;
	}


	if(timeOut>0){
		timeOut++;
		camera->forward=oldCamDir;
		camera->pos=oldCamPos;
		((CFPSController*)mouse->camControllers[0])->dir=oldCamDir;
		((CFPSController*)mouse->camControllers[0])->pos=oldCamPos;
		if(timeOut>15)
			timeOut=0;
		return;
	}

	if(selectedUnits.selectedUnits.empty())
		return;

	CUnit* u=*selectedUnits.selectedUnits.begin();

	if(mouse->currentCamController!=mouse->camControllers[0]){
		mouse->currentCamController->SetPos(u->midPos);
		return;
	}

	float deltaTime=gs->frameNum+gu->timeOffset-lastUpdateTime;
	lastUpdateTime=gs->frameNum+gu->timeOffset;

	float3 modPlanePos=u->midPos-u->frontdir*u->radius*3+u->speed*gu->timeOffset;
	if(modPlanePos.y<ground->GetHeight2(modPlanePos.x,modPlanePos.z)+u->radius*2)
		modPlanePos.y=ground->GetHeight2(modPlanePos.x,modPlanePos.z)+u->radius*2;

	tcp+=(modPlanePos-tcp)*(1-pow(0.95f,deltaTime));
	tcf+=(u->frontdir-tcf)*(1-pow(0.9f,deltaTime));
	tcf.Normalize();
	
	camera->pos=tcp;
	((CFPSController*)mouse->camControllers[0])->pos=tcp;

	//camera->forward=((u->pos+u->speed*gu->timeOffset-camera->pos).Normalize()+u->frontdir).Normalize();
	camera->forward=((u->pos+u->speed*gu->timeOffset-camera->pos).Normalize()+tcf.Normalize());
	camera->forward.Normalize();
	((CFPSController*)mouse->camControllers[0])->dir=camera->forward;

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
