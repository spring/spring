/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FPS_CONTROLLER_H
#define _FPS_CONTROLLER_H

#include "CameraController.h"

class CFPSController : public CCameraController
{
public:
	CFPSController();

	const std::string GetName() const { return "fps"; }

	void KeyMove(float3 move);
	void MousePress(int x, int y, int button) { /* empty */ }
	void MouseRelease(int x, int y, int button) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(const float3& newPos);
	void SetDir(const float3& newDir);
	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	float mouseScale;
	float oldHeight;
	float3 dir;
};

#endif // _FPS_CONTROLLER_H
