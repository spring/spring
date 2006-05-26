#include "StdAfx.h"
#include "CameraController.h"
#include "Camera.h"
#include "Map/Ground.h"
#include "UI/InfoConsole.h"
#include "UI/MouseHandler.h"
#include "Platform/ConfigHandler.h"
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
	mouseScale = atof(configHandler.GetString("FPSMouseScale", DEFAULT_MOUSE_SCALE).c_str());
	scrollSpeed=1;
	enabled=true;
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
	scrollSpeed=configHandler.GetInt("FPSScrollSpeed",10)*0.1;
	enabled=!!configHandler.GetInt("FPSEnabled",1)*0.1;
}

void CFPSController::KeyMove(float3 move)
{
	move*=move.z*400;
	pos+=(camera->forward*move.y+camera->right*move.x)*scrollSpeed;
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
{
	scrollSpeed=configHandler.GetInt("OverheadScrollSpeed",10)*0.1;
	enabled=!!configHandler.GetInt("OverheadEnabled",1)*0.1;
}

void COverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*200;
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->screeny*height*2;
	pos.x+=move.x*pixelsize*2*scrollSpeed;	
	pos.z-=move.y*pixelsize*2*scrollSpeed;
}

void COverheadController::MouseMove(float3 move)
{
	float pixelsize=100*mouseScale*tan(camera->fov/180/2*PI)*2/gu->screeny*height*2;
	pos.x+=move.x*pixelsize*(1+keys[SDLK_LSHIFT]*3)*scrollSpeed;	
	pos.z+=move.y*pixelsize*(1+keys[SDLK_LSHIFT]*3)*scrollSpeed;
}

void COverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void COverheadController::MouseWheelMove(float move)
{
	// tilt the camera if LCTRL is pressed
	if (keys[SDLK_LCTRL]) {
		zscale *= 1+move * mouseScale;
		if (zscale < 0.05) zscale = 0.05f;
		if (zscale > 10) zscale = 10;
	} else { // holding down LALT uses 'instant-zoom' from here to the end of the function
		//ZOOM IN to mouse cursor instead of mid screen
		if(move<0){
			float3 cpos=pos-dir*height;
			float dif=-height * move * mouseScale*0.7 * (keys[SDLK_LSHIFT] ? 3:1);
			if(height-dif<60)
				dif=height-60;
			if (keys[SDLK_LALT]) //instant-zoom: zoom in to standard view
				dif=height-500;
			float3 wantedPos= cpos + mouse->dir * dif;
			float newHeight=ground->LineGroundCol(wantedPos,wantedPos+dir*10000);
			if(newHeight<0)
				newHeight=height* (1+move * mouseScale*0.7 * (keys[SDLK_LSHIFT] ? 3:1));
			if(keys[SDLK_LALT] && newHeight < 500) // instant-zoom: set the new height to 500 if we are too low
				newHeight=500;
			if(wantedPos.y + dir.y * newHeight <0)
				newHeight = -wantedPos.y / dir.y;
			if(newHeight<3000){
				height=newHeight;
				pos= wantedPos + dir * height;
			}
		//ZOOM OUT from mid screen
		} else {
			if (keys[SDLK_LALT]) { // instant-zoom: zoom out to the max
				height=3000;
			} else {
				height*=1+move * mouseScale*0.7 * (keys[SDLK_LSHIFT] ? 3:1);
			}
		}
		// instant-zoom: turn on the smooth transition and reset the camera tilt
		if(keys[SDLK_LALT]){
			zscale=0.5f;
			mouse->inStateTransit=true;
			mouse->transitSpeed=1;
		}
	}
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
	scrollSpeed=configHandler.GetInt("TWScrollSpeed",10)*0.1;
	enabled=!!configHandler.GetInt("RotOverheadEnabled",1)*0.1;
}

void CTWController::KeyMove(float3 move)
{
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.Normalize();

	move*=sqrt(move.z)*200;
	pos+=(flatForward*move.y+camera->right*move.x)*scrollSpeed;
}

void CTWController::MouseMove(float3 move)
{
	float dist=-camera->rot.x*1500;
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->screeny*dist*2;
	move*=(1+keys[SDLK_LSHIFT]*3)*pixelsize;
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.Normalize();

	pos+=-(flatForward*move.y+camera->right*move.x)*scrollSpeed;
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
	scrollSpeed=configHandler.GetInt("RotOverheadScrollSpeed",10)*0.1;
	enabled=!!configHandler.GetInt("TWEnabled",1)*0.1;
}

void CRotOverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*400;

	float3 flatForward=camera->forward;
	if(camera->forward.y<-0.9)
		flatForward+=camera->up;
	flatForward.y=0;
	flatForward.Normalize();

	pos+=(flatForward*move.y+camera->right*move.x)*scrollSpeed;
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
	height*=1+move*mouseScale;
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

