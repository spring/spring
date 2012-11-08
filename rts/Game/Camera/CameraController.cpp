/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CameraController.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"


CONFIG(float, UseDistToGroundForIcons).defaultValue(0.95f);


CCameraController::CCameraController()
{
	// switchVal:
	// * 1.0 = 0 degree  = overview
	// * 0.0 = 90 degree = first person
	switchVal = configHandler->GetFloat("UseDistToGroundForIcons");
	scrollSpeed = 1;
	fov = 45.0f;
	pixelSize = 1.0f;
	enabled = true;
	pos = float3(gs->mapx * 0.5f * SQUARE_SIZE, 1000.f, gs->mapy * 0.5f * SQUARE_SIZE); // center map
	dir = float3(0.0f,0.0f,1.0f);
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
	const float dot       = std::min(1.0f, std::max(0.0f, math::fabs(dir.dot(UpVector))));

	if (dot < switchVal) {
		// flat angle (typical for first person camera)
		return false;
	} else {
		// steep angle (typical for overhead camera)
		return true;
	}
}



bool CCameraController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "fov", fov);

	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);

	SetStateFloat(sm, "dx", dir.x);
	SetStateFloat(sm, "dy", dir.y);
	SetStateFloat(sm, "dz", dir.z);

	return true;
}

void CCameraController::GetState(StateMap& sm) const
{
	sm["fov"] = fov;

	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;

	sm["dx"] = dir.x;
	sm["dy"] = dir.y;
	sm["dz"] = dir.z;
}

