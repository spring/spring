/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IVideoCapturing.h"

#if       defined AVI_CAPTURING
#include "AviVideoCapturing.h"
#else  // defined AVI_CAPTURING
#include "DummyVideoCapturing.h"
#endif // defined AVI_CAPTURING

#include "Rendering/GlobalRendering.h"


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


bool IVideoCapturing::SetCapturing(bool enable)
{
	const bool isCapturing = GetInstance()->IsCapturing();

	if (!isCapturing && enable) {
		GetInstance()->StartCapturing();
		return true;
	}
	if (isCapturing && !enable) {
		GetInstance()->StopCapturing();
		return true;
	}

	return false;
}

