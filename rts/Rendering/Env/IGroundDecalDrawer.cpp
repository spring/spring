/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "Rendering/Env/Decals/ShaderGroundDecalDrawer.h"
#include "System/Config/ConfigHandler.h"


CONFIG(int, GroundDecals).defaultValue(1);

bool IGroundDecalDrawer::drawDecals = 0;
unsigned int IGroundDecalDrawer::decalLevel = 0;


IGroundDecalDrawer::IGroundDecalDrawer()
{
	decalLevel = std::max(0, configHandler->GetInt("GroundDecals"));
}


IGroundDecalDrawer* IGroundDecalDrawer::GetInstance()
{
	static CGroundDecalHandler groundDecl; //FIXME
	return &groundDecl;
}
