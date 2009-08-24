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

static inline float fCropTo(const float& val, const float& min, const float& max) {

	if (val < min) {
		return min;
	} else if (val > max) {
		return max;
	} else {
		return val;
	}
}

// Uses distance to ground for large angles (near 90 degree),
// and distance to unit for flat angles (near 0 degree),
// when comparing the camera direction to the map surface,
// assuming the map is flat.
bool CCameraController::GetUseDistToGroundForIcons() {

	const float3& dir     = GetDir().UnsafeNormalize();
	const float dot       = fCropTo(fabs(dir.dot(UpVector)), 0.0f, 1.0f);
	const float switchVal = configHandler->Get("UseDistToGroundForIcons", 0.8f);

	// switchVal:
	// * 1.0 = 0 degree  = overview
	// * 0.0 = 90 degree = first person
	if (dot < switchVal) {
		// flat angle (typical for first person camera)
		return false;
	} else {
		// steep angle (typical for overhead camera)
		return true;
	}
}
