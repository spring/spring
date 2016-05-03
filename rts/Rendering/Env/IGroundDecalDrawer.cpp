/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "Rendering/Env/Decals/DecalsDrawerGL4.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"


CONFIG(int, GroundDecals).defaultValue(1).headlessValue(0).minimumValue(0).description("Controls whether ground decals underneath buildings and ground scars from explosions will be rendered. Values >1 define how long such decals will stay.");

int IGroundDecalDrawer::decalLevel = 0;


static IGroundDecalDrawer* singleton = NULL;

IGroundDecalDrawer::IGroundDecalDrawer()
{
	decalLevel = configHandler->GetInt("GroundDecals");
}


IGroundDecalDrawer* IGroundDecalDrawer::GetInstance()
{
	if (!singleton) {
#if 0
		try {
			singleton = new CDecalsDrawerGL4();
			LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "GL4");
		} catch(const unsupported_error& ex) {
		} catch(const opengl_error& ex) {
			LOG_L(L_ERROR, "IGroundDecalDrawer loading failed: %s", ex.what());
#endif
			SafeDelete(singleton);
			singleton = new CGroundDecalHandler();
			LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "Legacy");

#if 0
		}
#endif
	}

	return singleton;
}


void IGroundDecalDrawer::FreeInstance()
{
	SafeDelete(singleton);
}


void IGroundDecalDrawer::SetDrawDecals(bool v)
{
	if (v) {
		decalLevel =  std::abs(decalLevel);
	} else {
		decalLevel = -std::abs(decalLevel);
	}
}
