#ifndef __OVERHEAD_CONTROLLER_H__
#define __OVERHEAD_CONTROLLER_H__

#include "CameraController.h"


class COverheadController : public CCameraController
{
public:
	COverheadController();

	const std::string GetName() const { return "ta"; }

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

	bool flipped;

private:
	float zscale;
	float3 dir;
	float height;
	float oldAltHeight;
	bool changeAltHeight;
	float maxHeight;
	float tiltSpeed;
};

#endif
