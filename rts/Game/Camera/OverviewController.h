#ifndef __OV_CONTROLLER_H__
#define __OV_CONTROLLER_H__


#include "CameraController.h"


class COverviewController : public CCameraController
{
public:
	COverviewController();

	const std::string GetName() const { return "ov"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move);

	float3 GetPos();
	float3 GetDir();

	void SetPos(const float3& newPos);
	float3 SwitchFrom() const;
	void SwitchTo(bool showText);

	void GetState(std::vector<float>& fv) const;
	bool SetState(const std::vector<float>& fv, unsigned startPos=0);

private:
	bool minimizeMinimap;
};

#endif
