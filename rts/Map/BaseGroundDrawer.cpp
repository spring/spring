/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BaseGroundDrawer.h"
#include "System/Config/ConfigHandler.h"


CONFIG(float, GroundLODScaleReflection).defaultValue(1.0f).headlessValue(0.0f);
CONFIG(float, GroundLODScaleRefraction).defaultValue(1.0f).headlessValue(0.0f);
CONFIG(float, GroundLODScaleTerrainReflection).defaultValue(1.0f);


CBaseGroundDrawer::CBaseGroundDrawer()
{
	LODScaleReflection = configHandler->GetFloat("GroundLODScaleReflection");
	LODScaleRefraction = configHandler->GetFloat("GroundLODScaleRefraction");
	LODScaleTerrainReflection = configHandler->GetFloat("GroundLODScaleTerrainReflection");

	jamColor[0] = (int)(losColorScale * 0.1f);
	jamColor[1] = (int)(losColorScale * 0.0f);
	jamColor[2] = (int)(losColorScale * 0.0f);

	losColor[0] = (int)(losColorScale * 0.3f);
	losColor[1] = (int)(losColorScale * 0.3f);
	losColor[2] = (int)(losColorScale * 0.3f);

	radarColor[0] = (int)(losColorScale * 0.0f);
	radarColor[1] = (int)(losColorScale * 0.0f);
	radarColor[2] = (int)(losColorScale * 1.0f);

	alwaysColor[0] = (int)(losColorScale * 0.2f);
	alwaysColor[1] = (int)(losColorScale * 0.2f);
	alwaysColor[2] = (int)(losColorScale * 0.2f);

	radarColor2[0] = (int)(losColorScale * 0.0f);
	radarColor2[1] = (int)(losColorScale * 1.0f);
	radarColor2[2] = (int)(losColorScale * 0.0f);
}

