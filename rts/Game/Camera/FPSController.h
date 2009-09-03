#ifndef __FPS_CONTROLLER_H__
#define __FPS_CONTROLLER_H__

#include <boost/signals/connection.hpp>

#include "CameraController.h"

union SDL_Event;

class CFPSController : public CCameraController
{
public:
	CFPSController();

	const std::string GetName() const { return "fps"; }

	void KeyMove(float3 move);
	void MousePress(int, int, int) { /* empty */ }
	void MouseRelease(int, int, int) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	virtual void Update();

	void SetPos(const float3& newPos);
	void SetDir(const float3& newDir);
	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void JoyAxis(int axis, int state);

private:
	boost::signals::scoped_connection inputCon;
	bool HandleEvent(const SDL_Event& ev);
	float oldHeight;
	float3 dir;
	int moveSpeed;
	int roll;
	int pitch;
};


#endif
