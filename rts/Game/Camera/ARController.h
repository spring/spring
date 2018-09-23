/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AR_CONTROLLER_H
#define _AR_CONTROLLER_H

#include "CameraController.h"

class CARController : public CCameraController
{
public:
	CARController();

	const std::string GetName() const { return "ar"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void SetPos(const float3& newPos);

	float3 SwitchFrom() const;
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void Update();

private:
	float screenDivideRatio; 
	float mouseScale;
};

#endif // _ROTOH_CONTROLLER_H
