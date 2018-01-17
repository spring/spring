/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_TRACKER_H
#define UNIT_TRACKER_H

#include "System/float3.h"
#include "System/UnorderedSet.hpp"

class CUnit;

class CUnitTracker
{
public:
	void Track();
	void Disable();
	bool Enabled() const { return enabled; }

	void SetCam();

	int  GetMode() const;
	void IncMode();
	/// @param trackMode see enum CUnitTracker::TrackMode
	void SetMode(int trackMode);
	enum TrackMode {
		TrackSingle = 0,
		TrackAverage,
		TrackExtents,
		TrackModeCount
	};

protected:
	void NextUnit();
	void MakeTrackGroup();
	void CleanTrackGroup();
	CUnit* GetTrackUnit();
	float3 CalcAveragePos() const;
	float3 CalcExtentsPos() const;

protected:
	bool enabled = false;

	int trackMode = TrackSingle;
	int trackUnit = 0;

	int timeOut = 15;
	int lastFollowUnit = 0;
	float lastUpdateTime = 0.0f;

	float3 trackPos = {500.0f, 100.0f, 500.0f};
	float3 trackDir = FwdVector;

	float3 smoothedRight = RgtVector;
	float3 oldCamDir = RgtVector;
	float3 oldCamPos = {500.0f, 500.0f, 500.0f};

	spring::unordered_set<int> trackedUnitIDs;
	std::vector<int> deadUnitIDs;
	
	static const char* modeNames[TrackModeCount];
};

extern CUnitTracker unitTracker;

#endif /* UNIT_TRACKER_H */
