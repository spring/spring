/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/cstdint.hpp>
#include <SDL_keycode.h>

#include "SpringController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Input/KeyInput.h"


CONFIG(bool,  CamSpringEnabled).defaultValue(true).headlessValue(false);
CONFIG(int,   CamSpringScrollSpeed).defaultValue(10);
CONFIG(float, CamSpringFOV).defaultValue(45.0f);
CONFIG(bool,  CamSpringLockCardinalDirections).defaultValue(true).description("If when rotating cardinal directions should be `locked` for a short time.");
CONFIG(bool,  CamSpringZoomOutFromMousePos).defaultValue(false);
CONFIG(bool,  CamSpringEdgeRotate).defaultValue(false).description("Rotate camera when cursor touches screen borders.");


CSpringController::CSpringController()
: rot(2.677f, 0.f, 0.f)
, dist(float3(mapDims.mapx * 0.5f, 0.0f, mapDims.mapy * 0.55f).Length2D() * 1.5f * SQUARE_SIZE)
, maxDist(std::max(mapDims.mapx, mapDims.mapy) * SQUARE_SIZE * 1.333f)
, zoomBack(false)
, oldDist(0.f)
{
	enabled = configHandler->GetBool("CamSpringEnabled");
	Update();
}


void CSpringController::KeyMove(float3 move)
{
	if (KeyInput::GetKeyModState(KMOD_ALT)) {
		move *= math::sqrt(move.z);
		camera->SetRotY(MoveAzimuth(move.x));
		Update();
		return;
	}

	move *= math::sqrt(move.z) * 200.f;
	const float3 flatForward = (dir * XZVector).ANormalize();
	pos += (camera->GetRight() * move.x + flatForward * move.y) * pixelSize * 2.0f * scrollSpeed;
	Update();
}


void CSpringController::MouseMove(float3 move)
{
	if (KeyInput::GetKeyModState(KMOD_ALT)) {
		camera->SetRotY(MoveAzimuth(move.x * 0.01f));
		rot.x += move.y * move.z * 0.01f;
		rot.x  = Clamp(rot.x, PI * 0.51f, PI * 0.99f);
		camera->SetRotX(rot.x);
		Update();
		return;
	}

	move *= (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3);
	const float3 flatForward = (dir * XZVector).ANormalize();
	pos += (camera->GetRight() * move.x - flatForward * move.y) * pixelSize * 2.0f * scrollSpeed;
	Update();
}


void CSpringController::ScreenEdgeMove(float3 move)
{
	if (configHandler->GetBool("CamSpringEdgeRotate") && (mouse->lasty < globalRendering->viewSizeY / 3) && (mouse->lasty > globalRendering->viewSizeY / 10)) {
		// rotate camera when mouse touches top screen borders
		move *= (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3);
		camera->SetRotY(MoveAzimuth(move.x * 0.75f));
		move.x = 0.0f;
	}
	KeyMove(move);
}


void CSpringController::MouseWheelMove(float move)
{
	const float shiftSpeed = (KeyInput::GetKeyModState(KMOD_SHIFT) ? 3.0f : 1.0f);
	const float scaledMove = 1.0f + (move * shiftSpeed * 0.007f);

	// tilt the camera if CTRL is pressed
	if (KeyInput::GetKeyModState(KMOD_CTRL)) {
		rot.x -= (move * shiftSpeed * 0.001f);
		rot.x  = Clamp(rot.x, PI * 0.51f, PI * 0.99f);
		camera->SetRotX(rot.x);
	} else {
		const float curDist = dist;
		const float3 curCamPos = GetPos();

		camHandler->CameraTransition(0.15f);

		dist *= scaledMove;
		dist = std::min(dist, maxDist);

		if (move < 0.0f) {
			// ZOOM IN - to mouse cursor instead of mid screen

			if (KeyInput::GetKeyModState(KMOD_ALT) && zoomBack) {
				// instazoom in to standard view
				dist = oldDist;
				zoomBack = false;
				camHandler->CameraTransition(0.5f);
			} else {
				const float zoomInDist = CGround::LineGroundCol(curCamPos, curCamPos + mouse->dir * 150000.f, false);

				if (zoomInDist > 0.0f) {
					// current campos -> zoominpos groundpos -> wanted campos
					const float3 zoomInGroundPos = curCamPos + mouse->dir * zoomInDist;
					const float3 wantedCamPos = zoomInGroundPos + mouse->dir * zoomInDist * scaledMove;

					// campos -> groundpos
					const float newDist = CGround::LineGroundCol(wantedCamPos, wantedCamPos + dir * 150000.f, false);

					if (newDist > 0.0f) {
						dist = newDist;
						pos = wantedCamPos + dir * dist;
					}
				}
			}

			camera->SetPos(GetPos());
			camera->Update();
			camHandler->CameraTransition(0.25f);
		} else {
			// ZOOM OUT - from mid screen

			if (KeyInput::GetKeyModState(KMOD_ALT)) {
				// instazoom out to maximum height
				if (!zoomBack) {
					oldDist = curDist;
					zoomBack = true;
				}
				rot = float3(2.677f, rot.y, 0.f);
				pos.x = mapDims.mapx * SQUARE_SIZE * 0.5f;
				pos.z = mapDims.mapy * SQUARE_SIZE * 0.55f; // somewhat longer toward bottom
				dist = pos.Length2D() * 1.5f;
				camHandler->CameraTransition(1.0f);
			} else {
				zoomBack = false;

				if (configHandler->GetBool("CamSpringZoomOutFromMousePos")) {
					const float zoomInDist = CGround::LineGroundCol(curCamPos, curCamPos + mouse->dir * 150000.f, false);

					if (zoomInDist > 0.0f) {
						// zoominpos -> campos
						const float3 zoomInGroundPos = curCamPos + mouse->dir * zoomInDist;
						const float3 wantedCamPos = zoomInGroundPos - mouse->dir * zoomInDist * scaledMove;

						// campos -> groundpos
						const float newDist = CGround::LineGroundCol(wantedCamPos, wantedCamPos + dir * 150000.f, false);

						if (newDist > 0.0f) {
							dist = newDist;
							pos = wantedCamPos + dir * dist;
						}
					}

					camera->SetPos(GetPos());
					camera->Update();
					camHandler->CameraTransition(0.25f);
				}
			}
		}
	}

	Update();
}


