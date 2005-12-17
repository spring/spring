#include "System/StdAfx.h"
#include "CameraController.h"
#include "Camera.h"
#include "Sim/Map/Ground.h"
#include "UI/InfoConsole.h"
#include "UI/MouseHandler.h"
#include "System/Platform/ConfigHandler.h"
#include "SDL_types.h"
#include "SDL_keysym.h"

extern Uint8 *keys;

#ifdef _WIN32
#define DEFAULT_MOUSE_SCALE "0.01"
#else
#define DEFAULT_MOUSE_SCALE "0.003"
#endif

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
	mouseScale = atof(configHandler.GetString("FPSMouseScale", DEFAULT_MOUSE_SCALE).c_str());
}

void CFPSController::KeyMove(float3 move)
{
	move*=move.z*400;
	pos+=camera->forward*move.y+camera->right*move.x;
}

void CFPSController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale*move.x;
	camera->rot.x -= mouseScale*move.y*move.z;  

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
	if(pos.y>3000)
		pos.y=3000;

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
	height(500),zscale(0.5f)
{}

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
	pos.x+=move.x*pixelsize*(1+keys[SDLK_LSHIFT]*3);	
	pos.z+=move.y*pixelsize*(1+keys[SDLK_LSHIFT]*3);
}

void COverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void COverheadController::MouseWheelMove(float move)
{
	if (keys[SDLK_LCTRL]) {
		zscale *= 1+move*0.002;
		if (zscale < 0.05) zscale = 0.05f;
		if (zscale > 10) zscale = 10;
	} else
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
	if(height>3000)
		height=3000;

	pos.y=ground->GetHeight(pos.x,pos.z);
	dir=float3(0,-1,-zscale).Normalize();
	
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
	move*=(1+keys[SDLK_LSHIFT]*3)*pixelsize;
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

/////////////////////
//Rotating Overhead Controller
/////////////////////

CRotOverheadController::CRotOverheadController()
: pos(2000,70,1800),
	oldHeight(500)
{
	mouseScale = atof(configHandler.GetString("RotOverheadMouseScale", DEFAULT_MOUSE_SCALE).c_str());
}

void CRotOverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*400;

	float3 flatForward=camera->forward;
	if(camera->forward.y<-0.9)
		flatForward+=camera->up;
	flatForward.y=0;
	flatForward.Normalize();

	pos+=flatForward*move.y+camera->right*move.x;
}

void CRotOverheadController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale*move.x;
	camera->rot.x -= mouseScale*move.y*move.z;  

	if(camera->rot.x>PI*0.4999)
		camera->rot.x=PI*0.4999;
	if(camera->rot.x<-PI*0.4999)
		camera->rot.x=-PI*0.4999;
}

void CRotOverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void CRotOverheadController::MouseWheelMove(float move)
{
	float gheight=ground->GetHeight(pos.x,pos.z);
	float height=pos.y-gheight;
	height*=1+move*0.001;
	pos.y=height+gheight;
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
	if(pos.y>3000)
		pos.y=3000;

	oldHeight=pos.y-ground->GetHeight(pos.x,pos.z);

	return pos;
}

float3 CRotOverheadController::GetDir()
{
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.Normalize();
	return dir;
}

void CRotOverheadController::SetPos(float3 newPos)
{
	pos=newPos;
	pos.y=ground->GetHeight(pos.x,pos.z)+oldHeight;
}

float3 CRotOverheadController::SwitchFrom()
{
	return pos;
}

void CRotOverheadController::SwitchTo()
{
	info->AddLine("Switching to rotatable overhead camera");
}

