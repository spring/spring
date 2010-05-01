/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ROTOH_CONTROLLER_H__
#define __ROTOH_CONTROLLER_H__

#include "CameraController.h"

class CRotOverheadController : public CCameraController
{
public:
	CRotOverheadController();

	const std::string GetName() const { return "rot"; }

	void KeyMove(float3 move);
	void MousePress(int, int, int) { /* empty */ }
	void MouseRelease(int, int, int) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void Update();
	float3 GetPos();
	float3 GetDir();

	void SetPos(const float3& newPos);
	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	float mouseScale;
	float oldHeight;
	float3 dir;
};

#endif
