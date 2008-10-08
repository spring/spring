#ifndef __LUA_CONTROLLER_H__
#define __LUA_CONTROLLER_H__


#include "CameraController.h"


class CLuaCameraCtrl : public CCameraController {
public:
	CLuaCameraCtrl();

	const std::string GetName() const { return "lua"; }

	void KeyMove(float3 move);
	void MousePress(int, int, int) { /* empty */ }
	void MouseRelease(int, int, int) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void Update();

	float3 GetPos();
	float3 GetDir();

	void SetPos(const float3& newPos);
	void SetTrackingInfo(const float3& pos, float radius);

	bool DisableTrackingByKey();

	float3 SwitchFrom() const; // return pos that to send to new controllers SetPos
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	float3 dir;
};

#endif
