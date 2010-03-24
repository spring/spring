/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __OV_CONTROLLER_H__
#define __OV_CONTROLLER_H__

#include "CameraController.h"

class COverviewController : public CCameraController
{
public:
	COverviewController();

	const std::string GetName() const { return "ov"; }

	void KeyMove(float3 move);
	void MousePress(int, int, int) { /* empty */ }
	void MouseRelease(int, int, int) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(const float3& newPos);
	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	bool minimizeMinimap;
};

#endif // __OV_CONTROLLER_H__
