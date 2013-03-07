/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "UnitTracker.h"
#include "Game/Camera/FPSController.h"
#include "Game/CameraHandler.h"
#include "Game/Camera.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"


CUnitTracker unitTracker;


const char* CUnitTracker::modeNames[TrackModeCount] = {
	"Single",
	"Average",
	"Extents"
};


CUnitTracker::CUnitTracker():
	enabled(false),
	doRoll(false),
	firstUpdate(true),
	trackMode(TrackSingle),
	trackUnit(0),
	timeOut(15),
	lastFollowUnit(0),
	lastUpdateTime(0.0f),
	trackPos(500.0f, 100.0f, 500.0f),
	trackDir(0.0f, 0.0f, 1.0f),
	oldCamDir(1.0f, 0.0f, 0.0f),
	oldCamPos(500.0f, 500.0f, 500.0f)
{
	for (size_t a = 0; a < 32; ++a) {
		oldUp[a] = UpVector;
	}
}


CUnitTracker::~CUnitTracker()
{
}


void CUnitTracker::Disable()
{
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
	GML_RECMUTEX_LOCK(sel); // Track

	CUnitSet& units = selectedUnits.selectedUnits;

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
	GML_RECMUTEX_LOCK(sel); // MakeTrackGroup

	trackGroup.clear();
	CUnitSet& units = selectedUnits.selectedUnits;
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
		doRoll = !configHandler->GetInt("ReflectiveWater");
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
		camera->forward = oldCamDir;
		camera->pos = oldCamPos;
		if (camHandler->GetCurrentControllerNum() == CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
			camHandler->GetCurrentController().SetDir(oldCamDir);
			camHandler->GetCurrentController().SetPos(oldCamPos);
		}
		if (timeOut > 15) {
			timeOut = 0;
		}
		camHandler->UpdateCam();
		camera->Update();

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
		camHandler->UpdateCam();
		camera->Update();

	} else {
		// FPS Camera
		const float deltaTime = gs->frameNum + globalRendering->timeOffset - lastUpdateTime;
		lastUpdateTime = gs->frameNum + globalRendering->timeOffset;

		float3 modPlanePos(u->drawPos - (u->frontdir * u->radius * 3));
		const float minHeight = ground->GetHeightReal(modPlanePos.x, modPlanePos.z, false) + (u->radius * 2);
		if (modPlanePos.y < minHeight) {
  			modPlanePos.y = minHeight;
		}

		trackPos += (modPlanePos - trackPos) * (1 - math::pow(0.95f, deltaTime));
		trackDir += (u->frontdir - trackDir) * (1 - math::pow(0.90f, deltaTime));
		trackDir.ANormalize();

		camera->pos = trackPos;

		camera->forward = u->pos + (u->speed * globalRendering->timeOffset) - camera->pos;
		camera->forward.ANormalize();
		camera->forward += trackDir;
		camera->forward.ANormalize();

		CFPSController& fpsCamera = static_cast<CFPSController&>(camHandler->GetCurrentController());
		fpsCamera.SetDir(camera->forward);
		fpsCamera.SetPos(trackPos);

		if (doRoll) {
			oldUp[gs->frameNum % 32] = u->updir;
			float3 up(ZeroVector);
			for (size_t a = 0; a < 32; ++a) {
				up += oldUp[a];
			}
			camera->up = up;
		} else {
			camera->up = UpVector;
		}

		oldCamDir = camera->forward;
		oldCamPos = camera->pos;
	}
}
