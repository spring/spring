#include "StdAfx.h"
#include "CameraController.h"
#include "Camera.h"
#include "Game/Game.h"
#include "Game/UI/MiniMap.h"
#include "Map/Ground.h"
#include "Platform/ConfigHandler.h"
#include "UI/LuaUI.h"
#include "UI/MouseHandler.h"
#include "LogOutput.h"
#include "SDL_types.h"
#include "SDL_keysym.h"

extern Uint8 *keys;

#ifdef _WIN32
#define DEFAULT_MOUSE_SCALE "0.01"
#else
#define DEFAULT_MOUSE_SCALE "0.003"
#endif


static float GetConfigFloat(const string& name, const string& def)
{
	return (float)atof(configHandler.GetString(name, def).c_str());
}


CCameraController::CCameraController(int _num) : num(_num)
{
	mouseScale = GetConfigFloat("FPSMouseScale", DEFAULT_MOUSE_SCALE);
	scrollSpeed=1;
	fov = 45.0f;
	enabled=true;
}

CCameraController::~CCameraController(void)
{
}

/******************************************************************************/
/******************************************************************************/
//
//  FPS Controller
//
//

CFPSController::CFPSController(int num)
: CCameraController(num),
  pos(2000,70,1800),
  oldHeight(300)
{
	scrollSpeed = configHandler.GetInt("FPSScrollSpeed", 10) * 0.1f;
	enabled = !!configHandler.GetInt("FPSEnabled", 1);
	fov = GetConfigFloat("FPSFOV", "45.0");
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

	if(camera->rot.x>PI*0.4999f)
		camera->rot.x=PI*0.4999f;
	if(camera->rot.x<-PI*0.4999f)
		camera->rot.x=-PI*0.4999f;
}

void CFPSController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void CFPSController::MouseWheelMove(float move)
{
	pos += camera->up * move;
}

float3 CFPSController::GetPos()
{
	const float margin = 0.01f;
	const float xMin = margin;
	const float zMin = margin;
	const float xMax = (float)(gs->mapx * SQUARE_SIZE) - margin;
	const float zMax = (float)(gs->mapy * SQUARE_SIZE) - margin;

	pos.x = max(xMin, min(xMax, pos.x));
	pos.z = max(zMin, min(zMax, pos.z));

#ifdef DIRECT_CONTROL_ALLOWED
	if (!gu->directControl)
#endif
	{
		const float gndHeight = ground->GetHeight(pos.x, pos.z);
		const float yMin = gndHeight + 5.0f;
		const float yMax = 9000.0f;
		pos.y = max(yMin, min(yMax, pos.y));
		oldHeight = pos.y - gndHeight;
	}

	return pos;
}

float3 CFPSController::GetDir()
{
	dir.x = (float)(cos(camera->rot.x) * sin(camera->rot.y));
	dir.z = (float)(cos(camera->rot.x) * cos(camera->rot.y));
	dir.y = (float)(sin(camera->rot.x));
	dir.Normalize();
	return dir;
}

void CFPSController::SetPos(float3 newPos)
{
	pos = newPos;

#ifdef DIRECT_CONTROL_ALLOWED
	if (!gu->directControl)
#endif
	{
		pos.y = ground->GetHeight(pos.x, pos.z) + oldHeight;
	}
}

float3 CFPSController::SwitchFrom()
{
	return pos;
}

void CFPSController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to FPS style camera");
}

void CFPSController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/*  0 */ (float)num);
	fv.push_back(/*  1 */ pos.x);
	fv.push_back(/*  2 */ pos.y);
	fv.push_back(/*  3 */ pos.z);
	fv.push_back(/*  4 */ dir.x);
	fv.push_back(/*  5 */ dir.y);
	fv.push_back(/*  6 */ dir.z);
	fv.push_back(/*  7 */ camera->rot.x);
	fv.push_back(/*  8 */ camera->rot.y);
	fv.push_back(/*  9 */ camera->rot.z);
	fv.push_back(/* 10 */ oldHeight);
}

bool CFPSController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 11) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	dir.x = fv[4];
	dir.y = fv[5];
	dir.z = fv[6];
	camera->rot.x = fv[7];
	camera->rot.y = fv[8];
	camera->rot.z = fv[9];
	oldHeight = fv[10];
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Overhead Controller
//

