/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WorldObjectModelRenderer.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "System/Log/ILog.h"

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
	for (UnitRenderBinIt uIt = units.begin(); uIt != units.end(); ++uIt) {
		units[uIt->first].clear();
	}
	for (FeatureRenderBinIt fIt = features.begin(); fIt != features.end(); ++fIt) {
		features[fIt->first].clear();
	}
	for (ProjectileRenderBinIt pIt = projectiles.begin(); pIt != projectiles.end(); ++pIt) {
		projectiles[pIt->first].clear();
	}

	units.clear();
	features.clear();
	projectiles.clear();
}

void IWorldObjectModelRenderer::Draw()
{
	PushRenderState();

	{
		GML_RECMUTEX_LOCK(unit); // Draw

		for (UnitRenderBinIt uIt = units.begin(); uIt != units.end(); ++uIt) {
			DrawModels(units[uIt->first]);
		}
	}
	{
		GML_RECMUTEX_LOCK(feat); // Draw

		for (FeatureRenderBinIt fIt = features.begin(); fIt != features.end(); ++fIt) {
			DrawModels(features[fIt->first]);
		}
	}
	{
		GML_RECMUTEX_LOCK(proj); // Draw

		for (ProjectileRenderBinIt pIt = projectiles.begin(); pIt != projectiles.end(); ++pIt) {
			DrawModels(projectiles[pIt->first]);
		}
	}

	PopRenderState();
}



void IWorldObjectModelRenderer::DrawModels(const UnitSet& models)
{
	for (UnitSetIt uIt = models.begin(); uIt != models.end(); ++uIt) {
		DrawModel(*uIt);
	}
}

void IWorldObjectModelRenderer::DrawModels(const FeatureSet& models)
{
	for (FeatureSetIt fIt = models.begin(); fIt != models.end(); ++fIt) {
		DrawModel(fIt->first);
	}
}

void IWorldObjectModelRenderer::DrawModels(const ProjectileSet& models)
{
	for (ProjectileSetIt pIt = models.begin(); pIt != models.end(); ++pIt) {
		DrawModel(*pIt);
	}
}



void IWorldObjectModelRenderer::AddUnit(const CUnit* u)
{
	if (units.find(TEX_TYPE(u)) == units.end()) {
		units[TEX_TYPE(u)] = UnitSet();
	}

	// updating a unit's draw-position requires mutability
	if(units[TEX_TYPE(u)].insert(const_cast<CUnit*>(u)).second)
		numUnits += 1;
}

void IWorldObjectModelRenderer::DelUnit(const CUnit* u)
{
	if(units[TEX_TYPE(u)].erase(const_cast<CUnit*>(u)))
		numUnits -= 1;

	if (units[TEX_TYPE(u)].empty()) {
		units.erase(TEX_TYPE(u));
	}
}


void IWorldObjectModelRenderer::AddFeature(const CFeature* f, float alpha)
{
	if (features.find(TEX_TYPE(f)) == features.end()) {
		features[TEX_TYPE(f)] = FeatureSet();
	}

	FeatureSet &fs = features.find(TEX_TYPE(f))->second;
	FeatureSet::iterator i = fs.find(const_cast<CFeature*>(f));
	if(i != fs.end()) {
		if(i->second != alpha) {
			fs[const_cast<CFeature*>(f)] = alpha;
		}
	}
	else {
		fs[const_cast<CFeature*>(f)] = alpha;
		numFeatures += 1;
	}
}

void IWorldObjectModelRenderer::DelFeature(const CFeature* f)
{
	{
		FeatureRenderBin::iterator i = features.find(TEX_TYPE(f));
		if (i != features.end()) {
			if((*i).second.erase(const_cast<CFeature*>(f)))
				numFeatures -= 1;

			if ((*i).second.empty())
				features.erase(TEX_TYPE(f));
		}
	}

	{
		FeatureRenderBin::iterator i = featuresSave.find(TEX_TYPE(f));
		if (i != featuresSave.end()) {
			if((*i).second.erase(const_cast<CFeature*>(f)))
				numFeaturesSave -= 1;

			if ((*i).second.empty())
				featuresSave.erase(TEX_TYPE(f));
		}
	}
}

void IWorldObjectModelRenderer::SwapFeatures()
{
	features.swap(featuresSave);
	std::swap(numFeatures, numFeaturesSave);
}

void IWorldObjectModelRenderer::AddProjectile(const CProjectile* p)
{
	if (projectiles.find(TEX_TYPE(p)) == projectiles.end()) {
		projectiles[TEX_TYPE(p)] = ProjectileSet();
	}

	if(projectiles[TEX_TYPE(p)].insert(const_cast<CProjectile*>(p)).second)
		numProjectiles += 1;
}

void IWorldObjectModelRenderer::DelProjectile(const CProjectile* p)
{
	if(projectiles[TEX_TYPE(p)].erase(const_cast<CProjectile*>(p)))
		numProjectiles -= 1;

	if (projectiles[TEX_TYPE(p)].empty()) {
		projectiles.erase(TEX_TYPE(p));
	}
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
