// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_GLOBAL_H
#define RAI_GLOBAL_H

class cRAIGlobal;

#include "Krogs Metal Class v0.4/MetalMap.h"
#include "GTerrainMap.h"
//#include "GMetalMap.h"
//#include "ExternalAI/IAICallback.h"
#include <set>

class cRAIGlobal
{
public:
	cRAIGlobal(IAICallback* callback);
	~cRAIGlobal();

	cTerrainMap *TM;
	CMetalMap* KMM;
//	cRMetalMap* RMM;
	set<int> GeoSpot;
	int AIs;

private:
	void ClearLogFiles(IAICallback* callback);
	cLogFile *l;
};

#endif
