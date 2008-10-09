#ifndef __TW_CONTROLLER_H__
#define __TW_CONTROLLER_H__

#include "CameraController.h"


class CTWController : public CCameraController
{
public:
	CTWController();

	const std::string GetName() const { return "tw"; }

	void KeyMove(float3 move);
	void MousePress(int, int, int) { /* empty */ }
	void MouseRelease(int, int, int) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);
};


#endif
