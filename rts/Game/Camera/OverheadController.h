/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OVERHEAD_CONTROLLER_H
#define _OVERHEAD_CONTROLLER_H

#include "CameraController.h"


class COverheadController : public CCameraController
{
public:
	COverheadController();
	~COverheadController();

	const std::string GetName() const override { return "ta"; }

	void KeyMove(float3 move) override;
	void MouseMove(float3 move) override;
	void ScreenEdgeMove(float3 move) override;
	void MouseWheelMove(float move) override;

	void Update() override;
	float3 GetPos() const override { return (pos - dir * height); }
	void SetPos(const float3& newPos) override;

	float3 SwitchFrom() const override { return pos; }
	void SwitchTo(const int oldCam, const bool showText) override;

	void GetState(StateMap& sm) const override;
	bool SetState(const StateMap& sm) override;

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

public:
	bool flipped;
private:
	bool changeAltHeight;

	float middleClickScrollSpeed;
	float height;
	float oldAltHeight;

	float maxHeight;
	float tiltSpeed;
	float angle;

	static constexpr float DEFAULT_ANGLE = 0.464f;
};

#endif // _OVERHEAD_CONTROLLER_H
