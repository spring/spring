/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
#include "System/SpringMath.h"
#include "System/Input/KeyInput.h"


CONFIG(bool,  CamSpringEnabled).defaultValue(true).headlessValue(false);
CONFIG(int,   CamSpringScrollSpeed).defaultValue(10);
CONFIG(float, CamSpringFOV).defaultValue(45.0f);
CONFIG(bool,  CamSpringLockCardinalDirections).defaultValue(true).description("Whether cardinal directions should be `locked` for a short time when rotating.");
CONFIG(bool,  CamSpringZoomInToMousePos).defaultValue(true);
CONFIG(bool,  CamSpringZoomOutFromMousePos).defaultValue(false);
CONFIG(bool,  CamSpringEdgeRotate).defaultValue(false).description("Rotate camera when cursor touches screen borders.");


CSpringController::CSpringController()
	: rot(2.677f, 0.0f, 0.0f)
	, curDist(float3(mapDims.mapx * 0.5f, 0.0f, mapDims.mapy * 0.55f).Length2D() * 1.5f * SQUARE_SIZE)
	, maxDist(std::max(mapDims.mapx, mapDims.mapy) * SQUARE_SIZE * 1.333f)
	, oldDist(0.0f)
	, zoomBack(false)
	, cursorZoomIn(configHandler->GetBool("CamSpringZoomInToMousePos"))
	, cursorZoomOut(configHandler->GetBool("CamSpringZoomOutFromMousePos"))
{
	enabled = configHandler->GetBool("CamSpringEnabled");
}


void CSpringController::KeyMove(float3 move)
{
	move *= math::sqrt(move.z);

	if (KeyInput::GetKeyModState(KMOD_ALT)) {
		rot.x = Clamp(rot.x + move.y, math::PI * 0.51f, math::PI * 0.99f);
		MoveAzimuth(move.x);
		Update();
		return;
	}

	move *= 200.0f;
	const float3 flatForward = (dir * XZVector).ANormalize();
	pos += (camera->GetRight() * move.x + flatForward * move.y) * pixelSize * 2.0f * scrollSpeed;
	Update();
}


void CSpringController::MouseMove(float3 move)
{
	move *= 0.005f;
	move *= (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3);
	move.y = -move.y;
	move.z = 1.0f;

	KeyMove(move);
}


void CSpringController::ScreenEdgeMove(float3 move)
{
	const bool doRotate = configHandler->GetBool("CamSpringEdgeRotate");
	const bool belowMax = (mouse->lasty < globalRendering->viewSizeY /  3);
	const bool aboveMin = (mouse->lasty > globalRendering->viewSizeY / 10);

	if (doRotate && aboveMin && belowMax) {
		// rotate camera when mouse touches top screen borders
		move *= (1 + KeyInput::GetKeyModState(KMOD_SHIFT) * 3);
		MoveAzimuth(move.x * 0.75f);
		move.x = 0.0f;
	}

	KeyMove(move);
}


void CSpringController::MouseWheelMove(float move)
{
	const float shiftSpeed = (KeyInput::GetKeyModState(KMOD_SHIFT) ? 2.0f : 1.0f);
	const float scaledMove = 1.0f + (move * shiftSpeed * 0.007f);
	const float curDistPre = curDist;

	// tilt the camera if CTRL is pressed, otherwise zoom
	// no tweening during tilt, position is not fixed but
	// moves along an arc segment
	if (KeyInput::GetKeyModState(KMOD_CTRL)) {
		rot.x -= (move * shiftSpeed * 0.005f);
	} else {
		// depends on curDist
		const float3 curCamPos = GetPos();

		curDist *= scaledMove;
		curDist = std::min(curDist, maxDist);

		float zoomTransTime = 0.25f;

		if (move < 0.0f) {
			// ZOOM IN - to mouse cursor or along our own forward dir
			zoomTransTime = ZoomIn(curCamPos, float2(curDistPre, scaledMove));
		} else {
			// ZOOM OUT - from mid screen
			zoomTransTime = ZoomOut(curCamPos, float2(curDistPre, scaledMove));
		}

		camHandler->CameraTransition(zoomTransTime);
	}

	Update();
}


float CSpringController::ZoomIn(const float3& curCamPos, const float2& zoomParams)
{
	if (KeyInput::GetKeyModState(KMOD_ALT) && zoomBack) {
		// instazoom in to standard view
		curDist = oldDist;
		zoomBack = false;

		return 0.5f;
	}

	if (!cursorZoomIn)
		return 0.25f;

	float curGroundDist = CGround::LineGroundCol(curCamPos, curCamPos + mouse->dir * 150000.0f, false);

	if (curGroundDist <= 0.0f)
		curGroundDist = CGround::LinePlaneCol(curCamPos, mouse->dir, 150000.0f, readMap->GetCurrAvgHeight());
	if (curGroundDist <= 0.0f)
		return 0.25f;

	// zoom in to cursor, then back out (along same dir) based on scaledMove
	// to find where we want to place camera, but make sure the wanted point
	// is always in front of curCamPos
	const float3 cursorVec = mouse->dir * curGroundDist;
	const float3 wantedPos = curCamPos + cursorVec * (1.0f - zoomParams.y);

	// figure out how far we will end up from the ground at new wanted point
	float newGroundDist = CGround::LineGroundCol(wantedPos, wantedPos + dir * 150000.0f, false);

	if (newGroundDist <= 0.0f)
		newGroundDist = CGround::LinePlaneCol(wantedPos, dir, 150000.0f, readMap->GetCurrAvgHeight());

	pos = wantedPos + dir * (curDist = newGroundDist);

	return 0.25f;
}

