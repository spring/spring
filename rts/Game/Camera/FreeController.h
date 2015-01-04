/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FREE_CONTROLLER_H
#define _FREE_CONTROLLER_H

#include "CameraController.h"

class CFreeController : public CCameraController {
public:
	CFreeController();

	const std::string GetName() const { return "free"; }

	void Move(const float3& move, bool tilt, bool strafe, bool upDown);

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	bool DisableTrackingByKey() { return false; }

	void Update();
	float3 GetDir() const;
	
	void SetPos(const float3& newPos);
	void SetTrackingInfo(const float3& pos, float radius);
	float3 SwitchFrom() const;
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	float3 vel;      // velocity
	float3 avel;     // angular velocity
	float3 prevVel;  // previous velocity
	float3 prevAvel; // previous angular velocity

	float3 rot;
	float3 trackPos;
	float trackRadius;

	float tiltSpeed; // time it takes to max
	float velTime;   // time it takes to max
	float avelTime;  // time it takes to max

	float gndOffset; // 0:   disabled
	                 // <0:  locked to -gndOffset
	                 // >0:  allow ground locking and gravity
	float gravity;   // >=0: disabled
	float autoTilt;  // <=0: disabled
	float slide;     // <=0; disabled

	bool tracking;
	bool gndLock;
	bool invertAlt;
	bool goForward;
};

#endif // _FREE_CONTROLLER_H
