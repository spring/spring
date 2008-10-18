#ifndef __ORBIT_CONTROLLER_H__
#define __ORBIT_CONTROLLER_H__

#include "CameraController.h"

class COrbitController: public CCameraController {
	public:
		COrbitController();

		void Init(const float3& p, const float3& tar = ZeroVector);
		void Update();

		const std::string GetName() const { return "OrbitController"; }

		void KeyMove(float3 move);
		void MousePress(int, int, int);
		void MouseRelease(int, int, int);
		void MouseMove(float3 move);
		void ScreenEdgeMove(float3 move);
		void MouseWheelMove(float move);

		float3 GetPos();
		float3 GetDir();

		void SetPos(const float3& newPos);
		float3 SwitchFrom() const;
		void SwitchTo(bool showText);

		void GetState(StateMap& sm) const;
		bool SetState(const StateMap& sm);

	private:
		void MyMouseMove(int, int, int, int, int);
		void Pan(int, int);
		float3 GetOrbitPos() const;

		int lastMouseMoveX;
		int lastMouseMoveY;
		int lastMousePressX; // x-coor of last button press
		int lastMousePressY; // y-coor of last button-press
		int lastMouseButton; // button that was last pressed
		int currentState;
		float distance, cDistance;
		float rotation, cRotation;
		float elevation, cElevation;
		float3 cen;

		enum States {None, Orbiting, Zooming, Panning};
};

#endif
