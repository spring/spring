/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "CameraController.h"
#include "System/ConfigHandler.h"

CCameraController::CCameraController()
{
	// switchVal:
	// * 1.0 = 0 degree  = overview
	// * 0.0 = 90 degree = first person
	switchVal = configHandler->Get("UseDistToGroundForIcons", 0.95f);
	scrollSpeed = 1;
	fov = 45.0f;
	pixelSize = 1.0f;
	enabled = true;
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

	const float3& dir     = GetDir().UnsafeNormalize();
	const float dot       = std::min(1.0f, std::max(0.0f, fabs(dir.dot(UpVector))));

	if (dot < switchVal) {
		// flat angle (typical for first person camera)
		return false;
	} else {
		// steep angle (typical for overhead camera)
		return true;
	}
}
