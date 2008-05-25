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
	virtual ~CCameraController(void);

	virtual const std::string GetName() const = 0;

	virtual void KeyMove(float3 move)=0;
	virtual void MouseMove(float3 move)=0;
	virtual void ScreenEdgeMove(float3 move)=0;
	virtual void MouseWheelMove(float move)=0;

	virtual void Update() {}

	virtual float3 GetPos()=0;
	virtual float3 GetDir()=0;

	float GetFOV() const { return fov; };

	virtual void SetPos(const float3& newPos) { pos = newPos; };
	virtual bool DisableTrackingByKey() { return true; }

	virtual float3 SwitchFrom() const =0;			//return pos that to send to new controllers SetPos
	virtual void SwitchTo(bool showText=true)=0;
	
	virtual void GetState(StateMap& sm) const = 0;
	virtual bool SetState(const StateMap& sm) = 0;
	virtual void SetTrackingInfo(const float3& pos, float radius) { SetPos(pos); }

	/// should this mode appear when we toggle the camera controller?
	bool enabled;
	
protected:
	bool SetStateBool(const StateMap& sm, const std::string& name, bool& var);
	bool SetStateFloat(const StateMap& sm, const std::string& name, float& var);

protected:
	float fov;
	float mouseScale;
	float scrollSpeed;
	
	float3 pos;
};


#endif // __CAMERA_CONTROLLER_H__
