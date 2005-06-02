#include "StdAfx.h"
#include "CameraController.h"
#include "Camera.h"
#include "Ground.h"
#include <windows.h>
#include "InfoConsole.h"
#include "MouseHandler.h"

extern bool keys[256];

CCameraController::CCameraController(void)
{
}

CCameraController::~CCameraController(void)
{
}

/////////////////////
//FPS Controller
/////////////////////

CFPSController::CFPSController()
: pos(2000,70,1800)
{

}

void CFPSController::KeyMove(float3 move)
{
	move*=move.z*400;
	pos+=camera->forward*move.y+camera->right*move.x;
}

void CFPSController::MouseMove(float3 move)
{
	camera->rot.y -= 0.01*move.x;
	camera->rot.x -= 0.01*move.y*move.z;  

	if(camera->rot.x>PI*0.4999)
		camera->rot.x=PI*0.4999;
	if(camera->rot.x<-PI*0.4999)
		camera->rot.x=-PI*0.4999;
}

void CFPSController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void CFPSController::MouseWheelMove(float move)
{
	pos+=camera->up*move;
}

float3 CFPSController::GetPos()
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
	if(pos.y>2500)
		pos.y=2500;

	oldHeight=pos.y-ground->GetHeight(pos.x,pos.z);
	return pos;
}

float3 CFPSController::GetDir()
{
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.Normalize();
	return dir;
}

void CFPSController::SetPos(float3 newPos)
{
	pos=newPos;
	pos.y=ground->GetHeight(pos.x,pos.z)+oldHeight;
}

float3 CFPSController::SwitchFrom()
{
	return pos;
}

void CFPSController::SwitchTo()
{
	info->AddLine("Switching to FPS style camera");
}

/////////////////////
//Overhead Controller
/////////////////////

COverheadController::COverheadController()
: pos(2000,70,1800),
	height(500)
{
	dir=float3(0,-2,-1).Normalize();
}

void COverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*200;
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->screeny*height*2;
	pos.x+=move.x*pixelsize*2;	
	pos.z-=move.y*pixelsize*2;
}

void COverheadController::MouseMove(float3 move)
{
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->screeny*height*2;
#ifdef USE_GLUT
	int dep = (int)(pixelsize*(1+((glutGetModifiers()==GLUT_ACTIVE_SHIFT)?3:0)));
	pos.x+=move.x*dep;	
	pos.z+=move.y*dep;
#else
	pos.x+=move.x*pixelsize*(1+keys[VK_SHIFT]*3);	
	pos.z+=move.y*pixelsize*(1+keys[VK_SHIFT]*3);
#endif
}

void COverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void COverheadController::MouseWheelMove(float move)
{
	height*=1+move*0.001;
}

float3 COverheadController::GetPos()
{
	if(pos.x<0.01f)
		pos.x=0.01f;
	if(pos.z<0.01f)
		pos.z=0.01f;
	if(pos.x>(gs->mapx)*SQUARE_SIZE-0.01f)
		pos.x=(gs->mapx)*SQUARE_SIZE-0.01f;
	if(pos.z>(gs->mapy)*SQUARE_SIZE-0.01f)
		pos.z=(gs->mapy)*SQUARE_SIZE-0.01f;
	if(height<60)
		height=60;
	if(height>2500)
		height=2500;

	pos.y=ground->GetHeight(pos.x,pos.z);
	
	float3 cpos=pos-dir*height;

	return cpos;
}

float3 COverheadController::GetDir()
{
	return dir;
}

void COverheadController::SetPos(float3 newPos)
{
	pos=newPos;
}

float3 COverheadController::SwitchFrom()
{
	return pos;
}

void COverheadController::SwitchTo()
{
	info->AddLine("Switching to overhead (TA) style camera");
}

/////////////////////
//Total war Controller
/////////////////////

CTWController::CTWController()
: pos(2000,70,1800)
{
}

void CTWController::KeyMove(float3 move)
{
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.Normalize();

	move*=sqrt(move.z)*200;
	pos+=flatForward*move.y+camera->right*move.x;
}

void CTWController::MouseMove(float3 move)
{
	float dist=-camera->rot.x*1500;
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->screeny*dist*2;
#ifdef USE_GLUT
	move*=pixelsize*(1+((glutGetModifiers()==GLUT_ACTIVE_SHIFT)?3:0));
#else
	move*=(1+keys[VK_SHIFT]*3)*pixelsize;
#endif
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.Normalize();

	pos+=-flatForward*move.y+camera->right*move.x;
}

void CTWController::ScreenEdgeMove(float3 move)
{
	if(mouse->lasty<gu->screeny/3){
		camera->rot.y-=move.x*gu->lastFrameTime*0.5*200;
		move.x=0;
	}
	KeyMove(move);
}

void CTWController::MouseWheelMove(float move)
{
	camera->rot.x-=move*0.001;
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
	if(camera->rot.x>-0.1)
		camera->rot.x=-0.1;
	if(camera->rot.x<-PI*0.4)
		camera->rot.x=-PI*0.4;

	pos.y=ground->GetHeight(pos.x,pos.z);

	float3 dir;
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.Normalize();

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
	dir.Normalize();
	return dir;
}

void CTWController::SetPos(float3 newPos)
{
	pos=newPos;
}

float3 CTWController::SwitchFrom()
{
	return pos;
}

void CTWController::SwitchTo()
{
	info->AddLine("Switching to Total War style camera");
}

