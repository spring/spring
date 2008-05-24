#ifndef __TW_CONTROLLER_H__
#define __TW_CONTROLLER_H__

#include "CameraController.h"


class CTWController : public CCameraController
{
public:
	CTWController();

	const std::string GetName() const { return "tw"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(std::vector<float>& fv) const;
	bool SetState(const std::vector<float>& fv, unsigned startPos=0);
};


#endif
