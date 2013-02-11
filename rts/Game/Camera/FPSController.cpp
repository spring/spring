/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FPSController.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"

using std::min;
using std::max;

CONFIG(int, FPSScrollSpeed).defaultValue(10);
CONFIG(float, FPSMouseScale).defaultValue(0.01f);
CONFIG(bool, FPSEnabled).defaultValue(true);
CONFIG(float, FPSFOV).defaultValue(45.0f);


CFPSController::CFPSController()
	: oldHeight(300)
{
	scrollSpeed = configHandler->GetInt("FPSScrollSpeed") * 0.1f;
	mouseScale = configHandler->GetFloat("FPSMouseScale");
	enabled = configHandler->GetBool("FPSEnabled");
	fov = configHandler->GetFloat("FPSFOV");
	UpdateVectors();
}


void CFPSController::KeyMove(float3 move)
{
	move *= move.z * 400;
	pos  += (camera->forward * move.y + camera->right * move.x) * scrollSpeed;
	UpdateVectors();
}


void CFPSController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale * move.x;
	camera->rot.x -= mouseScale * move.y * move.z;
	camera->rot.x = Clamp(camera->rot.x, -PI*0.4999f, PI*0.4999f);
	UpdateVectors();
}


void CFPSController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}


void CFPSController::MouseWheelMove(float move)
{
	pos += camera->up * move;
	UpdateVectors();
}


void CFPSController::UpdateVectors()
{
	if (!gu->fpsMode) {
		const float margin = 0.01f;
		const float xMin = margin;
		const float zMin = margin;
		const float xMax = (float)(gs->mapx * SQUARE_SIZE) - margin;
		const float zMax = (float)(gs->mapy * SQUARE_SIZE) - margin;

		pos.x = Clamp(pos.x, xMin, xMax);
		pos.z = Clamp(pos.z, zMin, zMax);

		const float gndHeight = ground->GetHeightAboveWater(pos.x, pos.z, false);
		const float yMin = gndHeight + 5.0f;
		const float yMax = 9000.0f;
		pos.y = Clamp(pos.y, yMin, yMax);
		oldHeight = pos.y - gndHeight;
	}

	dir.x = (float)(math::cos(camera->rot.x) * math::sin(camera->rot.y));
	dir.z = (float)(math::cos(camera->rot.x) * math::cos(camera->rot.y));
	dir.y = (float)(math::sin(camera->rot.x));
	dir.ANormalize();
}


void CFPSController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);

	if (!gu->fpsMode) {
		pos.y = ground->GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;
	}
	UpdateVectors();
}


void CFPSController::SetDir(const float3& newDir)
{
	dir = newDir;
	UpdateVectors();
}


float3 CFPSController::SwitchFrom() const
{
	return pos;
}


void CFPSController::SwitchTo(bool showText)
{
	if (showText) {
		LOG("Switching to FPS style camera");
	}
}


void CFPSController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);
	sm["oldHeight"] = oldHeight;
}


bool CFPSController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);
	SetStateFloat(sm, "oldHeight", oldHeight);

	return true;
}


