#ifndef __CAMERA_CONTROLLER_H__
#define __CAMERA_CONTROLLER_H__

#include <string>
#include <vector>

#include "GlobalStuff.h"

class CCameraController
{
public:
	CCameraController(int num);
	virtual ~CCameraController(void);

	virtual const std::string GetName() const = 0;

	virtual void KeyMove(float3 move)=0;
	virtual void MouseMove(float3 move)=0;
	virtual void ScreenEdgeMove(float3 move)=0;
	virtual void MouseWheelMove(float move)=0;

	virtual void Update() {}

	virtual float3 GetPos()=0;
	virtual float3 GetDir()=0;

	float GetFOV() { return fov; }

	virtual void SetPos(const float3& newPos) { pos = newPos; };
	virtual bool DisableTrackingByKey() { return true; }

	virtual float3 SwitchFrom() const =0;			//return pos that to send to new controllers SetPos
	virtual void SwitchTo(bool showText=true)=0;
	
	virtual void GetState(std::vector<float>& fv) const = 0;
	virtual bool SetState(const std::vector<float>& fv) = 0;
	virtual void SetTrackingInfo(const float3& pos, float radius) { SetPos(pos); }

//FIXME		virtual const std::vector<std::string>& GetStateNames() const = 0;
	const int num;
	bool enabled;
	
protected:
	float fov;
	float mouseScale;
	float scrollSpeed;
	
	float3 pos;
};


extern CCameraController* camCtrl;


#endif // __CAMERA_CONTROLLER_H__
