/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FPS_CONTROLLER_H
#define _FPS_CONTROLLER_H

#include "CameraController.h"

class CFPSController : public CCameraController
{
public:
	CFPSController();
	~CFPSController();

	const std::string GetName() const override { return "fps"; }

	void KeyMove(float3 move) override;
	void MouseMove(float3 move) override;
	void ScreenEdgeMove(float3 move) override { KeyMove(move); }
	void MouseWheelMove(float move) override;

	void SetPos(const float3& newPos) override;
	void SetDir(const float3& newDir) override;
	float3 SwitchFrom() const override { return pos; }
	void SwitchTo(const int oldCam, const bool showText) override;

	void GetState(StateMap& sm) const override;
	bool SetState(const StateMap& sm) override;


	void Update() override;

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

private:
	float mouseScale;
	float oldHeight;
	bool clampPos;
};

#endif // _FPS_CONTROLLER_H