void CSpringController::Update()
{
	pos.ClampInMap();

	rot.x = Clamp(rot.x, PI * 0.51f, PI * 0.99f);
	dist = Clamp(dist, 20.f, maxDist);
	dir = camera->GetDir();

	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * dist * 2.0f;

	scrollSpeed = configHandler->GetInt("CamSpringScrollSpeed") * 0.1f;
	fov = configHandler->GetFloat("CamSpringFOV");
}


static float GetRotationWithCardinalLock(float rot)
{
	static float cardinalDirLockWidth = 0.2f;

	rot /= fastmath::HALFPI;
	const float rotMoved = std::abs(rot) - cardinalDirLockWidth * 0.5f;

	const float numerator = std::trunc(rotMoved);

	const float fract = rotMoved - numerator;
	const float b = 1.f / (1.f - cardinalDirLockWidth);
	const float c = 1.f - b;
	const float fx = (fract > cardinalDirLockWidth) ? fract * b + c : 0.f;

	return std::copysign(numerator + fx, rot) * fastmath::HALFPI;
}


float CSpringController::MoveAzimuth(float move)
{
	const float minRot = std::floor(rot.y / fastmath::HALFPI) * fastmath::HALFPI;
	const float maxRot = std::ceil(rot.y / fastmath::HALFPI) * fastmath::HALFPI;
	rot.y -= move;
	if (configHandler->GetBool("CamSpringLockCardinalDirections")) return GetRotationWithCardinalLock(rot.y);
	if (KeyInput::GetKeyModState(KMOD_CTRL)) rot.y = Clamp(rot.y, minRot + 0.02f, maxRot - 0.02f);
	return rot.y;
}


float CSpringController::GetAzimuth() const
{
	if (configHandler->GetBool("CamSpringLockCardinalDirections"))
		return GetRotationWithCardinalLock(rot.y);
	return rot.y;
}


float3 CSpringController::GetRot() const
{
	float3 r;
	r.x = rot.x;
	r.y = GetAzimuth();
	return r;
}


float3 CSpringController::GetPos() const
{
	float3 cpos = pos - dir * dist;
	cpos.y = std::max(cpos.y, CGround::GetHeightAboveWater(cpos.x, cpos.z, false) + 5);
	return cpos;
}


void CSpringController::SetPos(const float3& newPos)
{
	pos = newPos;
	Update();
}


float3 CSpringController::SwitchFrom() const
{
	return pos;
}


void CSpringController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText) {
		LOG("Switching to Spring style camera");
	}
	if (oldCam != CCameraHandler::CAMERA_MODE_OVERVIEW) {
		rot.x = camera->GetRot().x;
		rot.y = camera->GetRot().y;
	}
}


void CSpringController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);
	sm["dist"] = dist;
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}


bool CSpringController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);
	SetStateFloat(sm, "dist", dist);
	SetStateFloat(sm, "rx",   rot.x);
	SetStateFloat(sm, "ry",   rot.y);
	SetStateFloat(sm, "rz",   rot.z);
	return true;
}