float CSpringController::ZoomOut(const float3& curCamPos, const float2& zoomParams)
{
	if (KeyInput::GetKeyModState(KMOD_ALT)) {
		// instazoom out to maximum height
		if (!zoomBack) {
			oldDist = zoomParams.x;
			zoomBack = true;
		}

		rot = float3(2.677f, rot.y, 0.0f);
		pos.x = mapDims.mapx * SQUARE_SIZE * 0.5f;
		pos.z = mapDims.mapy * SQUARE_SIZE * 0.55f; // somewhat longer toward bottom
		curDist = pos.Length2D() * 1.5f;
		return 1.0f;
	}

	zoomBack = false;

	if (!cursorZoomOut)
		return 0.25f;

	const float zoomInDist = CGround::LineGroundCol(curCamPos, curCamPos + mouse->dir * 150000.0f, false);

	if (zoomInDist <= 0.0f)
		return 0.25f;

	// same logic as ZoomIn, but in opposite direction
	const float3 zoomedCamPos =    curCamPos + mouse->dir * zoomInDist;
	const float3 wantedCamPos = zoomedCamPos - mouse->dir * zoomInDist * zoomParams.y;

	const float newDist = CGround::LineGroundCol(wantedCamPos, wantedCamPos + dir * 150000.0f, false);

	if (newDist > 0.0f)
		pos = wantedCamPos + dir * (curDist = newDist);

	return 0.25f;
}



void CSpringController::Update()
{
	pos.ClampInMap();

	pos.y = CGround::GetHeightReal(pos.x, pos.z, false);
	rot.x = Clamp(rot.x, math::PI * 0.51f, math::PI * 0.99f);

	camera->SetRot(float3(rot.x, GetAzimuth(), rot.z));
	dir = camera->GetDir();

	curDist = Clamp(curDist, 20.0f, maxDist);
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * curDist * 2.0f;

	scrollSpeed = configHandler->GetInt("CamSpringScrollSpeed") * 0.1f;
	fov = configHandler->GetFloat("CamSpringFOV");
}


static float GetRotationWithCardinalLock(float rot)
{
	constexpr float cardinalDirLockWidth = 0.2f;

	const float rotMoved = std::abs(rot /= math::HALFPI) - cardinalDirLockWidth * 0.5f;
	const float numerator = std::trunc(rotMoved);

	const float fract = rotMoved - numerator;
	const float b = 1.0f / (1.0f - cardinalDirLockWidth);
	const float c = 1.0f - b;
	const float fx = (fract > cardinalDirLockWidth) ? fract * b + c : 0.0f;

	return std::copysign(numerator + fx, rot) * math::HALFPI;
}


float CSpringController::MoveAzimuth(float move)
{
	const float minRot = std::floor(rot.y / math::HALFPI) * math::HALFPI;
	const float maxRot = std::ceil(rot.y / math::HALFPI) * math::HALFPI;

	rot.y -= move;

	if (configHandler->GetBool("CamSpringLockCardinalDirections"))
		return GetRotationWithCardinalLock(rot.y);
	if (KeyInput::GetKeyModState(KMOD_CTRL))
		rot.y = Clamp(rot.y, minRot + 0.02f, maxRot - 0.02f);

	return rot.y;
}


float CSpringController::GetAzimuth() const
{
	if (configHandler->GetBool("CamSpringLockCardinalDirections"))
		return GetRotationWithCardinalLock(rot.y);
	return rot.y;
}


float3 CSpringController::GetPos() const
{
	const float3 cvec = dir * curDist;
	const float3 cpos = pos - cvec;
	return {cpos.x, std::max(cpos.y, CGround::GetHeightAboveWater(cpos.x, cpos.z, false) + 5.0f), cpos.z};
}


void CSpringController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText)
		LOG("Switching to Spring style camera");

	if (oldCam == CCameraHandler::CAMERA_MODE_OVERVIEW)
		return;

	rot = camera->GetRot() * XZVector;
}


void CSpringController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);
	sm["dist"] = curDist;
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}


bool CSpringController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);
	SetStateFloat(sm, "dist", curDist);
	SetStateFloat(sm, "rx",   rot.x);
	SetStateFloat(sm, "ry",   rot.y);
	SetStateFloat(sm, "rz",   rot.z);
	return true;
}
