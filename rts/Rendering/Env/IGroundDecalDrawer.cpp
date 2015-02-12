/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "Rendering/Env/Decals/DecalsDrawerGL4.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"


CONFIG(int, GroundDecals).defaultValue(1).headlessValue(0).minimumValue(0).description("Controls whether ground decals underneath buildings and ground scars from explosions will be rendered. Values >1 define how long such decals will stay.");

bool IGroundDecalDrawer::drawDecals = 0;
unsigned int IGroundDecalDrawer::decalLevel = 0;

static IGroundDecalDrawer* singleton = NULL;

IGroundDecalDrawer::IGroundDecalDrawer()
{
	decalLevel = std::max(0, configHandler->GetInt("GroundDecals"));
	drawDecals = (decalLevel > 0); //FIXME
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
			delete singleton;
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
	delete singleton;
	singleton = NULL;
}
