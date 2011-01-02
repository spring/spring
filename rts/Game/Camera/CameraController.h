/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __CAMERA_CONTROLLER_H__
#define __CAMERA_CONTROLLER_H__

#include <string>
#include <vector>
#include <map>

#include "float3.h"

class CCameraController
{
public:
	typedef std::map<std::string, float> StateMap;

public:
	CCameraController();
	virtual ~CCameraController(void) {}

	virtual const std::string GetName() const = 0;

	virtual void KeyMove(float3 move) = 0;
	virtual void MousePress(int x, int y, int button) = 0;
	virtual void MouseRelease(int x, int y, int button) = 0;
	virtual void MouseMove(float3 move) = 0;
	virtual void ScreenEdgeMove(float3 move) = 0;
	virtual void MouseWheelMove(float move) = 0;

	virtual void Update() {}

	virtual float3 GetPos() = 0;
	virtual float3 GetDir() = 0;

	/// In degree!
	float GetFOV() const { return fov; };

	virtual void SetPos(const float3& newPos) { pos = newPos; };
	virtual bool DisableTrackingByKey() { return true; }

	// return the position to send to new controllers SetPos
	virtual float3 SwitchFrom() const = 0;
	virtual void SwitchTo(bool showText = true) = 0;

	virtual void GetState(StateMap& sm) const = 0;
	virtual bool SetState(const StateMap& sm) = 0;
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


#endif // __CAMERA_CONTROLLER_H__
