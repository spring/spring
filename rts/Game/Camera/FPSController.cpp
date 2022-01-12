/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FPSController.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

CONFIG(int, FPSScrollSpeed).defaultValue(10);
CONFIG(float, FPSMouseScale).defaultValue(0.01f);
CONFIG(bool, FPSEnabled).defaultValue(true);
CONFIG(float, FPSFOV).defaultValue(45.0f);
CONFIG(bool, FPSClampPos).defaultValue(true);


CFPSController::CFPSController(): oldHeight(300.0f)
{
	ConfigUpdate();
	dir = camera->GetDir();

	configHandler->NotifyOnChange(this, {"FPSScrollSpeed", "FPSMouseScale", "FPSEnabled", "FPSFOV", "FPSClampPos"});
}

CFPSController::~CFPSController()
{
	configHandler->RemoveObserver(this);
}

void CFPSController::ConfigUpdate()
{
	scrollSpeed = configHandler->GetInt("FPSScrollSpeed") * 0.1f;
	mouseScale = configHandler->GetFloat("FPSMouseScale");
	enabled = configHandler->GetBool("FPSEnabled");
	fov = configHandler->GetFloat("FPSFOV");
	clampPos = configHandler->GetBool("FPSClampPos");
}

void CFPSController::ConfigNotify(const std::string& key, const std::string& value)
{
	ConfigUpdate();
}


void CFPSController::KeyMove(float3 move)
{
	move *= move.z * 400;
	pos  += (camera->GetDir() * move.y + camera->GetRight() * move.x) * scrollSpeed;
	Update();
}


void CFPSController::MouseMove(float3 move)
{
	camera->SetRotY(camera->GetRot().y + mouseScale * move.x);
	camera->SetRotX(Clamp(camera->GetRot().x + mouseScale * move.y * move.z, 0.01f, math::PI * 0.99f));
	dir = camera->GetDir();
	Update();
}


void CFPSController::MouseWheelMove(float move)
{
	pos += (camera->GetUp() * move);
	Update();
}


void CFPSController::Update()
{
	if (gu->fpsMode || !clampPos)
		return;

	const float margin = 0.01f;
	const float xMin = margin;
	const float zMin = margin;
	const float xMax = (float)(mapDims.mapx * SQUARE_SIZE) - margin;
	const float zMax = (float)(mapDims.mapy * SQUARE_SIZE) - margin;

	pos.x = Clamp(pos.x, xMin, xMax);
	pos.z = Clamp(pos.z, zMin, zMax);

	const float gndHeight = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	const float yMin = gndHeight + 5.0f;
	const float yMax = 9000.0f;

	pos.y = Clamp(pos.y, yMin, yMax);
	oldHeight = pos.y - gndHeight;
}


void CFPSController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);

	if (!gu->fpsMode)
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;

	Update();
}


void CFPSController::SetDir(const float3& newDir)
{
	dir = newDir;
	Update();
}


void CFPSController::SwitchTo(const int oldCam, const bool showText)
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


