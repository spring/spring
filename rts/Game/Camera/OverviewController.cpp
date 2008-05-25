#include "OverviewController.h"

#include "Map/Ground.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "LogOutput.h"

COverviewController::COverviewController()
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
	const float height = std::max(pos.x / aspect, pos.z);
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

	if (!gu->dualScreenMode) {
		minimap->SetMinimized(minimizeMinimap);
	}

	return rpos;
}

void COverviewController::SwitchTo(bool showText)
{
	if (showText) {
		logOutput.Print("Switching to Overview style camera");
	}

	if (!gu->dualScreenMode) {
		minimizeMinimap = minimap->GetMinimized();
		minimap->SetMinimized(true);
	}
}

void COverviewController::GetState(StateMap& sm) const
{
	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;
}

bool COverviewController::SetState(const StateMap& sm)
{
	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);
	return true;
}
