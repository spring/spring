/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROTOH_CONTROLLER_H
#define _ROTOH_CONTROLLER_H

#include "CameraController.h"

class CRotOverheadController : public CCameraController
{
public:
	CRotOverheadController();

	const std::string GetName() const { return "rot"; }

	void KeyMove(float3 move);
	void MousePress(int x, int y, int button) { /* empty */ }
	void MouseRelease(int x, int y, int button) { /* empty */ }
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

#endif // _ROTOH_CONTROLLER_H
