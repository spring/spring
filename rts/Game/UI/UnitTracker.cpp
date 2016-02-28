/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "UnitTracker.h"
#include "Game/Camera/FPSController.h"
#include "Game/CameraHandler.h"
#include "Game/Camera.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"


CUnitTracker unitTracker;


const char* CUnitTracker::modeNames[TrackModeCount] = {
	"Single",
	"Average",
	"Extents"
};


CUnitTracker::CUnitTracker():
	enabled(false),
	firstUpdate(true),
	trackMode(TrackSingle),
	trackUnit(0),
	timeOut(15),
	lastFollowUnit(0),
	lastUpdateTime(0.0f),
	trackPos(500.0f, 100.0f, 500.0f),
	trackDir(FwdVector),
	smoothedRight(RgtVector),
	oldCamDir(RgtVector),
	oldCamPos(500.0f, 500.0f, 500.0f)
{
}


CUnitTracker::~CUnitTracker()
{
}


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
	if (mode < 0) {
		trackMode = 0;
	} else if (mode >= TrackModeCount) {
		trackMode = TrackModeCount - 1;
	} else {
		trackMode = mode;
	}
	LOG("TrackMode: %s", modeNames[trackMode]);
}


/******************************************************************************/

void CUnitTracker::Track()
{
	CUnitSet& units = selectedUnitsHandler.selectedUnits;

	CleanTrackGroup();

	if (trackGroup.empty()) {
		if (units.empty()) {
			Disable();
		} else {
			MakeTrackGroup();
			trackUnit = *trackGroup.begin();
			enabled = true;
		}
	} else {
		if (!units.empty()) {
			MakeTrackGroup();
		}
		if (trackGroup.find(trackUnit) == trackGroup.end()) {
			trackUnit = *trackGroup.begin();
			enabled = true;
		} else if (enabled) {
			if (trackMode != TrackSingle) {
				trackMode = TrackSingle;
				LOG("TrackMode: %s", modeNames[TrackSingle]);
			}
			NextUnit();
		} else {
			enabled = true;
		}
	}
}


void CUnitTracker::MakeTrackGroup()
{
	smoothedRight = RgtVector;
	trackGroup.clear();
	CUnitSet& units = selectedUnitsHandler.selectedUnits;
	CUnitSet::const_iterator it;
	for (it = units.begin(); it != units.end(); ++it) {
		trackGroup.insert((*it)->id);
	}
}


void CUnitTracker::CleanTrackGroup()
{
	std::set<int>::iterator it = trackGroup.begin();

	while (it != trackGroup.end()) {
		if (unitHandler->GetUnitUnsafe(*it) != NULL) {
			++it;
			continue;
		}
		std::set<int>::iterator it_next = it;
		++it_next;
		if (trackUnit == *it) {
			if (it_next == trackGroup.end()) {
				trackUnit = *trackGroup.begin();
			} else {
				trackUnit = *it_next;
			}
		}
		trackGroup.erase(it);
		it = it_next;
	}

	if (trackGroup.empty()) {
		Disable();
	}
	else if (trackGroup.find(trackUnit) == trackGroup.end()) {
		trackUnit = *trackGroup.begin();
	}
}


void CUnitTracker::NextUnit()
{
	if (trackGroup.empty()) {
		return;
	}

	std::set<int>::iterator it = trackGroup.find(trackUnit);
	if (it == trackGroup.end()) {
		trackUnit = *trackGroup.begin();
	}
	else {
		++it;
		if (it == trackGroup.end()) {
			trackUnit = *trackGroup.begin();
			Disable();
		} else {
			trackUnit = *it;
		}
	}
}


CUnit* CUnitTracker::GetTrackUnit()
{
	CleanTrackGroup();

	if (trackGroup.empty()) {
		Disable();
		return NULL;
	}

	return unitHandler->GetUnitUnsafe(trackUnit);
}


float3 CUnitTracker::CalcAveragePos() const
{
	float3 p(ZeroVector);
	std::set<int>::const_iterator it;
	for (it = trackGroup.begin(); it != trackGroup.end(); ++it) {
		p += unitHandler->GetUnitUnsafe(*it)->drawPos;
	}
	p /= (float)trackGroup.size();
	return p;
}


