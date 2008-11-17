#include "StdAfx.h"
#include "mmgr.h"

#include "RotOverheadController.h"

#include "ConfigHandler.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "Map/Ground.h"


CRotOverheadController::CRotOverheadController()
	: oldHeight(500)
{
	mouseScale = configHandler.GetFloat("RotOverheadMouseScale", 0.01f);
	scrollSpeed = configHandler.Get("RotOverheadScrollSpeed",10)*0.1f;
	enabled=!!configHandler.Get("RotOverheadEnabled",1);
	fov = configHandler.GetFloat("RotOverheadFOV", 45.0f);
}


void CRotOverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*400;

	float3 flatForward=camera->forward;
	if(camera->forward.y<-0.9f)
		flatForward+=camera->up;
	flatForward.y=0;
	flatForward.ANormalize();

	pos+=(flatForward*move.y+camera->right*move.x)*scrollSpeed;
}


void CRotOverheadController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale*move.x;
	camera->rot.x -= mouseScale*move.y*move.z;

	if(camera->rot.x>PI*0.4999f)
		camera->rot.x=PI*0.4999f;
	if(camera->rot.x<-PI*0.4999f)
		camera->rot.x=-PI*0.4999f;
}


void CRotOverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}


void CRotOverheadController::MouseWheelMove(float move)
{
	const float gheight = ground->GetHeight(pos.x,pos.z);
	float height = pos.y - gheight;
	height *= 1.0f + (move * mouseScale);
	pos.y = height + gheight;
}


float3 CRotOverheadController::GetPos()
{
	if(pos.x<0.01f)
		pos.x=0.01f;
	if(pos.z<0.01f)
		pos.z=0.01f;
	if(pos.x>(gs->mapx)*SQUARE_SIZE-0.01f)
		pos.x=(gs->mapx)*SQUARE_SIZE-0.01f;
	if(pos.z>(gs->mapy)*SQUARE_SIZE-0.01f)
		pos.z=(gs->mapy)*SQUARE_SIZE-0.01f;
	if(pos.y<ground->GetHeight(pos.x,pos.z)+5)
		pos.y=ground->GetHeight(pos.x,pos.z)+5;
	if(pos.y>9000)
		pos.y=9000;

	oldHeight = pos.y - ground->GetHeight(pos.x,pos.z);

	return pos;
}


float3 CRotOverheadController::GetDir()
{
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.ANormalize();
	return dir;
}


void CRotOverheadController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);
	pos.y = ground->GetHeight(pos.x, pos.z) + oldHeight;
}


float3 CRotOverheadController::SwitchFrom() const
{
	return pos;
}


void CRotOverheadController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Rotatable overhead camera");
}


void CRotOverheadController::GetState(StateMap& sm) const
{
	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;

	sm["dx"] = dir.x;
	sm["dy"] = dir.y;
	sm["dz"] = dir.z;

	sm["rx"] = camera->rot.x;
	sm["ry"] = camera->rot.y;
	sm["rz"] = camera->rot.z;

	sm["oldHeight"] = oldHeight;
}


bool CRotOverheadController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);

	SetStateFloat(sm, "dx", dir.x);
	SetStateFloat(sm, "dy", dir.y);
	SetStateFloat(sm, "dz", dir.z);

	SetStateFloat(sm, "rx", camera->rot.x);
	SetStateFloat(sm, "ry", camera->rot.y);
	SetStateFloat(sm, "rz", camera->rot.z);

	SetStateFloat(sm, "oldHeight", oldHeight);

	return true;
}

