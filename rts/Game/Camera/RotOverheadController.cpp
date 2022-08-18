/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RotOverheadController.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/SpringMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include "Game/CameraHandler.h"

CONFIG(float, RotOverheadMouseScale).defaultValue(0.01f);
CONFIG(int, RotOverheadScrollSpeed).defaultValue(10);
CONFIG(bool, RotOverheadEnabled).defaultValue(true).headlessValue(false);
CONFIG(float, RotOverheadFOV).defaultValue(45.0f);
CONFIG(bool, RotOverheadClampMap).defaultValue(true).headlessValue(true);


CRotOverheadController::CRotOverheadController(): oldHeight(500.0f)
{
	mouseScale  = configHandler->GetFloat("RotOverheadMouseScale");
	scrollSpeed = configHandler->GetInt("RotOverheadScrollSpeed") * 0.1f;
	enabled     = configHandler->GetBool("RotOverheadEnabled");
	fov         = configHandler->GetFloat("RotOverheadFOV");
	clampToMap = configHandler->GetBool("RotOverheadClampMap");
}


void CRotOverheadController::KeyMove(float3 move)
{
	move *= math::sqrt(move.z) * 400;

	float3 flatForward = camera->GetDir();
	if(camera->GetDir().y < -0.9f)
		flatForward += camera->GetUp();
	flatForward.y = 0;
	flatForward.ANormalize();

	pos += (flatForward * move.y + camera->GetRight() * move.x) * scrollSpeed;
	Update();
}


void CRotOverheadController::MouseMove(float3 move)
{
	// use local dir state so CameraHandler can create smooth transition between
	// current camera rot and desired
	auto rot = CCamera::GetRotFromDir(dir);
	rot.x = Clamp(rot.x + mouseScale * move.y * move.z, math::PI * 0.4999f, math::PI * 0.9999f);

	float new_rot_y = ClampRad(rot.y + mouseScale * move.x + math::PI) - math::PI;
	float cam_rot_y = camera->GetRot().y;
	bool over_half_rot_y = (GetRadAngleToward(cam_rot_y, new_rot_y) *
			GetRadAngleToward(cam_rot_y , rot.y) > 0.0f) &&
			(fabsf(GetRadAngleToward(cam_rot_y, rot.y)) > math::HALFPI);

	if (!over_half_rot_y) {
		// pending rotation can't be over 180deg as CameraHandler will smooth it
		// in the opposite direction
		rot.y = new_rot_y;
	}

	dir = CCamera::GetFwdFromRot(rot);
	Update();
}


void CRotOverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}


void CRotOverheadController::MouseWheelMove(float move)
{
	const float gheight = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	float height = pos.y - gheight;

	height *= (1.0f + (move * mouseScale));
	pos.y = height + gheight;

	Update();
}

void CRotOverheadController::Update()
{
	if (clampToMap) {
		pos.x = Clamp(pos.x, 0.01f, mapDims.mapx * SQUARE_SIZE - 0.01f);
		pos.z = Clamp(pos.z, 0.01f, mapDims.mapy * SQUARE_SIZE - 0.01f);
	}
	else {
		pos.x = Clamp(pos.x, -1.0f * mapDims.mapx * SQUARE_SIZE, 2.0f * mapDims.mapx * SQUARE_SIZE - 0.01f);
		pos.z = Clamp(pos.z, -1.0f * mapDims.mapy * SQUARE_SIZE, 2.0f * mapDims.mapy * SQUARE_SIZE - 0.01f);
	}

	float h = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	pos.y = Clamp(pos.y, h + 5, 9000.0f);
	oldHeight = pos.y - h;
}

void CRotOverheadController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;
	Update();
}


float3 CRotOverheadController::SwitchFrom() const
{
	return pos;
}


void CRotOverheadController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText) {
		LOG("Switching to Rotatable overhead camera");
	}
}


void CRotOverheadController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);

	sm["oldHeight"] = oldHeight;
}


bool CRotOverheadController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);

	SetStateFloat(sm, "oldHeight", oldHeight);

	return true;
}

