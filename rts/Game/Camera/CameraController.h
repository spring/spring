/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_CONTROLLER_H
#define _CAMERA_CONTROLLER_H

#include <string>
#include <vector>
#include <map>

#include "Game/Camera.h"
#include "System/float3.h"


class CCameraController
{
public:
	typedef std::map<std::string, float> StateMap;

public:
	CCameraController();
	virtual ~CCameraController() {}

	virtual const std::string GetName() const = 0;

	virtual void KeyMove(float3 move) = 0;
	virtual void MouseMove(float3 move) = 0;
	virtual void ScreenEdgeMove(float3 move) = 0;
	virtual void MouseWheelMove(float move) = 0;

	virtual void Update() {}

	float GetFOV() const { return fov; } //< In degrees!

	virtual float3 GetPos() const { return pos; }
	virtual float3 GetDir() const { return dir; }
	virtual float3 GetRot() const { return CCamera::GetRotFromDir(GetDir()); }

	virtual void SetPos(const float3& newPos) { pos = newPos; }
	virtual void SetDir(const float3& newDir) { dir = newDir; }
	virtual bool DisableTrackingByKey() { return true; }

	// return the position to send to new controllers SetPos
	virtual float3 SwitchFrom() const = 0;
	virtual void SwitchTo(const int oldCam, const bool showText = true) = 0;

	virtual void GetState(StateMap& sm) const;
	virtual bool SetState(const StateMap& sm);
	virtual void SetTrackingInfo(const float3& pos, float radius) { SetPos(pos); }

	/**
	 * Whether the camera's distance to the ground or to the units
	 * should be used to determine whether to show them as icons or normal.
	 * This is called every frame, so it can be dynamically,
	 * eg depend on the current position/angle.
	 */
	virtual bool GetUseDistToGroundForIcons();

	/// should this mode appear when we toggle the camera controller?
	bool enabled;

protected:
	bool SetStateBool(const StateMap& sm, const std::string& name, bool& var);
	bool SetStateFloat(const StateMap& sm, const std::string& name, float& var);


	float fov;
	float3 pos;
	float3 dir;

	/**
	* @brief scrollSpeed
	* scales the scroll speed in general
	* (includes middleclick, arrowkey, screenedge scrolling)
	*/
	float scrollSpeed;

	/**
	 * @brief switchVal
	 * Where to switch from Camera-Unit-distance to Camera-Ground-distance
	 * for deciding whether to draw 3D view or icon of units.
	 * * 1.0 = 0 degree  = overview
	 * * 0.0 = 90 degree = first person
	 */
	float switchVal;

	float pixelSize;
};


#endif // _CAMERA_CONTROLLER_H
