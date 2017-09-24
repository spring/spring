/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "Rendering/Env/Decals/DecalsDrawerGL4.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"


CONFIG(int, GroundDecals).defaultValue(3).headlessValue(0).description("Controls whether ground decals underneath buildings and ground scars from explosions will be rendered. Values >1 define how long such decals will stay.");

static NullGroundDecalDrawer nullDecalDrawer;
IGroundDecalDrawer* IGroundDecalDrawer::singleton = &nullDecalDrawer;
int IGroundDecalDrawer::decalLevel = 0;


static IGroundDecalDrawer* GetInstance()
{
	IGroundDecalDrawer* instance = &nullDecalDrawer;
	if (!IGroundDecalDrawer::GetDrawDecals()) {
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "off");
		return instance;
	}

#if 0
	try {
		instance = new CDecalsDrawerGL4();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "GL4");
	} catch(const unsupported_error& ex) {
		LOG_L(L_ERROR, "IGroundDecalDrawer loading failed: %s", ex.what());
	} catch(const opengl_error& ex) {
		LOG_L(L_ERROR, "IGroundDecalDrawer loading failed: %s", ex.what());
	}
#endif

	if (instance == &nullDecalDrawer) {
		instance = new CGroundDecalHandler();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "Legacy");
	}

	return instance;
}


void IGroundDecalDrawer::Init()
{
	decalLevel = configHandler->GetInt("GroundDecals");

	FreeInstance();
	singleton = GetInstance();
}


void IGroundDecalDrawer::FreeInstance()
{
	if (singleton != &nullDecalDrawer)
		spring::SafeDelete(singleton);
}


void IGroundDecalDrawer::SetDrawDecals(bool v)
{
	if (v) {
		decalLevel =  std::abs(decalLevel);
	} else {
		decalLevel = -std::abs(decalLevel);
	}

	if (groundDecals == &nullDecalDrawer) {
		groundDecals = GetInstance();
	}

	groundDecals->OnDecalLevelChanged();
}
