#include "OverviewController.h"

#include "Map/Ground.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "LogOutput.h"

COverviewController::COverviewController(int num)
	: CCameraController(num)
{
	enabled = false;
}

void COverviewController::KeyMove(float3 move)
{
}

void COverviewController::MouseMove(float3 move)
{
}

void COverviewController::ScreenEdgeMove(float3 move)
{
}

void COverviewController::MouseWheelMove(float move)
{
}

float3 COverviewController::GetPos()
{
	// map not created when constructor run
	pos.x = gs->mapx * 4.0f;
	pos.z = gs->mapy * 4.0f;
	const float aspect = (gu->viewSizeX / gu->viewSizeY);
	const float height = max(pos.x / aspect, pos.z);
	pos.y = ground->GetHeight(pos.x, pos.z) + (2.5f * height);
	return pos;
}

float3 COverviewController::GetDir()
{
	return float3(0.0f, -1.0f, -0.001f).Normalize();
}

void COverviewController::SetPos(const float3& newPos)
{
}

float3 COverviewController::SwitchFrom() const
{
	float3 dir=mouse->dir;
	float length=ground->LineGroundCol(pos,pos+dir*50000);
	float3 rpos=pos+dir*length;

	minimap->SetMinimized(minimizeMinimap);

	return rpos;
}

void COverviewController::SwitchTo(bool showText)
{
	if(showText)
		logOutput.Print("Switching to Overview style camera");

	minimizeMinimap = minimap->GetMinimized();
	minimap->SetMinimized(true);
}

void COverviewController::GetState(std::vector<float>& fv) const
{
	fv.push_back(/* 0 */ (float)num);
	fv.push_back(/* 1 */ pos.x);
	fv.push_back(/* 2 */ pos.y);
	fv.push_back(/* 3 */ pos.z);
}

bool COverviewController::SetState(const std::vector<float>& fv)
{
	if ((fv.size() != 4) || (fv[0] != (float)num)) {
		return false;
	}
	pos.x = fv[1];
	pos.y = fv[2];
	pos.z = fv[3];
	return true;
}
