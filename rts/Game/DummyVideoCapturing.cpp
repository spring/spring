/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DummyVideoCapturing.h"
#include "System/Log/ILog.h"

void DummyVideoCapturing::StartCapturing() {

	LOG_L(L_WARNING, "Creating a video is not supported by this engine build.");
	LOG_L(L_WARNING, "(requires: OS=Win32 / Compiler=MinGW / \"#define AVI_CAPTURING\")");
	LOG_L(L_WARNING, "Please use an external frame-grabbing/-capturing application.");
}

