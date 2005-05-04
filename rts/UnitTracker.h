#pragma once

class CUnitTracker
{
public:
	CUnitTracker(void);
	~CUnitTracker(void);
	void SetCam(void);

	double lastUpdateTime;

	int lastFollowUnit;
	float3 tcp;
	float3 tcf;
	float3 oldUp[32];
	
	float3 oldCamDir;
	float3 oldCamPos;
	bool doRoll;
	bool firstUpdate;
	int timeOut;

};

extern CUnitTracker unitTracker;