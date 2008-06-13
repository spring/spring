// AirScript.h: Air scripting support
///////////////////////////////////////////////////////////////////////////

#ifndef __AIR_SCRIPT_H__
#define __AIR_SCRIPT_H__

#include <deque>
#include "Script.h"

class CAirScript :
	public CScript
{
public:
	CAirScript(void);
	~CAirScript(void);
	void GameStart();
	void Update(void);

	std::deque<int> planes;
	int curPlane;
	float lastUpdateTime;
	float3 tcp;
	float3 tcf;
	float3 oldUp[32];
	
	float3 oldCamDir;
	float3 oldCamPos;
	bool doRoll;
	int timeOut;
	void SetCamera(void);
};

#endif // __AIR_SCRIPT_H__
