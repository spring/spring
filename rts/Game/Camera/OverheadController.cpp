/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>
#include <boost/cstdint.hpp>

#include "OverheadController.h"

#include "System/Config/ConfigHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"

CONFIG(float, MiddleClickScrollSpeed).defaultValue(0.01f);
CONFIG(int, OverheadScrollSpeed).defaultValue(10);
CONFIG(float, OverheadTiltSpeed).defaultValue(1.0f);
CONFIG(bool, OverheadEnabled).defaultValue(true);
CONFIG(float, OverheadFOV).defaultValue(45.0f);

COverheadController::COverheadController()
	: flipped(false)
	, zscale(0.5f)
	, height(1500)
	, oldAltHeight(1500)
	, changeAltHeight(true)
	, maxHeight(10000)
{
	middleClickScrollSpeed = configHandler->GetFloat("MiddleClickScrollSpeed");
	scrollSpeed = configHandler->GetInt("OverheadScrollSpeed") * 0.1f;
	tiltSpeed = configHandler->GetFloat("OverheadTiltSpeed");
	enabled = configHandler->GetBool("OverheadEnabled");
	fov = configHandler->GetFloat("OverheadFOV");

	if (globalRendering) {
		// make whole map visible
		const float h = std::max(pos.x / globalRendering->aspectRatio, pos.z);
		height = CGround::GetHeightAboveWater(pos.x, pos.z, false) + (2.5f * h);
	}

	maxHeight = 9.5f * std::max(gs->mapx, gs->mapy);
	UpdateVectors();
}

void COverheadController::KeyMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= math::sqrt(move.z) * 200;

	pos.x += move.x * pixelSize * 2.0f * scrollSpeed;
	pos.z -= move.y * pixelSize * 2.0f * scrollSpeed;
	UpdateVectors();
}

void COverheadController::MouseMove(float3 move)
{
	if (flipped) {
		move.x = -move.x;
		move.y = -move.y;
	}
	move *= 100 * middleClickScrollSpeed;

	pos.x += move.x * pixelSize * (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3) * scrollSpeed;
	pos.z += move.y * pixelSize * (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3) * scrollSpeed;
	UpdateVectors();
}

void COverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}

// FIXME: 100% identical to SmoothController::MouseWheelMove
void COverheadController::MouseWheelMove(float move)
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
			float newHeight = CGround::LineGroundCol(wantedPos, wantedPos + dir * 15000, false);

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

void COverheadController::UpdateVectors()
{
	pos.x = Clamp(pos.x, 0.01f, gs->mapx * SQUARE_SIZE - 0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy * SQUARE_SIZE - 0.01f);
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	height = Clamp(height, 60.0f, maxHeight);
	dir = float3(0.0f, -1.0f, flipped ? zscale : -zscale).ANormalize();
}

void COverheadController::Update()
{
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * height * 2.0f;
}

float3 COverheadController::GetPos() const
{
	float3 cpos = pos - dir * height;
	return cpos;
}

void COverheadController::SetPos(const float3& newPos)
{
	pos = newPos;
	UpdateVectors();
}

float3 COverheadController::SwitchFrom() const
{
	return pos;
}

void COverheadController::SwitchTo(bool showText)
{
	if (showText) {
		LOG("Switching to Overhead (TA) style camera");
	}
}

void COverheadController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);

	sm["height"]  = height;
	sm["zscale"]  = zscale;
	sm["flipped"] = flipped ? +1.0f : -1.0f;
}

bool COverheadController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);

	SetStateFloat(sm, "height", height);
	SetStateFloat(sm, "zscale", zscale);
	SetStateBool (sm, "flipped", flipped);
	UpdateVectors();

	return true;
}
