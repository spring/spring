/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __SMOOTH_CONTROLLER_H__
#define __SMOOTH_CONTROLLER_H__

#include "CameraController.h"

/**
 * @brief Smooth Overview-Camera
 *
 * Behaves like the Overview(TA-style)-camera, but has an acceleration and breakrate to move smooth.
 */
class SmoothController : public CCameraController
{
public:
	SmoothController();

	const std::string GetName() const { return "sm"; }

	void KeyMove(float3 move);
	void MousePress(int, int, int) { /* empty */ }
	void MouseRelease(int, int, int) { /* empty */ }
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	void Update();
	float3 GetPos();
	float3 GetDir();

	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	bool flipped;

private:
	void Move(const float3& move, const unsigned timeDiff);
	
	float middleClickScrollSpeed;
	float zscale;
	float3 dir;
	float height;
	float oldAltHeight;
	bool changeAltHeight;
	float maxHeight;
	float tiltSpeed;
	enum MoveSource {Key, ScreenEdge, Noone};
	
	/// The source of the last move command by the user
	MoveSource lastSource;
	
	/**
	@brief the current speed factor
	
	If this is maxSpeedFactor, the camera has its full speed. Otherwise the camera moves with (speedFactor/maxSpeedFactor)
	*/
	unsigned speedFactor;
	
	/// the last move order > 0 given by the user
	float3 lastMove;
	
	/// the time in ms needed to accelerate to full speed
	static const unsigned maxSpeedFactor = 300;
};

#endif
