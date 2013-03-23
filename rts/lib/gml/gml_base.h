/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GML_BASE_H_
#define GML_BASE_H_

#include "gmlcls.h"

#ifdef USE_GML
#include <SDL_timer.h>
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
	inline void SetLuaUIState(lua_State *L) { gmlLuaUIState = L; }
	inline void SetCheckCallChain(bool cc) { gmlCheckCallChain = cc; }
	inline void EnableCallChainWarnings(bool cw) { gmlCallChainWarning = (cw ? 0 : GML_MAX_CALL_CHAIN_WARNINGS); }
	inline unsigned int UpdateTicks() { gmlNextTickUpdate = 100; return gmlCurrentTicks = SDL_GetTicks(); }
	inline void GetTicks(unsigned int &var) { var = (--gmlNextTickUpdate > 0) ? gmlCurrentTicks : UpdateTicks(); }
	inline bool ServerActive() { return gmlServerActive; }
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
	inline void SetLuaUIState(void *L) {}
	inline void SetCheckCallChain(bool cc) {}
	inline void EnableCallChainWarnings(bool cw) {}
	inline unsigned int UpdateTicks() { return 0; }
	inline void GetTicks(unsigned int &var) {}
	inline bool ServerActive() { return false; }
};
#endif

#endif // GML_BASE_H_
