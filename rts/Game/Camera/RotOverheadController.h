/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROTOH_CONTROLLER_H
#define _ROTOH_CONTROLLER_H

#include "CameraController.h"

class CRotOverheadController : public CCameraController
{
public:
	CRotOverheadController();

	const std::string GetName() const override { return "rot"; }

	void KeyMove(float3 move) override;
	void MouseMove(float3 move) override;
	void ScreenEdgeMove(float3 move) override;
	void MouseWheelMove(float move) override;

	void SetPos(const float3& newPos) override;

	float3 SwitchFrom() const override;
	void SwitchTo(const int oldCam, const bool showText) override;

	void GetState(StateMap& sm) const override;
	bool SetState(const StateMap& sm) override;

	void Update() override;

private:
	float mouseScale;
	float oldHeight;
};

#endif // _ROTOH_CONTROLLER_H
