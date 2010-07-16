/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IVideoCapturing.h"


#if       defined AVI_CAPTURING
#include "AviVideoCapturing.h"
#else  // defined AVI_CAPTURING
#include "DummyVideoCapturing.h"
#endif // defined AVI_CAPTURING

//#include "System/LogOutput.h"
#include <cstdlib> // for NULL


IVideoCapturing* IVideoCapturing::instance;

IVideoCapturing* IVideoCapturing::GetInstance() {

	if (instance == NULL) {
#if       defined AVI_CAPTURING
		instance = new AviVideoCapturing();
#else  // defined AVI_CAPTURING
		instance = new DummyVideoCapturing();
#endif // defined AVI_CAPTURING
	}
	return instance;
}

void IVideoCapturing::FreeInstance() {

	delete instance;
	instance = NULL;
}


IVideoCapturing::IVideoCapturing() {
}

IVideoCapturing::~IVideoCapturing() {
}

