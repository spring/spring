#ifndef UNITTRACKER_H
#define UNITTRACKER_H

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

		int  GetMode();
		void IncMode();
		void SetMode(int);
		enum TrackMode {
			TrackSingle = 0,
			TrackAverage,
			TrackExtents,
			TrackModeCount
		};

	protected:
		void NextUnit(void);
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

#endif /* UNITTRACKER_H */
