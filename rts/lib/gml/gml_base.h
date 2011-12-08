/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GML_BASE_H_
#define GML_BASE_H_

#include "gml.h"

#ifdef USE_GML
namespace GML {
	void Init();
	void Exit();
	void PumpAux();
	bool SimThreadRunning();
	inline bool IsSimThread() { return gmlThreadNumber == GML_SIM_THREAD_NUM; }
	inline bool ShareLists() { return gmlShareLists; }
	inline bool SimEnabled() {	return GML_ENABLE_SIM ? true : false; }
};
#else
namespace GML {
	inline void Init() {}
	inline void Exit() {}
	inline void PumpAux() {}
	inline bool SimThreadRunning() { return false; }
	inline bool IsSimThread() { return false; }
	inline bool ShareLists() { return false; }
	inline bool SimEnabled() {	return false; }
};
#endif

#endif // GML_BASE_H_
