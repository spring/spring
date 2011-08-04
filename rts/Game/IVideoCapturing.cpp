/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IVideoCapturing.h"


#if       defined AVI_CAPTURING
#include "AviVideoCapturing.h"
#else  // defined AVI_CAPTURING
#include "DummyVideoCapturing.h"
#endif // defined AVI_CAPTURING

#include <cstdlib> // for NULL

IVideoCapturing* IVideoCapturing::GetInstance()
{
#if       defined AVI_CAPTURING
	static AviVideoCapturing instance;
#else  // defined AVI_CAPTURING
	static DummyVideoCapturing instance;
#endif // defined AVI_CAPTURING

	return &instance;
}


void IVideoCapturing::FreeInstance()
{
	SetCapturing(false);
}


IVideoCapturing::IVideoCapturing()
{
}

IVideoCapturing::~IVideoCapturing()
{
	FreeInstance();
}

void IVideoCapturing::SetCapturing(bool enabled) {

	if (!GetInstance()->IsCapturing() && enabled) {
		GetInstance()->StartCapturing();
	} else if (GetInstance()->IsCapturing() && !enabled) {
		GetInstance()->StopCapturing();
	}
}
