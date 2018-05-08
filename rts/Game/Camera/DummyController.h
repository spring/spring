/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DUMMY_CONTROLLER_H
#define _DUMMY_CONTROLLER_H

#include "CameraController.h"

class CDummyController: public CCameraController {
public:
	CDummyController(): CCameraController() { enabled = false; }

	const std::string GetName() const override { return "dummy"; }

	void KeyMove(float3 move) override {}
	void MouseMove(float3 move) override {}
	void ScreenEdgeMove(float3 move) override {}
	void MouseWheelMove(float move) override {}

	float3 SwitchFrom() const override { return ZeroVector; }
	void SwitchTo(const int, const bool) override {}
};

#endif

