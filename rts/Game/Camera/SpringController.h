/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_CONTROLLER_H
#define _SPRING_CONTROLLER_H

#include "CameraController.h"
#include "System/Misc/SpringTime.h"
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
	float3 GetPos() const;
	void SetPos(const float3& newPos);
	float3 GetRot() const;

	float3 SwitchFrom() const;
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	float GetAzimuth() const;
	float MoveAzimuth(float move);

private:
	float3 rot;
	float dist;
	float maxDist;
	bool zoomBack;
	float oldDist;

	spring_time warpMouseStart;
	int2 warpMousePosOld;
	int2 warpMousePosNew;
};

#endif // _SPRING_CONTROLLER_H
