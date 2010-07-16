/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DummyVideoCapturing.h"

#include "System/LogOutput.h"


DummyVideoCapturing::DummyVideoCapturing() {
}

DummyVideoCapturing::~DummyVideoCapturing() {
}


bool DummyVideoCapturing::IsCapturingSupported() const {
	return false;
}

bool DummyVideoCapturing::IsCapturing() const {
	return false;
}

void DummyVideoCapturing::StartCapturing() {

	logOutput.Print("Creating a video is not supported by this engine build.");
	logOutput.Print("(requires: OS=Win32 / Compiler=MinGW / \"#define AVI_CAPTURING\")");
	logOutput.Print("Please use an external frame-grabbing/-capturing application.");
}

void DummyVideoCapturing::StopCapturing() {
}

void DummyVideoCapturing::RenderFrame() {
}
