#include "StdAfx.h"

#include <SDL_keysym.h>

#include "mmgr.h"

#include "TWController.h"

#include "ConfigHandler.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Game/UI/MouseHandler.h"
#include "GlobalUnsynced.h"

extern uint8_t *keys;


CTWController::CTWController()
{
	scrollSpeed = configHandler->Get("TWScrollSpeed",10) * 0.1f;
	enabled = !!configHandler->Get("TWEnabled",1);
	fov = configHandler->Get("TWFOV", 45.0f);
}


void CTWController::KeyMove(float3 move)
{
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.ANormalize();

	move*=sqrt(move.z)*200;
	pos+=(flatForward*move.y+camera->right*move.x)*scrollSpeed;
}


void CTWController::MouseMove(float3 move)
{
	float dist=-camera->rot.x*1500;
	float pixelsize=camera->GetTanHalfFov()*2/gu->viewSizeY*dist*2;
	move*=(1+keys[SDLK_LSHIFT]*3)*pixelsize;
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.ANormalize();

	pos+=-(flatForward*move.y-camera->right*move.x)*scrollSpeed;
}


void CTWController::ScreenEdgeMove(float3 move)
{
	if(mouse->lasty<gu->viewSizeY/3){
		camera->rot.y-=move.x*gu->lastFrameTime*0.5f*200;
		move.x=0;
	}
	KeyMove(move);
}


void CTWController::MouseWheelMove(float move)
{
	camera->rot.x-=move*0.001f;
}


float3 CTWController::GetPos()
{
	if(pos.x<0.01f)
		pos.x=0.01f;
	if(pos.z<0.01f)
		pos.z=0.01f;
	if(pos.x>(gs->mapx)*SQUARE_SIZE-0.01f)
		pos.x=(gs->mapx)*SQUARE_SIZE-0.01f;
	if(pos.z>(gs->mapy)*SQUARE_SIZE-0.01f)
		pos.z=(gs->mapy)*SQUARE_SIZE-0.01f;
	if(camera->rot.x>-0.1f)
		camera->rot.x=-0.1f;
	if(camera->rot.x<-PI*0.4f)
		camera->rot.x=-PI*0.4f;

	pos.y=ground->GetHeight(pos.x,pos.z);

	float3 dir;
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.ANormalize();

	float dist=-camera->rot.x*1500;

	float3 cpos=pos-dir*dist;

	if(cpos.y<ground->GetHeight(cpos.x,cpos.z)+5)
		cpos.y=ground->GetHeight(cpos.x,cpos.z)+5;

	return cpos;
}


float3 CTWController::GetDir()
{
	float3 dir;
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.ANormalize();
	return dir;
}


float3 CTWController::SwitchFrom() const
{
	return pos;
}


void CTWController::SwitchTo(bool showText)
{
	if (showText) {
		logOutput.Print("Switching to Total War style camera");
	}
}


void CTWController::GetState(StateMap& sm) const
{
	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;
	sm["rx"] = camera->rot.x;
	sm["ry"] = camera->rot.y;
	sm["rz"] = camera->rot.z;
}


bool CTWController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);
	SetStateFloat(sm, "rx", camera->rot.x);
	SetStateFloat(sm, "ry", camera->rot.y);
	SetStateFloat(sm, "rz", camera->rot.z);
	return true;
}
