/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include <SDL_keysym.h>
#include <boost/cstdint.hpp>

#include "OverheadController.h"

#include "ConfigHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/UI/MouseHandler.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "myMath.h"

extern boost::uint8_t *keys;

COverheadController::COverheadController()
	: flipped(false),
	zscale(0.5f),
	height(500),
	oldAltHeight(500),
	changeAltHeight(true),
	maxHeight(10000)
{
	middleClickScrollSpeed = configHandler->Get("MiddleClickScrollSpeed", 0.01f);
	scrollSpeed = configHandler->Get("OverheadScrollSpeed",10)*0.1f;
	tiltSpeed = configHandler->Get("OverheadTiltSpeed",1.0f);
	enabled = !!configHandler->Get("OverheadEnabled",1);
	fov = configHandler->Get("OverheadFOV", 45.0f);
}

void COverheadController::KeyMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= sqrt(move.z) * 200;
	float pixelsize = camera->GetTanHalfFov() * 2/globalRendering->viewSizeY * height * 2;
	pos.x += move.x * pixelsize * 2 * scrollSpeed;
	pos.z -= move.y * pixelsize * 2 * scrollSpeed;
}

void COverheadController::MouseMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= 100 * middleClickScrollSpeed;
	float pixelsize = camera->GetTanHalfFov() * 2/globalRendering->viewSizeY * height * 2;
	pos.x += move.x * pixelsize * (1+keys[SDLK_LSHIFT]*3) * scrollSpeed;
	pos.z += move.y * pixelsize * (1+keys[SDLK_LSHIFT]*3) * scrollSpeed;
}

void COverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

void COverheadController::MouseWheelMove(float move)
{
	// tilt the camera if LCTRL is pressed
	if (keys[SDLK_LCTRL]) {
		zscale *= (1.0f + (0.01f * move * tiltSpeed * (keys[SDLK_LSHIFT] ? 3.0f : 1.0f)));
		zscale = Clamp(zscale, 0.05f, 10.0f);
	} else { // holding down LALT uses 'instant-zoom' from here to the end of the function
		// ZOOM IN to mouse cursor instead of mid screen
		if (move < 0) {
			float3 cpos = pos - dir * height;
			float dif = -height * move * 0.007f * (keys[SDLK_LSHIFT] ? 3:1);
			if ((height - dif) < 60.0f) {
				dif = height - 60.0f;
			}
			if (keys[SDLK_LALT]) { // instant-zoom: zoom in to standard view
				dif = (height - oldAltHeight) / mouse->dir.y * dir.y;
			}
			float3 wantedPos = cpos + mouse->dir * dif;
			float newHeight = ground->LineGroundCol(wantedPos, wantedPos + dir * 15000);
			if (newHeight < 0) {
				newHeight = height* (1.0f + move * 0.007f * (keys[SDLK_LSHIFT] ? 3:1));
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
				if(height < maxHeight*0.5f && changeAltHeight){
					oldAltHeight = height;
					changeAltHeight = false;
				}
				height = maxHeight;
				pos.x  = gs->mapx * 4;
				pos.z  = gs->mapy * 4.8f; // somewhat longer toward bottom
			} else {
				height *= 1 + move * 0.007f * (keys[SDLK_LSHIFT] ? 3:1);
			}
		}
		// instant-zoom: turn on the smooth transition and reset the camera tilt
		if (keys[SDLK_LALT]) {
			zscale = 0.5f;
			camHandler->CameraTransition(1.0f);
		} else {
			changeAltHeight = true;
		}
	}
}

void COverheadController::Update()
{
}

float3 COverheadController::GetPos()
{
	maxHeight = 9.5f * std::max(gs->mapx, gs->mapy);	//map not created when constructor run

	pos.x = Clamp(pos.x, 0.01f, gs->mapx * SQUARE_SIZE - 0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy * SQUARE_SIZE - 0.01f);
	height = Clamp(height, 60.0f, maxHeight);

	pos.y = ground->GetHeight(pos.x,pos.z);
	dir = float3(0.0f, -1.0f, flipped ? zscale : -zscale).ANormalize();

	float3 cpos = pos - dir * height;
	return cpos;
}

float3 COverheadController::GetDir()
{
	return dir;
}

float3 COverheadController::SwitchFrom() const
{
	return pos;
}

void COverheadController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Overhead (TA) style camera");
}

void COverheadController::GetState(StateMap& sm) const
{
	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;

	sm["dx"] = dir.x;
	sm["dy"] = dir.y;
	sm["dz"] = dir.z;

	sm["height"]  = height;
	sm["zscale"]  = zscale;
	sm["flipped"] = flipped ? +1.0f : -1.0f;
}

bool COverheadController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);

	SetStateFloat(sm, "dx", dir.x);
	SetStateFloat(sm, "dy", dir.y);
	SetStateFloat(sm, "dz", dir.z);

	SetStateFloat(sm, "height", height);
	SetStateFloat(sm, "zscale", zscale);
	SetStateBool (sm, "flipped", flipped);

	return true;
}
