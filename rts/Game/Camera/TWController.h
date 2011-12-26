/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TW_CONTROLLER_H
#define _TW_CONTROLLER_H

#include "CameraController.h"

class CTWController : public CCameraController
{
public:
	CTWController();

	const std::string GetName() const { return "tw"; }

	void KeyMove(float3 move);
	void MousePress(int x, int y, int button) { /* empty */ }
	void MouseRelease(int x, int y, int button) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void Update();
	float3 GetPos();
	float3 GetDir();

	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);
};

#endif // _TW_CONTROLLER_H
