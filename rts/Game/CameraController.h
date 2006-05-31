#ifndef __CAMERA_CONTROLLER_H__
#define __CAMERA_CONTROLLER_H__

class CCameraController
{
public:
	CCameraController(void);
	virtual ~CCameraController(void);

	virtual void KeyMove(float3 move)=0;
	virtual void MouseMove(float3 move)=0;
	virtual void ScreenEdgeMove(float3 move)=0;
	virtual void MouseWheelMove(float move)=0;

	virtual float3 GetPos()=0;
	virtual float3 GetDir()=0;

	virtual void SetPos(float3 newPos)=0;
	virtual float3 SwitchFrom()=0;			//return pos that to send to new controllers SetPos
	virtual void SwitchTo()=0;
	
	float mouseScale;
	float scrollSpeed;
	bool enabled;
};

class CFPSController : public CCameraController
{
public:
	CFPSController();

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(float3 newPos);
	float3 SwitchFrom();
	void SwitchTo();

	float3 pos;
	float oldHeight;

	float3 dir;
};

class COverheadController : public CCameraController
{
public:
	COverheadController();

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(float3 newPos);
	float3 SwitchFrom();
	void SwitchTo();

	float zscale;
	float3 pos;
	float3 dir;
	float height;
	float oldAltHeight;
};

class CTWController : public CCameraController
{
public:
	CTWController();

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(float3 newPos);
	float3 SwitchFrom();
	void SwitchTo();

	float3 pos;
};

class CRotOverheadController : public CCameraController
{
public:
	CRotOverheadController();

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(float3 newPos);
	float3 SwitchFrom();
	void SwitchTo();

	float3 pos;
	float oldHeight;

	float3 dir;
};

#endif // __CAMERA_CONTROLLER_H__
