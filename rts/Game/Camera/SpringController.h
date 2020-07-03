/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_CONTROLLER_H
#define _SPRING_CONTROLLER_H

#include "CameraController.h"
#include "System/type2.h"

class CSpringController : public CCameraController
{
public:
	CSpringController();

	const std::string GetName() const override { return "spring"; }

	void KeyMove(float3 move) override;
	void MouseMove(float3 move) override;
	void ScreenEdgeMove(float3 move) override;
	void MouseWheelMove(float move) override;

	void Update() override;
	void SetPos(const float3& newPos) override { pos = newPos; Update(); }
	float3 GetPos() const override;
	float3 GetRot() const override { return (float3(rot.x, GetAzimuth(), 0.0f)); }

	float3 SwitchFrom() const override { return pos; }
	void SwitchTo(const int oldCam, const bool showText) override;

	void GetState(StateMap& sm) const override;
	bool SetState(const StateMap& sm) override;

private:
	float GetAzimuth() const;
	float MoveAzimuth(float move);

	inline float ZoomIn(const float3& curCamPos, const float2& zoomParams);
	inline float ZoomOut(const float3& curCamPos, const float2& zoomParams);

private:
	float3 rot;

	float curDist; // current zoom-out distance
	float maxDist; // maximum zoom-out distance
	float oldDist;

	bool zoomBack;
	bool cursorZoomIn;
	bool cursorZoomOut;
};

#endif // _SPRING_CONTROLLER_H
