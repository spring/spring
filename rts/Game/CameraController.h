#ifndef __CAMERA_CONTROLLER_H__
#define __CAMERA_CONTROLLER_H__

#include <vector>

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
	virtual void SwitchTo(bool showText=true)=0;
	
	virtual std::vector<float> GetState() const = 0;
	virtual bool SetState(const std::vector<float>& fv) = 0;
	
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
	void SwitchTo(bool showText);

	std::vector<float> GetState() const;
	bool SetState(const std::vector<float>& fv);

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
	void SwitchTo(bool showText);

	std::vector<float> GetState() const;
	bool SetState(const std::vector<float>& fv);

	float zscale;
	float3 pos;
	float3 dir;
	float height;
	float oldAltHeight;
	bool changeAltHeight;
	float maxHeight;
	float tiltSpeed;
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
	void SwitchTo(bool showText);

	std::vector<float> GetState() const;
	bool SetState(const std::vector<float>& fv);

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
	void SwitchTo(bool showText);

	std::vector<float> GetState() const;
	bool SetState(const std::vector<float>& fv);

	float3 pos;
	float oldHeight;

	float3 dir;
};

class COverviewController : public CCameraController
{
public:
	COverviewController();

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(float3 newPos);
	float3 SwitchFrom();
	void SwitchTo(bool showText);

	std::vector<float> GetState() const;
	bool SetState(const std::vector<float>& fv);

	float3 pos;
	bool minimizeMinimap;
};

#endif // __CAMERA_CONTROLLER_H__
