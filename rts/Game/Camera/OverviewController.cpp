/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "OverviewController.h"

#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"

COverviewController::COverviewController()
{
	enabled = false;
	minimizeMinimap = false;

	pos.x = mapDims.mapx * 0.5f * SQUARE_SIZE;
	pos.z = mapDims.mapy * 0.5f * SQUARE_SIZE;
	const float height = std::max(pos.x / globalRendering->aspectRatio, pos.z);
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + (2.5f * height);

	dir = float3(0.0f, -1.0f, -0.001f).ANormalize();
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

void COverviewController::SetDir(const float3& newDir)
{
}

void COverviewController::SetPos(const float3& newPos)
{
}

float3 COverviewController::SwitchFrom() const
{
	float3 dir = mouse->dir;
	float length = CGround::LineGroundCol(pos, pos + dir * 50000, false);
	float3 rpos = pos + dir * length;

	if (!globalRendering->dualScreenMode) {
		minimap->SetMinimized(minimizeMinimap);
	}

	return rpos;
}

void COverviewController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText) {
		LOG("Switching to Overview style camera");
	}

	if (!globalRendering->dualScreenMode) {
		minimizeMinimap = minimap->GetMinimized();
		minimap->SetMinimized(true);
	}
}

void COverviewController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);
}

bool COverviewController::SetState(const StateMap& sm)
{
	CCameraController::SetState(sm);
	return true;
}
