/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_TRACKER_H
#define UNIT_TRACKER_H

#include <set>
#include "float3.h"

class CUnit;

class CUnitTracker
{
	public:
		CUnitTracker();
		~CUnitTracker();

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
		bool enabled;
		bool doRoll;
		bool firstUpdate;

		int trackMode;
		int trackUnit;
		std::set<int> trackGroup;
		
		int timeOut;
		int lastFollowUnit;
		float lastUpdateTime;

		float3 trackPos;
		float3 trackDir;

		float3 oldUp[32];
		float3 oldCamDir;
		float3 oldCamPos;
		
		static const char* modeNames[TrackModeCount];
};

extern CUnitTracker unitTracker;

#endif /* UNIT_TRACKER_H */
