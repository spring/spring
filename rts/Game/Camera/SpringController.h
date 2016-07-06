/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_CONTROLLER_H
#define _SPRING_CONTROLLER_H

#include "CameraController.h"
#include "System/type2.h"

class CSpringController : public CCameraController
{
public:
	CSpringController();

	const std::string GetName() const { return "spring"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void Update();
	void SetPos(const float3& newPos) { pos = newPos; Update(); }
	float3 GetPos() const;
	float3 GetRot() const { return (float3(rot.x, GetAzimuth(), 0.0f)); }

	float3 SwitchFrom() const { return pos; }
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

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
