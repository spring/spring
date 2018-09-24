
/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ARController.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/myMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

CONFIG(float, ARMouseScale).defaultValue(0.01f);
CONFIG(int, ARScrollSpeed).defaultValue(10);
CONFIG(bool, AREnabled).defaultValue(true).headlessValue(false);
CONFIG(float, ARFOV).defaultValue(45.0f);
CONFIG(float, ARScreenDivideRatio).defaultValue(0.5f);


CARController::CARController()
{
	mouseScale  = configHandler->GetFloat("ARMouseScale");
	scrollSpeed = configHandler->GetInt("ARScrollSpeed") * 0.1f;
	enabled     = configHandler->GetBool("AREnabled");
	fov         = configHandler->GetFloat("ARFOV");
	screenDivideRatio = configHandler->GetFloat("ARScreenDivideRatio");
}


void CARController::KeyMove(float3 move)
{
	Update();
}


void CARController::MouseMove(float3 move)
{
	Update();
}


void CARController::ScreenEdgeMove(float3 move)
{
	KeyMove(move);
}


void CARController::MouseWheelMove(float move)
{

	Update();
}

void CARController::Update()
{
}

void CARController::SetPos(const float3& newPos)
{
	CCameraController::SetPos(newPos);
	Update();
}


float3 CARController::SwitchFrom() const
{
	return pos;
}


void CARController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText) {
		LOG("Switching to Augmented Reality camera");
	}
}



void CARController::GetState(StateMap& sm) const
{
	sm["screenDivideRatio"] = screenDivideRatio;
	CCameraController::GetState(sm);
}


bool CARController::SetState(const StateMap& sm)
{

	SetStateFloat(sm, "screenDivideRatio", screenDivideRatio);
	CCameraController::SetState(sm);

	return true;
}

