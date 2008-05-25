#ifndef __FPS_CONTROLLER_H__
#define __FPS_CONTROLLER_H__


#include "CameraController.h"


class CFPSController : public CCameraController
{
public:
	CFPSController();

	const std::string GetName() const { return "fps"; }

	void KeyMove(float3 move);
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
	float oldHeight;
	float3 dir;
};


#endif