COverheadController::COverheadController(int num)
: CCameraController(num),
  pos(2000,70,1800),
  height(500),zscale(0.5f),
  oldAltHeight(500),
  maxHeight(10000),
  changeAltHeight(true),
  flipped(false)
{
	scrollSpeed = configHandler.GetInt("OverheadScrollSpeed",10)*0.1f;
	tiltSpeed = GetConfigFloat("OverheadTiltSpeed","1");
	enabled = !!configHandler.GetInt("OverheadEnabled",1);
	fov = GetConfigFloat("OverheadFOV", "45.0");
}

void COverheadController::KeyMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move*=sqrt(move.z)*200;
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->viewSizeY*height*2;
	pos.x+=move.x*pixelsize*2*scrollSpeed;
	pos.z-=move.y*pixelsize*2*scrollSpeed;
}

void COverheadController::MouseMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	float pixelsize=100*mouseScale*tan(camera->fov/180/2*PI)*2/gu->viewSizeY*height*2;
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
		zscale *= (1.0f + (move * tiltSpeed * mouseScale * (keys[SDLK_LSHIFT] ? 3.0f : 1.0f)));
		if (zscale < 0.05f) zscale = 0.05f;
		if (zscale > 10) zscale = 10;
	} else { // holding down LALT uses 'instant-zoom' from here to the end of the function
		// ZOOM IN to mouse cursor instead of mid screen
		if (move < 0) {
			float3 cpos=pos-dir*height;
			float dif=-height * move * mouseScale*0.7f * (keys[SDLK_LSHIFT] ? 3:1);
			if ((height - dif) <60.0f) {
				dif = height - 60.0f;
			}
			if (keys[SDLK_LALT]) { // instant-zoom: zoom in to standard view
				dif = (height - oldAltHeight) / mouse->dir.y * dir.y;
			}
			float3 wantedPos = cpos + mouse->dir * dif;
			float newHeight = ground->LineGroundCol(wantedPos, wantedPos + dir * 15000);
			if (newHeight < 0) {
				newHeight = height* (1.0f + move * mouseScale * 0.7f * (keys[SDLK_LSHIFT] ? 3:1));
			}
			if ((wantedPos.y + (dir.y * newHeight)) < 0) {
				newHeight = -wantedPos.y / dir.y;
			}
			if (newHeight < maxHeight) {
				height = newHeight;
				pos = wantedPos + dir * height;
			}
		// ZOOM OUT from mid screen
		} else {
			if (keys[SDLK_LALT]) { // instant-zoom: zoom out to the max
				if(height<maxHeight*0.5f && changeAltHeight){
					oldAltHeight=height;
					changeAltHeight=false;
				}
				height=maxHeight;
				pos.x=gs->mapx*4;
				pos.z=gs->mapy*4.8f; // somewhat longer toward bottom
			} else {
				height*=1+move * mouseScale*0.7f * (keys[SDLK_LSHIFT] ? 3:1);
			}
		}
		// instant-zoom: turn on the smooth transition and reset the camera tilt
		if (keys[SDLK_LALT]) {
			zscale = 0.5f;
			mouse->CameraTransition(1.0f);
		} else {
			changeAltHeight = true;
		}
	}
}

float3 COverheadController::GetPos()
{
	maxHeight=9.5f*max(gs->mapx,gs->mapy);		//map not created when constructor run

	if (pos.x < 0.01f) { pos.x = 0.01f; }
	if (pos.z < 0.01f) { pos.z = 0.01f; }
	if (pos.x > ((gs->mapx * SQUARE_SIZE) - 0.01f)) {
		pos.x = ((gs->mapx * SQUARE_SIZE) - 0.01f);
	}
	if (pos.z > ((gs->mapy * SQUARE_SIZE) - 0.01f)) {
		pos.z = ((gs->mapy * SQUARE_SIZE) - 0.01f);
	}
	if (height < 60.0f) {
		height = 60.0f;
	}
	if (height > maxHeight) {
		height = maxHeight;
	}

	pos.y = ground->GetHeight(pos.x,pos.z);
	dir = float3(0.0f, -1.0f, flipped ? zscale : -zscale).Normalize();

	float3 cpos = pos - dir * height;

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

void COverheadController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Overhead (TA) style camera");
}

