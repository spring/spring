/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ModelRenderContainer.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#define LOG_SECTION_WORLD_OBJECT_MODEL_RENDERER "ModelRenderContainer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_WORLD_OBJECT_MODEL_RENDERER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_WORLD_OBJECT_MODEL_RENDERER


IModelRenderContainer* IModelRenderContainer::GetInstance(int modelType)
{
	switch (modelType) {
		case MODELTYPE_3DO: return (new ModelRenderContainer3DO());
		case MODELTYPE_S3O: return (new ModelRenderContainerS3O());
		case MODELTYPE_OBJ: return (new ModelRenderContainerOBJ());
		case MODELTYPE_ASS: return (new ModelRenderContainerASS());
		default: return (new IModelRenderContainer(MODELTYPE_OTHER));
	}
}

IModelRenderContainer::~IModelRenderContainer()
{
	for (auto& uIt: units) {
		uIt.second.clear();
	}
	for (auto& fIt: features) {
		fIt.second.clear();
	}
	for (auto& pIt: projectiles) {
		pIt.second.clear();
	}

	units.clear();
	features.clear();
	projectiles.clear();
}



void IModelRenderContainer::AddUnit(const CUnit* u)
{
	UnitSet& us = units[TEX_TYPE(u)];

	// updating a unit's draw-position requires mutability
	if (!VectorInsertUnique(us, const_cast<CUnit*>(u)))
		assert(false);

	numUnits += 1;
}

void IModelRenderContainer::DelUnit(const CUnit* u)
{
	UnitSet& us = units[TEX_TYPE(u)];

	// Unit can be absent from this container, since we can't
	// know in UnitDrawer.cpp whether it's cloaked or not.
	if (VectorErase(us, const_cast<CUnit*>(u))) {
		numUnits -= 1;
	}

	if (us.empty()) {
		units.erase(TEX_TYPE(u));
	}
}


void IModelRenderContainer::AddFeature(const CFeature* f)
{
	FeatureSet& fs = features[TEX_TYPE(f)];

	if (!VectorInsertUnique(fs, const_cast<CFeature*>(f)))
		assert(false);

	numFeatures += 1;
}

void IModelRenderContainer::DelFeature(const CFeature* f)
{
	FeatureSet& fs = features[TEX_TYPE(f)];

	if (VectorErase(fs, const_cast<CFeature*>(f))) {
		numFeatures -= 1;
	}

	if (fs.empty()) {
		features.erase(TEX_TYPE(f));
	}
}


void IModelRenderContainer::AddProjectile(const CProjectile* p)
{
	ProjectileSet& ps = projectiles[TEX_TYPE(p)];

	// updating a projectile's draw-position requires mutability
	if (!VectorInsertUnique(ps, const_cast<CProjectile*>(p)))
		assert(false);

	numProjectiles += 1;
}

void IModelRenderContainer::DelProjectile(const CProjectile* p)
{
	ProjectileSet& ps = projectiles[TEX_TYPE(p)];

	if (!VectorErase(ps, const_cast<CProjectile*>(p)))
		assert(false);

	numProjectiles -= 1;
}

