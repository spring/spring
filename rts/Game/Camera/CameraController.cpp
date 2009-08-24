#include "StdAfx.h"
#include "mmgr.h"

#include "CameraController.h"

#include "ConfigHandler.h"


CCameraController::CCameraController() : pos(2000, 70, 1800)
{
	mouseScale = configHandler->Get("FPSMouseScale", 0.01f);
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

// Uses distance to ground for large angles (near 90 degree),
// and distance to unit for flat angles (near 0 degree),
// when comparing the camera direction to the map surface,
// assuming the map is flat.
bool CCameraController::GetUseDistToGroundForIcons() {

	const float3& dir  = GetDir().UnsafeANormalize();
	const float dot    = fabs(dir.dot(UpVector));

	// dot: 1.0=overview, 0.0=first person
	if (dot < 0.8) {
		// flat angle (typical for first person camera)
		return false;
	} else {
		// steep angle (typical for overhead camera)
		return true;
	}
}
