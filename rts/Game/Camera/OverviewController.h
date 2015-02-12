/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OV_CONTROLLER_H
#define _OV_CONTROLLER_H

#include "CameraController.h"

class COverviewController : public CCameraController
{
public:
	COverviewController();

	const std::string GetName() const { return "ov"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void SetPos(const float3& newPos);
	void SetDir(const float3& newDir);
	float3 SwitchFrom() const;
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	bool minimizeMinimap;
};

#endif // _OV_CONTROLLER_H
