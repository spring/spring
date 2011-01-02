/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <boost/cstdint.hpp>

#include "SmoothController.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"

SmoothController::SmoothController()
	: flipped(false)
	, zscale(0.5f)
	, height(500)
	, oldAltHeight(500)
	, changeAltHeight(true)
	, maxHeight(10000)
	, speedFactor(1)
{
	middleClickScrollSpeed = configHandler->Get("MiddleClickScrollSpeed", 0.01f);
	scrollSpeed = configHandler->Get("SmoothScrollSpeed",10)*0.1f;
	tiltSpeed = configHandler->Get("SmoothTiltSpeed",1.0f);
	enabled = !!configHandler->Get("SmoothEnabled",1);
	fov = configHandler->Get("SmoothFOV", 45.0f);
	lastSource = Noone;
}

void SmoothController::KeyMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}

	move *= sqrt(move.z) * 200.0f;

	const float3 thisMove(move.x * pixelSize * 2.0f * scrollSpeed, 0.0f, -move.y * pixelSize * 2.0f * scrollSpeed);

	static unsigned lastKeyMove = SDL_GetTicks();
	const unsigned timeDiff = SDL_GetTicks() - lastKeyMove;

	lastKeyMove = SDL_GetTicks();

	if (thisMove.x != 0 || thisMove.z != 0) {
		// user want to move with keys
		lastSource = Key;
		Move(thisMove, timeDiff);
	} else if (lastSource == Key) {
		// last move order was given by keys, so call Move() to break
		Move(thisMove, timeDiff);
	}
}


void SmoothController::MouseMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= 100 * middleClickScrollSpeed;

	// do little smoothing here (and because its little it won't hurt if it depends on framerate)
	static float3 lastMove = ZeroVector;
	const float3 thisMove(
		move.x * pixelSize * (1 + keyInput->GetKeyState(SDLK_LSHIFT) * 3) * scrollSpeed,
		0.0f,
		move.y * pixelSize * (1 + keyInput->GetKeyState(SDLK_LSHIFT) * 3) * scrollSpeed
	);

	pos += (thisMove + lastMove) / 2.0f;
	lastMove = (thisMove + lastMove) / 2.0f;
}


void SmoothController::ScreenEdgeMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= sqrt(move.z) * 200.0f;

	const float3 thisMove(move.x * pixelSize * 2.0f * scrollSpeed, 0.0f, -move.y * pixelSize * 2.0f * scrollSpeed);

	static unsigned lastScreenMove = SDL_GetTicks();
	const unsigned timeDiff = SDL_GetTicks() - lastScreenMove;

	lastScreenMove = SDL_GetTicks();

	if (thisMove.x != 0 || thisMove.z != 0) {
		// user want to move with mouse on screen edge
		lastSource = ScreenEdge;
		Move(thisMove, timeDiff);
	} else if (lastSource == ScreenEdge) {
		// last move order was given by screen edge, so call Move() to break
		Move(thisMove, timeDiff);
	}
}


void SmoothController::MouseWheelMove(float move)
{
	// tilt the camera if LCTRL is pressed
	if (keyInput->IsKeyPressed(SDLK_LCTRL)) {
		zscale *= (1.0f + (0.01f * move * tiltSpeed * (keyInput->IsKeyPressed(SDLK_LSHIFT) ? 3.0f : 1.0f)));
		zscale = Clamp(zscale, 0.05f, 10.0f);
	} else { // holding down LALT uses 'instant-zoom' from here to the end of the function
		// ZOOM IN to mouse cursor instead of mid screen
		if (move < 0) {
			float3 cpos = pos - dir * height;
			float dif = -height * move * 0.007f * (keyInput->IsKeyPressed(SDLK_LSHIFT) ? 3:1);
			if ((height - dif) < 60.0f) {
				dif = height - 60.0f;
			}
			if (keyInput->IsKeyPressed(SDLK_LALT)) { // instant-zoom: zoom in to standard view
				dif = (height - oldAltHeight) / mouse->dir.y * dir.y;
			}
			float3 wantedPos = cpos + mouse->dir * dif;
			float newHeight = ground->LineGroundCol(wantedPos, wantedPos + dir * 15000);
			if (newHeight < 0) {
				newHeight = height * (1.0f + move * 0.007f * (keyInput->IsKeyPressed(SDLK_LSHIFT) ? 3:1));
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
			if (keyInput->IsKeyPressed(SDLK_LALT)) { // instant-zoom: zoom out to the max
				if(height < maxHeight*0.5f && changeAltHeight){
					oldAltHeight = height;
					changeAltHeight = false;
				}
				height = maxHeight;
				pos.x  = gs->mapx * 4;
				pos.z  = gs->mapy * 4.8f; // somewhat longer toward bottom
			} else {
				height *= 1 + move * 0.007f * (keyInput->IsKeyPressed(SDLK_LSHIFT) ? 3:1);
			}
		}
		// instant-zoom: turn on the smooth transition and reset the camera tilt
		if (keyInput->IsKeyPressed(SDLK_LALT)) {
			zscale = 0.5f;
			camHandler->CameraTransition(1.0f);
		} else {
			changeAltHeight = true;
		}
	}
}

void SmoothController::Update()
{
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * height * 2.0f;
}

float3 SmoothController::GetPos()
{
	maxHeight = 9.5f * std::max(gs->mapx, gs->mapy);	//map not created when constructor run

	pos.x = Clamp(pos.x, 0.01f, gs->mapx * SQUARE_SIZE - 0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy * SQUARE_SIZE - 0.01f);
	height = Clamp(height, 60.0f, maxHeight);

	pos.y = ground->GetHeightAboveWater(pos.x,pos.z);
	dir = float3(0.0f, -1.0f, flipped ? zscale : -zscale).ANormalize();

	float3 cpos = pos - dir * height;
	cpos.y = std::max(cpos.y, ground->GetHeightAboveWater(cpos.x,cpos.z)+5.0f);
	return cpos;
}


float3 SmoothController::GetDir()
{
	return dir;
}


float3 SmoothController::SwitchFrom() const
{
	return pos;
}


void SmoothController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Smooth style camera");
}


void SmoothController::GetState(StateMap& sm) const
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

bool SmoothController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);

	SetStateFloat(sm, "dx", dir.x);
	SetStateFloat(sm, "dy", dir.y);
	SetStateFloat(sm, "dz", dir.z);

	SetStateFloat(sm, "height",  height);
	SetStateFloat(sm, "zscale",  zscale);
	SetStateBool (sm, "flipped", flipped);

	return true;
}


void SmoothController::Move(const float3& move, const unsigned timeDiff)
{
	if ((move.x != 0 || move.z != 0) && speedFactor < maxSpeedFactor)
	{
		// accelerate
		speedFactor += timeDiff;
		if (speedFactor > maxSpeedFactor)
			speedFactor = maxSpeedFactor; // don't know why, but std::min produces undefined references here
		lastMove = move;
		pos += move*static_cast<float>(speedFactor)/static_cast<float>(maxSpeedFactor);
	}
	else if ((move.x == 0 || move.z == 0) && speedFactor > 1)
	{
		// break
		if (timeDiff > speedFactor)
			speedFactor = 0;
		else
			speedFactor -= timeDiff;
		pos += lastMove*static_cast<float>(speedFactor)/static_cast<float>(maxSpeedFactor);
	}
	else
	{
		// move or hold position (move = {0,0,0})
		pos += move;
		lastMove = move;
	}
}
