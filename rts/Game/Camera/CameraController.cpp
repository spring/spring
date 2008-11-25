#include "StdAfx.h"
#include "mmgr.h"

#include "CameraController.h"

#include "ConfigHandler.h"


CCameraController::CCameraController() : pos(2000, 70, 1800)
{
	mouseScale = configHandler.Get("FPSMouseScale", 0.01f);
	scrollSpeed = 1;
	fov = 45.0f;
	enabled = true;
}


CCameraController::~CCameraController(void)
{
}


bool CCameraController::SetStateBool(const StateMap& sm,
                                     const std::string& name, bool& var)
{
	StateMap::const_iterator it = sm.find(name);
	if (it != sm.end()) {
		const float value = it->second;
		var = (value > 0.0f);
		return true;
	}
	return false;
}


bool CCameraController::SetStateFloat(const StateMap& sm,
                                      const std::string& name, float& var)
{
	StateMap::const_iterator it = sm.find(name);
	if (it != sm.end()) {
		const float value = it->second;
		var = value;
		return true;
	}
	return false;
}