void COverheadController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/* 0 */ (float)num);
	fv.push_back(/* 1 */ pos.x);
	fv.push_back(/* 2 */ pos.y);
	fv.push_back(/* 3 */ pos.z);
	fv.push_back(/* 4 */ dir.x);
	fv.push_back(/* 5 */ dir.y);
	fv.push_back(/* 6 */ dir.z);
	fv.push_back(/* 7 */ height);
	fv.push_back(/* 8 */ zscale);
	fv.push_back(/* 9 */ flipped ? +1.0f : -1.0f);
}

bool COverheadController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 10) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x   =  fv[1];
	pos.y   =  fv[2];
	pos.z   =  fv[3];
	dir.x   =  fv[4];
	dir.y   =  fv[5];
	dir.z   =  fv[6];
	height  =  fv[7];
	zscale  =  fv[8];
	flipped = (fv[9] > 0.0f);
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Total war Controller
//

CTWController::CTWController(int num)
: CCameraController(num),
  pos(2000,70,1800)
{
	scrollSpeed = configHandler.GetInt("TWScrollSpeed",10) * 0.1f;
	enabled = !!configHandler.GetInt("TWEnabled",1);
	fov = GetConfigFloat("TWFOV", "45.0");
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
	float pixelsize=tan(camera->fov/180/2*PI)*2/gu->viewSizeY*dist*2;
	move*=(1+keys[SDLK_LSHIFT]*3)*pixelsize;
	float3 flatForward=camera->forward;
	flatForward.y=0;
	flatForward.Normalize();

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

void CTWController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Total War style camera");
}

void CTWController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/* 0 */ (float)num);
	fv.push_back(/* 1 */ pos.x);
	fv.push_back(/* 2 */ pos.y);
	fv.push_back(/* 3 */ pos.z);
	fv.push_back(/* 4 */ camera->rot.x);
	fv.push_back(/* 5 */ camera->rot.y);
	fv.push_back(/* 6 */ camera->rot.z);
}

bool CTWController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 7) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	camera->rot.x = fv[4];
	camera->rot.y = fv[5];
	camera->rot.z = fv[6];
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Rotating Overhead Controller
//

CRotOverheadController::CRotOverheadController(int num)
: CCameraController(num),
  pos(2000,70,1800),
  oldHeight(500)
{
	mouseScale = GetConfigFloat("RotOverheadMouseScale", DEFAULT_MOUSE_SCALE);
	scrollSpeed = configHandler.GetInt("RotOverheadScrollSpeed",10)*0.1f;
	enabled=!!configHandler.GetInt("RotOverheadEnabled",1);
	fov = GetConfigFloat("RotOverheadFOV", "45.0");
}

void CRotOverheadController::KeyMove(float3 move)
{
	move*=sqrt(move.z)*400;

	float3 flatForward=camera->forward;
	if(camera->forward.y<-0.9f)
		flatForward+=camera->up;
	flatForward.y=0;
	flatForward.Normalize();

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
	dir.Normalize();
	return dir;
}

void CRotOverheadController::SetPos(float3 newPos)
{
	pos = newPos;
	pos.y = ground->GetHeight(pos.x, pos.z) + oldHeight;
}

float3 CRotOverheadController::SwitchFrom()
{
	return pos;
}

void CRotOverheadController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Rotatable overhead camera");
}

void CRotOverheadController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/*  0 */ (float)num);
	fv.push_back(/*  1 */ pos.x);
	fv.push_back(/*  2 */ pos.y);
	fv.push_back(/*  3 */ pos.z);
	fv.push_back(/*  4 */ dir.x);
	fv.push_back(/*  5 */ dir.y);
	fv.push_back(/*  6 */ dir.z);
	fv.push_back(/*  7 */ camera->rot.x);
	fv.push_back(/*  8 */ camera->rot.y);
	fv.push_back(/*  9 */ camera->rot.z);
	fv.push_back(/* 10 */ oldHeight);
}

bool CRotOverheadController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 11) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	dir.x = fv[4];
	dir.y = fv[5];
	dir.z = fv[6];
	camera->rot.x = fv[7];
	camera->rot.y = fv[8];
	camera->rot.z = fv[9];
	oldHeight = fv[10];
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Overview Controller
//

