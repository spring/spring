
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


CARController::CARController()
{
	mouseScale  = configHandler->GetFloat("ARMouseScale");
	scrollSpeed = configHandler->GetInt("ARScrollSpeed") * 0.1f;
	enabled     = configHandler->GetBool("AREnabled");
	fov         = configHandler->GetFloat("ARFOV");
}


void CARController::KeyMove(float3 move)
{
	
}


void CARController::MouseMove(float3 move)
{

}


void CARController::ScreenEdgeMove(float3 move)
{
}


void CARController::MouseWheelMove(float move)
{

	
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
	CCameraController::GetState(sm);
}


bool CARController::SetState(const StateMap& sm)
{

	CCameraController::SetState(sm);

	return true;
}

