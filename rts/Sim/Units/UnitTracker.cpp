/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "UnitTracker.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Game/Camera/FPSController.h"
#include "Game/CameraHandler.h"
#include "Game/Camera.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "ConfigHandler.h"
#include "GlobalUnsynced.h"
#include "LogOutput.h"


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
	lastUpdateTime(0),
	trackPos(500,100,500),
	trackDir(0,0,1),
	oldCamDir(1,0,0),
	oldCamPos(500,500,500)
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


int CUnitTracker::GetMode()
{
	return trackMode;
}


void CUnitTracker::IncMode()
{
	trackMode = (trackMode + 1) % TrackModeCount;
	logOutput.Print("TrackMode: %s", modeNames[trackMode]);
}


void CUnitTracker::SetMode(int m)
{
	if (m < 0) {
		trackMode = 0;
	} else if (m >= TrackModeCount) {
		trackMode = TrackModeCount - 1;
	} else {
		trackMode = m;
	}
	logOutput.Print("TrackMode: %s", modeNames[trackMode]);
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
				logOutput.Print("TrackMode: %s", modeNames[TrackSingle]);
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
		if (uh->units[*it] != NULL) {
			it++;
			continue;
		}
		std::set<int>::iterator it_next = it;
		it_next++;
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
		it++;
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

	return uh->units[trackUnit];
}


float3 CUnitTracker::CalcAveragePos() const
{
	float3 p(0,0,0);
	std::set<int>::const_iterator it;
	for (it = trackGroup.begin(); it != trackGroup.end(); ++it) {
		p += uh->units[*it]->drawPos;
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
		const float3& p = uh->units[*it]->drawPos;
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
	if(firstUpdate){
		firstUpdate=false;
		doRoll=!configHandler->Get("ReflectiveWater",1);
	}

	CUnit* u = GetTrackUnit();
	if (u == NULL) {
		Disable();
		return;
	}

	if(lastFollowUnit!=0 && uh->units[lastFollowUnit]==0){
		timeOut=1;
		lastFollowUnit=0;
	}

	if(timeOut>0){
		timeOut++;
		camera->forward=oldCamDir;
		camera->pos=oldCamPos;
		if (camHandler->GetCurrentControllerNum() == 0)
		{
			CFPSController& fpsCamera = dynamic_cast<CFPSController&>(camHandler->GetCurrentController());
			fpsCamera.SetDir(oldCamDir);
			fpsCamera.SetPos(oldCamPos);
		}
		if(timeOut>15){
			timeOut=0;
		}
		return;
	}

	// non-FPS camera modes  (immediate positional tracking)
	if (camHandler->GetCurrentControllerNum() != 0) {
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
				pos = u->drawPos;
				break;
			}
		}
		camHandler->GetCurrentController().SetTrackingInfo(pos, u->radius * 2.7182818f);
		return;
	}

	float deltaTime = gs->frameNum + globalRendering->timeOffset - lastUpdateTime;
	lastUpdateTime = gs->frameNum + globalRendering->timeOffset;

	float3 modPlanePos = u->drawPos - (u->frontdir * u->radius * 3);
	float minHeight = ground->GetHeight2(modPlanePos.x,modPlanePos.z) + (u->radius * 2);
	if(modPlanePos.y < minHeight) {
  		modPlanePos.y = minHeight;
	}

	trackPos += (modPlanePos - trackPos) * (1 - pow(0.95f, deltaTime));
	trackDir += (u->frontdir - trackDir) * (1 - pow(0.90f, deltaTime));
	trackDir.ANormalize();

	camera->pos=trackPos;

	camera->forward = u->pos + (u->speed * globalRendering->timeOffset) - camera->pos;
	camera->forward.ANormalize();
	camera->forward += trackDir;
	camera->forward.ANormalize();
	if (camHandler->GetCurrentControllerNum() == 0)
	{
		CFPSController& fpsCamera = dynamic_cast<CFPSController&>(camHandler->GetCurrentController());
		fpsCamera.SetDir(camera->forward);
		fpsCamera.SetPos(trackPos);
	}
	
	if(doRoll){
		oldUp[gs->frameNum%32] = u->updir;
		float3 up(0,0,0);
		for(int a=0;a<32;++a){
			up+=oldUp[a];
		}
		camera->up=up;
	} else {
		camera->up=UpVector;
	}

	oldCamDir=camera->forward;
	oldCamPos=camera->pos;
}