COverviewController::COverviewController(int num)
: CCameraController(num)
{
	enabled = false;
}

void COverviewController::KeyMove(float3 move)
{
}

void COverviewController::MouseMove(float3 move)
{
}

void COverviewController::ScreenEdgeMove(float3 move)
{
}

void COverviewController::MouseWheelMove(float move)
{
}

float3 COverviewController::GetPos()
{
	// map not created when constructor run
	pos.x = gs->mapx * 4.0f;
	pos.z = gs->mapy * 4.0f;
	const float aspect = (gu->viewSizeX / gu->viewSizeY);
	const float height = max(pos.x / aspect, pos.z);
	pos.y = ground->GetHeight(pos.x, pos.z) + (2.5f * height);
	return pos;
}

float3 COverviewController::GetDir()
{
	return float3(0.0f, -1.0f, -0.001f).Normalize();
}

void COverviewController::SetPos(float3 newPos)
{
}

float3 COverviewController::SwitchFrom()
{
	float3 dir=mouse->dir;
	float length=ground->LineGroundCol(pos,pos+dir*50000);
	float3 rpos=pos+dir*length;

	minimap->SetMinimized(minimizeMinimap);

	return rpos;
}

void COverviewController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Overview style camera");

	minimizeMinimap = minimap->GetMinimized();
	minimap->SetMinimized(true);
}

void COverviewController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/* 0 */ (float)num);
	fv.push_back(/* 1 */ pos.x);
	fv.push_back(/* 2 */ pos.y);
	fv.push_back(/* 3 */ pos.z);
}

bool COverviewController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 4) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Free Controller
//
//  TODO: - separate speed variable for tracking state
//        - smooth ransitions between tracking states and units
//        - improve controls
//        - rename it?  ;-)
//

CFreeController::CFreeController(int num)
: CCameraController(num),
  pos(0.0f, 100.0f, 0.0f),
  dir(0.0f, 0.0f, 0.0f),
  vel(0.0f, 0.0f, 0.0f),
  avel(0.0f, 0.0f, 0.0f),
  prevVel(0.0f, 0.0f, 0.0f),
  prevAvel(0.0f, 0.0f, 0.0f),
  tracking(false),
  trackPos(0.0f, 0.0f, 0.0f),
  trackRadius(0.0f),
  gndLock(false)
{
	enabled      = !!configHandler.GetInt("CamFreeEnabled",   0);
	invertAlt    = !!configHandler.GetInt("CamFreeInvertAlt", 0);
	goForward    = !!configHandler.GetInt("CamFreeGoForward", 0);
	fov          = GetConfigFloat("CamFreeFOV",           "45.0");
	scrollSpeed  = GetConfigFloat("CamFreeScrollSpeed",  "500.0");
	gravity      = GetConfigFloat("CamFreeGravity",     "-500.0");
	slide        = GetConfigFloat("CamFreeSlide",          "0.5");
	gndOffset    = GetConfigFloat("CamFreeGroundOffset",  "16.0");
	tiltSpeed    = GetConfigFloat("CamFreeTiltSpeed",    "150.0");
	tiltSpeed    = tiltSpeed * (PI / 180.0);
	autoTilt     = GetConfigFloat("CamFreeAutoTilt",     "150.0");
	autoTilt     = autoTilt * (PI / 180.0);
	velTime      = GetConfigFloat("CamFreeVelTime",        "1.5");
	velTime      = max(0.1f, velTime);
	avelTime     = GetConfigFloat("CamFreeAngVelTime",     "1.0");
	avelTime     = max(0.1f, avelTime);
}


void CFreeController::SetTrackingInfo(const float3& target, float radius)
{
	tracking = true;
	trackPos = target;
	trackRadius = radius;;

	// lock the view direction to the target
	const float3 diff = (trackPos - pos);
	const float rads = atan2(diff.x, diff.z);
	float radDiff = -fmod(camera->rot.y - rads, 2.0f * PI);

	if (radDiff < -PI) {
		radDiff += (2.0 * PI);
	} else if (radDiff > PI) {
		radDiff -= (2.0 * PI);
	}
	camera->rot.y = rads;

	const float len2D = diff.Length2D();
	if (fabs(len2D) <= 0.001f) {
		camera->rot.x = 0.0f;
	} else {
		camera->rot.x = atan2((trackPos.y - pos.y), len2D);
	}

	camera->UpdateForward();
}


