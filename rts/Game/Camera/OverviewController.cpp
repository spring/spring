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

	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + (2.5f * std::max(pos.x / globalRendering->aspectRatio, pos.z));
	dir = float3(0.0f, -1.0f, -0.001f).ANormalize();
}


float3 COverviewController::SwitchFrom() const
{
	const float3 mdir = mouse->dir;
	const float3 rpos = pos + mdir * CGround::LineGroundCol(pos, pos + mdir * 50000.0f, false);

	if (!globalRendering->dualScreenMode)
		minimap->SetMinimized(minimizeMinimap);

	return rpos;
}

void COverviewController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText)
		LOG("Switching to Overview style camera");

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
