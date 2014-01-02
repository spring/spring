/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>
#include <boost/cstdint.hpp>

#include "SmoothController.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"
#include "System/Misc/SpringTime.h"


CONFIG(int, SmoothScrollSpeed).defaultValue(10);
CONFIG(float, SmoothTiltSpeed).defaultValue(1.0f);
CONFIG(bool, SmoothEnabled).defaultValue(true);
CONFIG(float, SmoothFOV).defaultValue(45.0f);

const float SmoothController::maxSpeedFactor = 300;

SmoothController::SmoothController()
	: flipped(false)
	, zscale(0.5f)
	, height(1500)
	, oldAltHeight(1500)
	, changeAltHeight(true)
	, maxHeight(10000)
	, speedFactor(1.0f)
{
	middleClickScrollSpeed = configHandler->GetFloat("MiddleClickScrollSpeed");
	scrollSpeed = configHandler->GetInt("SmoothScrollSpeed") * 0.1f;
	tiltSpeed = configHandler->GetFloat("SmoothTiltSpeed");
	enabled = configHandler->GetBool("SmoothEnabled");
	fov = configHandler->GetFloat("SmoothFOV");
	lastSource = Noone;

	if (ground && globalRendering) {
		// make whole map visible
		const float h = std::max(pos.x / globalRendering->aspectRatio, pos.z);
		height = ground->GetHeightAboveWater(pos.x, pos.z, false) + (2.5f * h);
	}

	maxHeight = 9.5f * std::max(gs->mapx, gs->mapy);
	UpdateVectors();
}

void SmoothController::KeyMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}

	move *= math::sqrt(move.z) * 200.0f;

	const float3 thisMove(move.x * pixelSize * 2.0f * scrollSpeed, 0.0f, -move.y * pixelSize * 2.0f * scrollSpeed);

	static spring_time lastScreenMove = spring_gettime();
	const spring_time timeDiff = spring_gettime() - lastScreenMove;
	lastScreenMove = spring_gettime();

	if (thisMove.x != 0 || thisMove.z != 0) {
		// user want to move with keys
		lastSource = Key;
		Move(thisMove, timeDiff.toMilliSecsf());
	} else if (lastSource == Key) {
		// last move order was given by keys, so call Move() to break
		Move(thisMove, timeDiff.toMilliSecsf());
	}
}


void SmoothController::ScreenEdgeMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= math::sqrt(move.z) * 200.0f;

	const float3 thisMove(move.x * pixelSize * 2.0f * scrollSpeed, 0.0f, -move.y * pixelSize * 2.0f * scrollSpeed);

	static spring_time lastScreenMove = spring_gettime();
	const spring_time timeDiff = spring_gettime() - lastScreenMove;
	lastScreenMove = spring_gettime();

	if (thisMove.x != 0 || thisMove.z != 0) {
		// user want to move with mouse on screen edge
		lastSource = ScreenEdge;
		Move(thisMove, timeDiff.toMilliSecsf());
	} else if (lastSource == ScreenEdge) {
		// last move order was given by screen edge, so call Move() to break
		Move(thisMove, timeDiff.toMilliSecsf());
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
		move.x * pixelSize * (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3) * scrollSpeed,
		0.0f,
		move.y * pixelSize * (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3) * scrollSpeed
	);

	pos += (thisMove + lastMove) / 2.0f;
	lastMove = (thisMove + lastMove) / 2.0f;
	UpdateVectors();
}