void CFreeController::Update()
{
	if (!gu->active) {
		vel  = ZeroVector;
		avel = ZeroVector;
		prevVel  = vel;
		prevAvel = avel;
		return;
	}
	
	// safeties
	velTime  = max(0.1f,  velTime);
	avelTime = max(0.1f, avelTime);

	// save some old state
	const float ctrlVelY = vel.y;
	const float3 prevPos = pos;

	// setup the time fractions
	const float ft = gu->lastFrameTime;
	const float nt = (ft / velTime); // next time factor
	const float pt = (1.0f - nt);    // prev time factor
	const float ant = (ft / avelTime); // next time factor
	const float apt = (1.0f - ant);    // prev time factor

	// adjustment to match the ground slope
	float autoTiltVel = 0.0f;
	if (gndLock && (autoTilt > 0.0f)) {
		const float gndHeight = ground->GetHeight2(pos.x, pos.z);
		if (pos.y < (gndHeight + gndOffset + 1.0f)) {
			float3 hDir;
			hDir.y = 0.0f;
			hDir.x = (float)sin(camera->rot.y);
			hDir.z = (float)cos(camera->rot.y);
			const float3 gndNormal = ground->GetSmoothNormal(pos.x, pos.z);
			const float dot = gndNormal.dot(hDir);
			const float gndRotX = (float)acos(dot) - (PI * 0.5f);
			const float rotXdiff = (gndRotX - camera->rot.x);
			autoTiltVel = (autoTilt * rotXdiff);
		}
	}

	// convert control velocity into position velocity
	if (!tracking) {
		if (goForward) {
			const float3 tmpVel((camera->forward * vel.x) +
													(UpVector        * vel.y) +
													(camera->right   * vel.z));
			vel = tmpVel;
		}
		else {
			float3 forwardNoY(camera->forward.x, 0.0f, camera->forward.z);
			forwardNoY.Normalize();
			const float3 tmpVel((forwardNoY    * vel.x) +
													(UpVector      * vel.y) +
													(camera->right * vel.z));
			vel = tmpVel;
		}
	}

	// smooth the velocities
	vel  =  (vel * nt)  +  (prevVel * pt);
	avel = (avel * ant) + (prevAvel * apt);

	// no smoothing for gravity (still isn't right)
	if (gndLock) {
		const float dGrav = (gravity * ft);
		vel.y += dGrav;
		if (slide > 0.0f) {
			const float gndHeight = ground->GetHeight2(pos.x, pos.z);
			if (pos.y < (gndHeight + gndOffset + 1.0f)) {
				const float3 gndNormal = ground->GetSmoothNormal(pos.x, pos.z);
				const float dotVal = gndNormal.y;
				const float scale = (dotVal * slide * -dGrav);
				vel.x += (gndNormal.x * scale);
				vel.z += (gndNormal.z * scale);
			}
		}
	}

	// set the new position/rotation
	if (!tracking) {
		pos           += (vel         * ft);
		camera->rot   += (avel        * ft);
		camera->rot.x += (autoTiltVel * ft); // note that this is not smoothed
	}
	else {
		// speed along the tracking direction varies with distance
		const float3 diff = (pos - trackPos);
		if (goForward) {
			const float dist = max(0.1f, diff.Length());
			const float nomDist = 512.0f;
			float speedScale = (dist / nomDist);
			speedScale = max(0.25f, min(16.0f, speedScale));
			const float delta = -vel.x * (ft * speedScale);
			const float newDist = max(trackRadius, (dist + delta));
			const float scale = (newDist / dist);
			pos = trackPos + (diff * scale);
			pos.y += (vel.y * ft);
		}
		else {
			const float dist = max(0.1f, diff.Length2D());
			const float nomDist = 512.0f;
			float speedScale = (dist / nomDist);
			speedScale = max(0.25f, min(16.0f, speedScale));
			const float delta = -vel.x * (ft * speedScale);
			const float newDist = max(trackRadius, (dist + delta));
			const float scale = (newDist / dist);
			pos.x = trackPos.x + (scale * diff.x);
			pos.z = trackPos.z + (scale * diff.z);
			pos.y += (vel.y * ft);
		}

		// convert the angular velocity into its positional change
		const float3 diff2 = (pos - trackPos);
		const float deltaRad = (avel.y * ft);
		const float cos_val = cos(deltaRad);
		const float sin_val = sin(deltaRad);
		pos.x = trackPos.x + ((cos_val * diff2.x) + (sin_val * diff2.z));
		pos.z = trackPos.z + ((cos_val * diff2.z) - (sin_val * diff2.x));
	}

	// setup ground lock
	const float gndHeight = ground->GetHeight2(pos.x, pos.z);
	if (keys[SDLK_LSHIFT]) {
		if (ctrlVelY > 0.0f) {
			gndLock = false;
		} else if ((gndOffset > 0.0f) && (ctrlVelY < 0.0f) &&
		           (pos.y < (gndHeight + gndOffset))) {
			gndLock = true;
		}
	}

	// positional clamps
	if (gndOffset < 0.0f) {
		pos.y = (gndHeight - gndOffset);
		vel.y = 0.0f;
	}
	else if (gndLock && (gravity >= 0.0f)) {
		pos.y = (gndHeight + gndOffset);
		vel.y = 0.0f;
	}
	else if (gndOffset > 0.0f) {
		const float minHeight = (gndHeight + gndOffset);
		if (pos.y < minHeight) {
			pos.y = minHeight;
			if (gndLock) {
				vel.y = min(fabs(scrollSpeed), ((minHeight - prevPos.y) / ft));
			} else {
				vel.y = 0.0f;
			}
		}
	}

	// angular clamps
	const float xRotLimit = (PI * 0.4999f);
	if (camera->rot.x > xRotLimit) {
		camera->rot.x = xRotLimit;
		avel.x = 0.0f;
	}
	else if (camera->rot.x < -xRotLimit) {
		camera->rot.x = -xRotLimit;
		avel.x = 0.0f;
	}
	camera->rot.y = fmod(camera->rot.y, PI * 2.0f);

	// setup for the next loop
	prevVel  = vel;
	prevAvel = avel;
	vel  = ZeroVector;
	avel = ZeroVector;

	tracking = false;
}


