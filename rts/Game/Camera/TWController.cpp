/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>
#include <SDL_keysym.h>

#include "StdAfx.h"
#include "mmgr.h"

#include "TWController.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"

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

	move *= sqrt(move.z) * 200;
	pos  += (camera->right * move.x + flatForward * move.y) * scrollSpeed;
}


void CTWController::MouseMove(float3 move)
{
	move *= (1 + keyInput->GetKeyState(SDLK_LSHIFT) * 3) * pixelSize;

	float3 flatForward = camera->forward;
	flatForward.y = 0;
	flatForward.ANormalize();

	pos += (camera->right * move.x - flatForward * move.y) * scrollSpeed;
}


void CTWController::ScreenEdgeMove(float3 move)
{
	if(mouse->lasty<globalRendering->viewSizeY/3){
		camera->rot.y-=move.x*globalRendering->lastFrameTime*0.5f*200;
		move.x=0;
	}
	KeyMove(move);
}


void CTWController::MouseWheelMove(float move)
{
	camera->rot.x-=move*0.001f;
}

void CTWController::Update()
{
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * (-camera->rot.x * 1500) * 2.0f;
}

float3 CTWController::GetPos()
{
	pos.x = Clamp(pos.x, 0.01f, gs->mapx*SQUARE_SIZE-0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy*SQUARE_SIZE-0.01f);
	pos.y = ground->GetHeightAboveWater(pos.x,pos.z);

	camera->rot.x = Clamp(camera->rot.x, -PI*0.4f, -0.1f);

	float3 dir;
	dir.x=(float)(sin(camera->rot.y)*cos(camera->rot.x));
	dir.y=(float)(sin(camera->rot.x));
	dir.z=(float)(cos(camera->rot.y)*cos(camera->rot.x));
	dir.ANormalize();

	float dist = -camera->rot.x * 1500;

	float3 cpos = pos - dir * dist;
	if(cpos.y < ground->GetHeightAboveWater(cpos.x,cpos.z) + 5)
		cpos.y = ground->GetHeightAboveWater(cpos.x,cpos.z) + 5;

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