void SmoothController::MouseWheelMove(float move)
{
	if (move == 0.0f)
		return;

	camHandler->CameraTransition(0.05f);

	const float shiftSpeed = (KeyInput::GetKeyModState(KMOD_SHIFT) ? 3.0f : 1.0f);
	const float altZoomDist = height * move * 0.007f * shiftSpeed;

	// tilt the camera if LCTRL is pressed
	//
	// otherwise holding down LALT uses 'instant-zoom'
	// from here to the end of the function (smoothed)
	if (KeyInput::GetKeyModState(KMOD_CTRL)) {
		zscale *= (1.0f + (0.01f * move * tiltSpeed * shiftSpeed));
		zscale = Clamp(zscale, 0.05f, 10.0f);
	} else {
		if (move < 0.0f) {
			// ZOOM IN to mouse cursor instead of mid screen
			float3 cpos = pos - dir * height;
			float dif = -altZoomDist;

			if ((height - dif) < 60.0f) {
				dif = height - 60.0f;
			}
			if (KeyInput::GetKeyModState(KMOD_ALT)) {
				// instazoom in to standard view
				dif = (height - oldAltHeight) / mouse->dir.y * dir.y;
			}

			float3 wantedPos = cpos + mouse->dir * dif;
			float newHeight = ground->LineGroundCol(wantedPos, wantedPos + dir * 15000, false);

			if (newHeight < 0.0f) {
				newHeight = height * (1.0f + move * 0.007f * shiftSpeed);
			}
			if ((wantedPos.y + (dir.y * newHeight)) < 0.0f) {
				newHeight = -wantedPos.y / dir.y;
			}
			if (newHeight < maxHeight) {
				height = newHeight;
				pos = wantedPos + dir * height;
			}
		} else {
			// ZOOM OUT from mid screen
			if (KeyInput::GetKeyModState(KMOD_ALT)) {
				// instazoom out to maximum height
				if (height < maxHeight*0.5f && changeAltHeight) {
					oldAltHeight = height;
					changeAltHeight = false;
				}
				height = maxHeight;
				pos.x  = gs->mapx * 4;
				pos.z  = gs->mapy * 4.8f; // somewhat longer toward bottom
			} else {
				height *= (1.0f + (altZoomDist / height));
			}
		}

		// instant-zoom: turn on the smooth transition and reset the camera tilt
		if (KeyInput::GetKeyModState(KMOD_ALT)) {
			zscale = 0.5f;
			camHandler->CameraTransition(1.0f);
		} else {
			changeAltHeight = true;
		}
	}

	UpdateVectors();
}

void SmoothController::UpdateVectors()
{
	pos.x = Clamp(pos.x, 0.01f, gs->mapx * SQUARE_SIZE - 0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy * SQUARE_SIZE - 0.01f);
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z, false);
	height = Clamp(height, 60.0f, maxHeight);
	dir = float3(0.0f, -1.0f, flipped ? zscale : -zscale).ANormalize();
}

void SmoothController::Update()
{
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * height * 2.0f;
}

float3 SmoothController::GetPos() const
{
	float3 cpos = pos - dir * height;
	cpos.y = std::max(cpos.y, ground->GetHeightAboveWater(cpos.x, cpos.z, false) + 5.0f);
	return cpos;
}

void SmoothController::SetPos(const float3& newPos)
{
	pos = newPos;
	UpdateVectors();
}

float3 SmoothController::SwitchFrom() const
{
	return pos;
}


void SmoothController::SwitchTo(bool showText)
{
	if (showText) {
		LOG("Switching to Smooth style camera");
	}
}


void SmoothController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);

	sm["height"]  = height;
	sm["zscale"]  = zscale;
	sm["flipped"] = flipped ? +1.0f : -1.0f;
}

bool SmoothController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);

	SetStateFloat(sm, "height",  height);
	SetStateFloat(sm, "zscale",  zscale);
	SetStateBool (sm, "flipped", flipped);

	return true;
}


void SmoothController::Move(const float3& move, const float timeDiff)
{
	if ((move.x != 0 || move.z != 0) && speedFactor < maxSpeedFactor)
	{
		// accelerate
		speedFactor += timeDiff;
		if (speedFactor > maxSpeedFactor)
			speedFactor = maxSpeedFactor; // don't know why, but std::min produces undefined references here
		lastMove = move;
		pos += move * speedFactor / maxSpeedFactor;
	}
	else if ((move.x == 0 || move.z == 0) && speedFactor > 1)
	{
		// break
		if (timeDiff > speedFactor)
			speedFactor = 0.0f;
		else
			speedFactor -= timeDiff;
		pos += lastMove * speedFactor / maxSpeedFactor;
	}
	else
	{
		// move or hold position (move = {0,0,0})
		pos += move;
		lastMove = move;
	}
	UpdateVectors();
}