void CFreeController::KeyMove(float3 move)
{
	const float qy = (move.y == 0.0f) ? 0.0f : (move.y > 0.0f ? 1.0f : -1.0f);
	const float qx = (move.x == 0.0f) ? 0.0f : (move.x > 0.0f ? 1.0f : -1.0f);

	const float speed  = keys[SDLK_LMETA] ? 4.0f * scrollSpeed : scrollSpeed;
	const float aspeed = keys[SDLK_LMETA] ? 2.0f * tiltSpeed   : tiltSpeed;

	if (keys[SDLK_LCTRL]) {
		avel.x += (aspeed * -qy); // tilt
	}
	else if (keys[SDLK_LSHIFT]) {
		vel.y += (speed * -qy); // up/down
	}
	else {
		vel.x += (speed * qy); // forwards/backwards
	}

	if (tracking) {
		avel.y += (aspeed * qx); // turntable rotation
	}
	else if (!keys[SDLK_LALT] == invertAlt) {
		vel.z += (speed * qx); // left/right
	}
	else {
		avel.y += (aspeed * -qx); // spin
	}

	return;
}


void CFreeController::MouseMove(float3 move)
{
	Uint8 prevAlt   = keys[SDLK_LALT];
	Uint8 prevCtrl  = keys[SDLK_LCTRL];
	Uint8 prevShift = keys[SDLK_LSHIFT];

	keys[SDLK_LCTRL] = !keys[SDLK_LCTRL]; // tilt
	keys[SDLK_LALT] = (invertAlt == !keys[SDLK_LALT]);
	KeyMove(move);

	keys[SDLK_LALT] = prevAlt;
	keys[SDLK_LCTRL] = prevCtrl;
	keys[SDLK_LSHIFT] = prevShift;
}


