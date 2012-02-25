/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GML_BASE_H_
#define GML_BASE_H_

#include "gml.h"
#include "Game/GameController.h"

#ifdef USE_GML
namespace GML {
	void Init();
	void Exit();
	void PumpAux();
	bool SimThreadRunning();
	inline bool IsSimThread() { return gmlThreadNumber == GML_SIM_THREAD_NUM; }
	inline bool ShareLists() { return gmlShareLists; }
	inline bool MultiThreadSim() { return gmlMultiThreadSim; }
	inline void MultiThreadSim(bool mts) { gmlMultiThreadSim = mts; }
	inline int ThreadNumber() { return gmlThreadNumber; }
	inline void ThreadNumber(int num) { set_threadnum(num); }
	inline int ThreadCount() { return gmlThreadCount; }
	bool UpdateSim(CGameController *ac);
};
#else
namespace GML {
	inline void Init() {}
	inline void Exit() {}
	inline void PumpAux() {}
	inline bool SimThreadRunning() { return false; }
	inline bool IsSimThread() { return false; }
	inline bool ShareLists() { return false; }
	inline bool MultiThreadSim() { return false; }
	inline void MultiThreadSim(bool mts) {}
	inline int ThreadNumber() { return 0; }
	inline void ThreadNumber(int num) { }
	inline int ThreadCount() { return 1; }
	inline bool UpdateSim(CGameController *ac) { return ac->Update(); }
};
#endif

#endif // GML_BASE_H_
