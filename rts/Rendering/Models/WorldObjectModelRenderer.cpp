/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WorldObjectModelRenderer.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#define LOG_SECTION_WORLD_OBJECT_MODEL_RENDERER "WorldObjectModelRenderer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_WORLD_OBJECT_MODEL_RENDERER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_WORLD_OBJECT_MODEL_RENDERER


IWorldObjectModelRenderer* IWorldObjectModelRenderer::GetInstance(int modelType)
{
	switch (modelType) {
		case MODELTYPE_3DO: return (new WorldObjectModelRenderer3DO());
		case MODELTYPE_S3O: return (new WorldObjectModelRendererS3O());
		case MODELTYPE_OBJ: return (new WorldObjectModelRendererOBJ());
		case MODELTYPE_ASS: return (new WorldObjectModelRendererASS());
		default: return (new IWorldObjectModelRenderer(MODELTYPE_OTHER));
	}
}

IWorldObjectModelRenderer::~IWorldObjectModelRenderer()
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

void IWorldObjectModelRenderer::Draw()
{
	PushRenderState();

	for (auto& uIt: units) {
		DrawModels(uIt.second);
	}
	for (auto& fIt: features) {
		DrawModels(fIt.second);
	}
	for (auto& pIt: projectiles) {
		DrawModels(pIt.second);
	}

	PopRenderState();
}



void IWorldObjectModelRenderer::DrawModels(const UnitSet& models)
{
	for (auto& unit: models) {
		DrawModel(unit);
	}
}

void IWorldObjectModelRenderer::DrawModels(const FeatureSet& models)
{
	for (auto drawFeature: models) {
		DrawModel(drawFeature);
	}
}

void IWorldObjectModelRenderer::DrawModels(const ProjectileSet& models)
{
	for (auto& projectile: models) {
		DrawModel(projectile);
	}
}



void IWorldObjectModelRenderer::AddUnit(const CUnit* u)
{
	UnitSet& us = units[TEX_TYPE(u)];

	// updating a unit's draw-position requires mutability
	if (!VectorInsertUnique(us, const_cast<CUnit*>(u)))
		assert(false);

	numUnits += 1;
}

void IWorldObjectModelRenderer::DelUnit(const CUnit* u)
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


void IWorldObjectModelRenderer::AddFeature(const CFeature* f)
{
	FeatureSet& fs = features[TEX_TYPE(f)];

	if (!VectorInsertUnique(fs, const_cast<CFeature*>(f)))
		assert(false);

	numFeatures += 1;
}

void IWorldObjectModelRenderer::DelFeature(const CFeature* f)
{
	FeatureSet& fs = features[TEX_TYPE(f)];

	if (VectorErase(fs, const_cast<CFeature*>(f))) {
		numFeatures -= 1;
	}

	if (fs.empty()) {
		features.erase(TEX_TYPE(f));
	}
}


void IWorldObjectModelRenderer::AddProjectile(const CProjectile* p)
{
	ProjectileSet& ps = projectiles[TEX_TYPE(p)];

	// updating a projectile's draw-position requires mutability
	if (!VectorInsertUnique(ps, const_cast<CProjectile*>(p)))
		assert(false);

	numProjectiles += 1;
}

void IWorldObjectModelRenderer::DelProjectile(const CProjectile* p)
{
	ProjectileSet& ps = projectiles[TEX_TYPE(p)];

	if (!VectorErase(ps, const_cast<CProjectile*>(p)))
		assert(false);

	numProjectiles -= 1;
}



void WorldObjectModelRenderer3DO::PushRenderState()
{
	texturehandler3DO->Set3doAtlases();
	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);
}
void WorldObjectModelRenderer3DO::PopRenderState()
{
	glPopAttrib();
}

void WorldObjectModelRendererS3O::PushRenderState()
{
	if (globalRendering->supportRestartPrimitive)
		glPrimitiveRestartIndexNV(-1U);
}
void WorldObjectModelRendererS3O::PopRenderState()
{
	// no-op
}

void WorldObjectModelRendererOBJ::PushRenderState() // TODO implement me
{
}
void WorldObjectModelRendererOBJ::PopRenderState() // TODO implement me
{
}

void WorldObjectModelRendererASS::PushRenderState() // TODO implement me
{
}
void WorldObjectModelRendererASS::PopRenderState() // TODO implement me
{
}

void WorldObjectModelRendererS3O::DrawModel(const CUnit* u)
{
	LOG_L(L_DEBUG, "[%s(%s)] id=%d", __FUNCTION__, "CUnit", u->id);
}

void WorldObjectModelRendererS3O::DrawModel(const CFeature* f)
{
	LOG_L(L_DEBUG, "[%s(%s)] id=%d", __FUNCTION__, "CFeature", f->id);
}

void WorldObjectModelRendererS3O::DrawModel(const CProjectile* p)
{
	LOG_L(L_DEBUG, "[%s(%s)] id=%d", __FUNCTION__, "CProjectile", p->id);
}