void CFreeController::ScreenEdgeMove(float3 move)
{
	Uint8 prevAlt   = keys[SDLK_LALT];
	Uint8 prevCtrl  = keys[SDLK_LCTRL];
	Uint8 prevShift = keys[SDLK_LSHIFT];

	keys[SDLK_LALT] = (invertAlt == !keys[SDLK_LALT]);
	KeyMove(move);

	keys[SDLK_LALT] = prevAlt;
	keys[SDLK_LCTRL] = prevCtrl;
	keys[SDLK_LSHIFT] = prevShift;
}


void CFreeController::MouseWheelMove(float move)
{
	Uint8 prevCtrl  = keys[SDLK_LCTRL];
	Uint8 prevShift = keys[SDLK_LSHIFT];
	keys[SDLK_LCTRL] = 0;
	keys[SDLK_LSHIFT] = 1;
	const float3 m(0.0f, move, 0.0f);
	KeyMove(m);
	keys[SDLK_LCTRL] = prevCtrl;
	keys[SDLK_LSHIFT] = prevShift;
}


void CFreeController::SetPos(float3 newPos)
{
	const float oldPosY = pos.y;
	pos = newPos;
	pos.y = oldPosY;
	if (gndOffset != 0.0f) {
		const float h = ground->GetHeight2(pos.x, pos.z);
		const float absH = h + fabs(gndOffset);
		if (pos.y < absH) {
			pos.y = absH;
		}
	}
	prevVel  = ZeroVector;
	prevAvel = ZeroVector;
}


float3 CFreeController::GetPos()
{
	return pos;
}


float3 CFreeController::GetDir()
{
	dir.x = (float)(sin(camera->rot.y) * cos(camera->rot.x));
	dir.z = (float)(cos(camera->rot.y) * cos(camera->rot.x));
	dir.y = (float)(sin(camera->rot.x));
	dir.Normalize();
	return dir;
}


float3 CFreeController::SwitchFrom()
{
	const float x = max(0.1f, min(float3::maxxpos - 0.1f, pos.x));
	const float z = max(0.1f, min(float3::maxzpos - 0.1f, pos.z));
	return float3(x, ground->GetHeight(x, z) + 5.0f, z);
}


void CFreeController::SwitchTo(bool showText)
{
	if (showText) {
		logOutput.Print("Switching to Free style camera");
	}
	prevVel  = ZeroVector;
	prevAvel = ZeroVector;
}


void CFreeController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/*  0 */ (float)num);
	fv.push_back(/*  1 */ pos.x);
	fv.push_back(/*  2 */ pos.y);
	fv.push_back(/*  3 */ pos.z);
	fv.push_back(/*  4 */ dir.x);
	fv.push_back(/*  5 */ dir.y);
	fv.push_back(/*  6 */ dir.z);
	fv.push_back(/*  7 */ camera->rot.x);
	fv.push_back(/*  8 */ camera->rot.y);
	fv.push_back(/*  9 */ camera->rot.z);

	fv.push_back(/* 10 */ fov);
	fv.push_back(/* 11 */ gndOffset);
	fv.push_back(/* 12 */ gravity);
	fv.push_back(/* 13 */ slide);
	fv.push_back(/* 14 */ scrollSpeed);
	fv.push_back(/* 15 */ tiltSpeed);
	fv.push_back(/* 16 */ velTime);
	fv.push_back(/* 17 */ avelTime);
	fv.push_back(/* 18 */ autoTilt);
	fv.push_back(/* 19 */ goForward ? 1.0f : -1.0f);
	fv.push_back(/* 20 */ invertAlt ? 1.0f : -1.0f);
	fv.push_back(/* 21 */ gndLock   ? 1.0f : -1.0f);
}


bool CFreeController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 22) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	dir.x = fv[4];
	dir.y = fv[5];
	dir.z = fv[6];
	camera->rot.x = fv[7];
	camera->rot.y = fv[8];
	camera->rot.z = fv[9];

	fov         =  fv[10];
	gndOffset   =  fv[11];
	gravity     =  fv[12];
	slide       =  fv[13];
	scrollSpeed =  fv[14];
	tiltSpeed   =  fv[15];
	velTime     =  fv[16];
	avelTime    =  fv[17];
	autoTilt    =  fv[18];
	goForward   = (fv[19] > 0.0f);
	invertAlt   = (fv[20] > 0.0f);
	gndLock     = (fv[21] > 0.0f);

	return true;
}


/******************************************************************************/
/******************************************************************************/