float3 CUnitTracker::CalcExtentsPos() const
{
	float3 minPos(+1e9f, +1e9f, +1e9f);
	float3 maxPos(-1e9f, -1e9f, -1e9f);
	std::set<int>::const_iterator it;
	for (it = trackGroup.begin(); it != trackGroup.end(); ++it) {
		const float3& p = unitHandler->GetUnitUnsafe(*it)->drawPos;

		if (p.x < minPos.x) { minPos.x = p.x; }
		if (p.y < minPos.y) { minPos.y = p.y; }
		if (p.z < minPos.z) { minPos.z = p.z; }
		if (p.x > maxPos.x) { maxPos.x = p.x; }
		if (p.y > maxPos.y) { maxPos.y = p.y; }
		if (p.z > maxPos.z) { maxPos.z = p.z; }
	}
	return (minPos + maxPos) / 2.0f;
}


/******************************************************************************/

void CUnitTracker::SetCam()
{
	if (firstUpdate) {
		firstUpdate = false;
	}

	CUnit* u = GetTrackUnit();
	if (!u) {
		Disable();
		return;
	}

	if (lastFollowUnit != 0 && unitHandler->GetUnitUnsafe(lastFollowUnit) == 0) {
		timeOut = 1;
		lastFollowUnit = 0;
	}

	if (timeOut > 0) {
		// Transition between 2 targets
		timeOut++;
		camera->SetDir(oldCamDir);
		camera->SetPos(oldCamPos);
		if (camHandler->GetCurrentControllerNum() == CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
			camHandler->GetCurrentController().SetDir(oldCamDir);
			camHandler->GetCurrentController().SetPos(oldCamPos);
		}
		if (timeOut > 15) {
			timeOut = 0;
		}
		camHandler->UpdateTransition();

	} else if (camHandler->GetCurrentControllerNum() != CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
		// non-FPS camera modes  (immediate positional tracking)
		float3 pos;
		switch (trackMode) {
			case TrackAverage: {
				pos = CalcAveragePos();
				break;
			}
			case TrackExtents: {
				pos = CalcExtentsPos();
				break;
			}
			default: {
				pos = u->drawMidPos;
				break;
			}
		}
		camHandler->GetCurrentController().SetTrackingInfo(pos, u->radius * 2.7182818f);
		camHandler->UpdateTransition();

	} else {
		// FPS Camera
		const float deltaTime = gs->frameNum + globalRendering->timeOffset - lastUpdateTime;
		lastUpdateTime = gs->frameNum + globalRendering->timeOffset;

		float3 modPlanePos(u->drawMidPos - (u->frontdir * u->radius * 3));
		const float minHeight = CGround::GetHeightReal(modPlanePos.x, modPlanePos.z, false) + (u->radius * 2);
		modPlanePos.y = std::max(modPlanePos.y, minHeight);

		trackPos += (modPlanePos - trackPos) * (1 - math::pow(0.95f, deltaTime));
		trackDir += (u->frontdir - trackDir) * (1 - math::pow(0.90f, deltaTime));
		trackDir.ANormalize();

		camera->SetPos(trackPos);

		float3 wantedDir = u->drawMidPos - camera->GetPos();
		wantedDir.ANormalize();
		wantedDir += trackDir;
		wantedDir.ANormalize();
		camera->SetDir(wantedDir);

		CFPSController& fpsCamera = static_cast<CFPSController&>(camHandler->GetCurrentController());
		fpsCamera.SetDir(camera->GetDir());
		fpsCamera.SetPos(trackPos);

		const float3 right = mix<float3>(smoothedRight, mix<float3>(u->rightdir, RgtVector, 0.75f), deltaTime * 0.05f).ANormalize();
		camera->SetRotZ(std::atan2(right.y, right.Length2D()));
		smoothedRight = right;

		oldCamDir = camera->GetDir();
		oldCamPos = camera->GetPos();
	}
}
