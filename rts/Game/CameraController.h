#ifndef __CAMERA_CONTROLLER_H__
#define __CAMERA_CONTROLLER_H__

#include <string>
#include <vector>

class CCameraController {
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

		virtual float GetFOV() { return fov; }

		virtual void SetPos(float3 newPos)=0;
		virtual bool DisableTrackingByKey() { return true; }

		virtual float3 SwitchFrom()=0;			//return pos that to send to new controllers SetPos
		virtual void SwitchTo(bool showText=true)=0;
		
		virtual void GetState(std::vector<float>& fv) const = 0;
		virtual bool SetState(const std::vector<float>& fv) = 0;
		virtual void SetTrackingInfo(const float3& pos, float radius) { SetPos(pos); }

//FIXME		virtual const std::vector<std::string>& GetStateNames() const = 0;
		
		const int num;
		float fov;
		float mouseScale;
		float scrollSpeed;
		bool enabled;
};


class CFPSController : public CCameraController {
	public:
		CFPSController(int num);

		const std::string GetName() const { return "fps"; }

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		float3 SwitchFrom();
		void SwitchTo(bool showText);

		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

		float3 pos;
		float oldHeight;

		float3 dir;
};


class COverheadController : public CCameraController {
	public:
		COverheadController(int num);

		const std::string GetName() const { return "ta"; }

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		float3 SwitchFrom();
		void SwitchTo(bool showText);

		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

		float zscale;
		float3 pos;
		float3 dir;
		float height;
		float oldAltHeight;
		bool changeAltHeight;
		float maxHeight;
		float tiltSpeed;
		bool flipped;
};


class CTWController : public CCameraController {
	public:
		CTWController(int num);

		const std::string GetName() const { return "tw"; }

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		float3 SwitchFrom();
		void SwitchTo(bool showText);

		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

		float3 pos;
};


class CRotOverheadController : public CCameraController {
	public:
		CRotOverheadController(int num);

		const std::string GetName() const { return "rot"; }

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		float3 SwitchFrom();
		void SwitchTo(bool showText);

		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

		float3 pos;
		float oldHeight;

		float3 dir;
};


class COverviewController : public CCameraController {
	public:
		COverviewController(int num);

		const std::string GetName() const { return "ov"; }

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		float3 SwitchFrom();
		void SwitchTo(bool showText);

		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

		float3 pos;
		bool minimizeMinimap;
};


class CFreeController : public CCameraController {
	public:
		CFreeController(int num);

		const std::string GetName() const { return "free"; }

		void Move(const float3& move, bool tilt, bool strafe, bool upDown);

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		bool DisableTrackingByKey() { return false; }

		void Update();

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		void SetTrackingInfo(const float3& pos, float radius);
		float3 SwitchFrom();
		void SwitchTo(bool showText);

		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

		float3 pos;
		float3 dir;
		float3 vel;      // velocity
		float3 avel;     // angular velocity
		float3 prevVel;  // previous velocity
		float3 prevAvel; // previous angular velocity

		bool tracking;
		float3 trackPos;
		float trackRadius;
		bool gndLock;

		float tiltSpeed; // time it takes to max
		float velTime;   // time it takes to max
		float avelTime;  // time it takes to max

		float gndOffset; // 0:   disabled
		                 // <0:  locked to -gndOffset
		                 // >0:  allow ground locking and gravity
		float gravity;   // >=0: disabled
		float autoTilt;  // <=0: disabled
		float slide;     // <=0; disabled

		bool invertAlt;
		bool goForward;
};


class CLuaCameraCtrl : public CCameraController {
	public:
		CLuaCameraCtrl(int num);

		const std::string GetName() const { return "lua"; }

		void KeyMove(float3 move);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		void Update();

		float3 GetPos();
		float3 GetDir();

		void SetPos(float3 newPos);
		void SetTrackingInfo(const float3& pos, float radius);

		bool DisableTrackingByKey();

		float3 SwitchFrom(); // return pos that to send to new controllers SetPos
		void SwitchTo(bool showText);
		
		void GetState(std::vector<float>& fv) const;
		bool SetState(const std::vector<float>& fv);

	private:
		float3 pos;
		float3 dir;
};


#endif // __CAMERA_CONTROLLER_H__
