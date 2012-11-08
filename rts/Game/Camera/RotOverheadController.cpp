/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RotOverheadController.h"

#include "System/Config/ConfigHandler.h"
#include "Game/Camera.h"
#include "System/Log/ILog.h"
#include "Map/Ground.h"
#include "System/myMath.h"

CONFIG(float, RotOverheadMouseScale).defaultValue(0.01f);
CONFIG(int, RotOverheadScrollSpeed).defaultValue(10);
CONFIG(bool, RotOverheadEnabled).defaultValue(true);
CONFIG(float, RotOverheadFOV).defaultValue(45.0f);


CRotOverheadController::CRotOverheadController()
	: oldHeight(500)
{
	mouseScale  = configHandler->GetFloat("RotOverheadMouseScale");
	scrollSpeed = configHandler->GetInt("RotOverheadScrollSpeed") * 0.1f;
	enabled     = configHandler->GetBool("RotOverheadEnabled");
	fov         = configHandler->GetFloat("RotOverheadFOV");
	UpdateVectors();
}


void CRotOverheadController::KeyMove(float3 move)
{
	move *= math::sqrt(move.z) * 400;

	float3 flatForward = camera->forward;
	if(camera->forward.y < -0.9f)
		flatForward += camera->up;
	flatForward.y = 0;
	flatForward.ANormalize();

	pos += (flatForward * move.y + camera->right * move.x) * scrollSpeed;
	UpdateVectors();
}


void CRotOverheadController::MouseMove(float3 move)
{
	camera->rot.y -= mouseScale * move.x;
	camera->rot.x -= mouseScale * move.y * move.z;
	camera->rot.x = Clamp(camera->rot.x, -PI*0.4999f, PI*0.4999f);
	UpdateVectors();
}


void CRotOverheadController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}


void CRotOverheadController::MouseWheelMove(float move)
{
	const float gheight = ground->GetHeightAboveWater(pos.x, pos.z, false);
	float height = pos.y - gheight;
	height *= 1.0f + (move * mouseScale);
	pos.y = height + gheight;
	UpdateVectors();
}

void CRotOverheadController::UpdateVectors()
{
	dir.x=(float)(math::sin(camera->rot.y) * math::cos(camera->rot.x));
	dir.y=(float)(math::sin(camera->rot.x));
	dir.z=(float)(math::cos(camera->rot.y) * math::cos(camera->rot.x));
	dir.ANormalize();

	pos.x = Clamp(pos.x, 0.01f, gs->mapx * SQUARE_SIZE - 0.01f);
	pos.z = Clamp(pos.z, 0.01f, gs->mapy * SQUARE_SIZE - 0.01f);

	float h = ground->GetHeightAboveWater(pos.x, pos.z, false);
	pos.y = Clamp(pos.y, h + 5, 9000.0f);
	oldHeight = pos.y - h;
}

void CRotOverheadController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);
	pos.y = ground->GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;
	UpdateVectors();
}


float3 CRotOverheadController::SwitchFrom() const
{
	return pos;
}


void CRotOverheadController::SwitchTo(bool showText)
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

