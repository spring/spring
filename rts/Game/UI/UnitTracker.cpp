/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "UnitTracker.h"
#include "Game/Camera/FPSController.h"
#include "Game/CameraHandler.h"
#include "Game/Camera.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"


CUnitTracker unitTracker;


const char* CUnitTracker::modeNames[TrackModeCount] = {
	"Single",
	"Average",
	"Extents"
};


void CUnitTracker::Disable()
{
	smoothedRight = RgtVector;
	enabled = false;
}


int CUnitTracker::GetMode() const
{
	return trackMode;
}


void CUnitTracker::IncMode()
{
	trackMode = (trackMode + 1) % TrackModeCount;
	LOG("TrackMode: %s", modeNames[trackMode]);
}


void CUnitTracker::SetMode(int mode)
{
	trackMode = Clamp(mode, 0, TrackModeCount - 1);
	LOG("TrackMode: %s", modeNames[trackMode]);
}


/******************************************************************************/

void CUnitTracker::Track()
{
	const auto& units = selectedUnitsHandler.selectedUnits;

	CleanTrackGroup();

	if (trackedUnitIDs.empty()) {
		if (units.empty()) {
			Disable();
		} else {
			MakeTrackGroup();
			trackUnit = *trackedUnitIDs.begin();
			enabled = true;
		}

		return;
	}

	if (!units.empty())
		MakeTrackGroup();

	if (trackedUnitIDs.find(trackUnit) == trackedUnitIDs.end()) {
		trackUnit = *trackedUnitIDs.begin();
		enabled = true;
		return;
	}

	if (enabled) {
		if (trackMode != TrackSingle) {
			trackMode = TrackSingle;
			LOG("TrackMode: %s", modeNames[TrackSingle]);
		}
		NextUnit();
		return;
	}

	enabled = true;
}


void CUnitTracker::MakeTrackGroup()
{
	smoothedRight = RgtVector;
	trackedUnitIDs.clear();

	for (const int unitID: selectedUnitsHandler.selectedUnits) {
		trackedUnitIDs.insert(unitID);
	}
}


void CUnitTracker::CleanTrackGroup()
{
	deadUnitIDs.clear();
	deadUnitIDs.reserve(trackedUnitIDs.size());

	for (const int unitID: trackedUnitIDs) {
		if (unitHandler.GetUnitUnsafe(unitID) == nullptr)
			deadUnitIDs.push_back(unitID);
	}

	for (const int deadUnitID: deadUnitIDs) {
		trackedUnitIDs.erase(deadUnitID);
	}

	if (trackedUnitIDs.empty()) {
		Disable();
		return;
	}

	// reset trackUnit if it was erased above
	if (trackedUnitIDs.find(trackUnit) != trackedUnitIDs.end())
		return;

	trackUnit = *trackedUnitIDs.begin();
}


void CUnitTracker::NextUnit()
{
	if (trackedUnitIDs.empty())
		return;

	auto it = trackedUnitIDs.find(trackUnit);

	if (it == trackedUnitIDs.end()) {
		trackUnit = *trackedUnitIDs.begin();
		return;
	}

	if ((++it) == trackedUnitIDs.end()) {
		trackUnit = *trackedUnitIDs.begin();
		Disable();
	} else {
		trackUnit = *it;
	}
}


CUnit* CUnitTracker::GetTrackUnit()
{
	CleanTrackGroup();

	if (trackedUnitIDs.empty()) {
		Disable();
		return nullptr;
	}

	return unitHandler.GetUnitUnsafe(trackUnit);
}


float3 CUnitTracker::CalcAveragePos() const
{
	float3 p;

	for (const int unitID: trackedUnitIDs) {
		p += unitHandler.GetUnitUnsafe(unitID)->drawPos;
	}

	return (p / trackedUnitIDs.size());
}


float3 CUnitTracker::CalcExtentsPos() const
{
	float3 minPos(+1e9f, +1e9f, +1e9f);
	float3 maxPos(-1e9f, -1e9f, -1e9f);

	for (const int unitID: trackedUnitIDs) {
		const float3& p = unitHandler.GetUnitUnsafe(unitID)->drawPos;

		minPos = float3::min(minPos, p);
		maxPos = float3::max(maxPos, p);
	}

	return ((minPos + maxPos) * 0.5f);
}


/******************************************************************************/

void CUnitTracker::SetCam()
{
	CUnit* u = GetTrackUnit();

	if (u == nullptr) {
		Disable();
		return;
	}

	if (lastFollowUnit != 0 && unitHandler.GetUnitUnsafe(lastFollowUnit) == nullptr) {
		timeOut = 1;
		lastFollowUnit = 0;
	}

	if (timeOut > 0) {
		// Transition between 2 targets
		camera->SetDir(oldCamDir);
		camera->SetPos(oldCamPos);

		if (camHandler->GetCurrentControllerNum() == CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
			camHandler->GetCurrentController().SetDir(oldCamDir);
			camHandler->GetCurrentController().SetPos(oldCamPos);
		}

		timeOut += 1;
		timeOut *= (timeOut <= 15);

		camHandler->UpdateTransition();
	} else if (camHandler->GetCurrentControllerNum() != CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
		// non-FPS camera modes  (immediate positional tracking)
		float3 pos;
		switch (trackMode) {
			case TrackAverage: {
				pos = CalcAveragePos();
			} break;
			case TrackExtents: {
				pos = CalcExtentsPos();
			} break;
			default: {
				pos = u->drawMidPos;
			} break;
		}

		camHandler->GetCurrentController().SetTrackingInfo(pos, u->radius * 2.7182818f);
		camHandler->UpdateTransition();
	} else {
		// FPS Camera
		const float offsetTime = gs->frameNum + globalRendering->timeOffset;
		const float deltaTime = offsetTime - lastUpdateTime;
		lastUpdateTime = offsetTime;

		const float3 modFrontVec = u->frontdir * u->radius * 3.0f;
		const float3 mixRightDir = mix<float3>(u->rightdir, RgtVector, 0.75f); // NB: will be 0 if u->r == -R
		      float3 modPlanePos = u->drawMidPos - modFrontVec;

		modPlanePos.y = std::max(modPlanePos.y, CGround::GetHeightReal(modPlanePos.x, modPlanePos.z, false) + (u->radius * 2.0f));

		trackPos += (modPlanePos - trackPos) * (1 - math::pow(0.95f, deltaTime));
		trackDir += (u->frontdir - trackDir) * (1 - math::pow(0.90f, deltaTime));
		smoothedRight = mix<float3>(smoothedRight, mixRightDir, deltaTime * 0.05f).SafeANormalize();

		const float3 wantedDir = (u->drawMidPos - camera->GetPos()).SafeANormalize();
		const float3 cameraDir = (wantedDir + trackDir.SafeANormalize()).SafeANormalize();

		camera->SetPos(trackPos);
		camera->SetDir(cameraDir);

		CFPSController& fpsCamera = static_cast<CFPSController&>(camHandler->GetCurrentController());
		fpsCamera.SetDir(camera->GetDir());
		fpsCamera.SetPos(camera->GetPos());

		camera->SetRotZ(std::atan2(smoothedRight.y, smoothedRight.Length2D()));

		oldCamDir = camera->GetDir();
		oldCamPos = camera->GetPos();
	}
}
