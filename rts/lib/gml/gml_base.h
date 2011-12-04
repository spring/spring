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
};
#else
namespace GML {
	void Init() {}
	void Exit() {}
	void PumpAux() {}
	bool SimThreadRunning() { return false; }
};
#endif

#endif // GML_BASE_H_
