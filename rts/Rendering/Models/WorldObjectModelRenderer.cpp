#include "WorldObjectModelRenderer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "System/LogOutput.h"

#define WORLDOBJECT_MODEL_RENDERER_DEBUG 0

IWorldObjectModelRenderer* IWorldObjectModelRenderer::GetInstance(int modelType)
{
	switch (modelType) {
		case MODELTYPE_3DO: { return (new WorldObjectModelRenderer3DO()); } break;
		case MODELTYPE_S3O: { return (new WorldObjectModelRendererS3O()); } break;
		case MODELTYPE_OBJ: { return (new WorldObjectModelRendererOBJ()); } break;
		default: { return (new IWorldObjectModelRenderer(MODELTYPE_OTHER)); } break;
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
		GML_STDMUTEX_LOCK(proj); // Draw

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
		DrawModel(*fIt);
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


void IWorldObjectModelRenderer::AddFeature(const CFeature* f)
{
	if (features.find(TEX_TYPE(f)) == features.end()) {
		features[TEX_TYPE(f)] = FeatureSet();
	}

	if(features[TEX_TYPE(f)].insert(const_cast<CFeature*>(f)).second)
		numFeatures += 1;
}

void IWorldObjectModelRenderer::DelFeature(const CFeature* f)
{
	if(features[TEX_TYPE(f)].erase(const_cast<CFeature*>(f)))
		numFeatures -= 1;

	if (features[TEX_TYPE(f)].empty()) {
		features.erase(TEX_TYPE(f));
	}
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
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif

	texturehandler3DO->Set3doAtlases();
	glPushAttrib(GL_POLYGON_BIT);
	glDisable(GL_CULL_FACE);
}
void WorldObjectModelRenderer3DO::PopRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif

	glPopAttrib();
}

void WorldObjectModelRendererS3O::PushRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	// no-op
}
void WorldObjectModelRendererS3O::PopRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	// no-op
}

void WorldObjectModelRendererOBJ::PushRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	// WRITEME
}
void WorldObjectModelRendererOBJ::PopRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	// WRITEME
}



void WorldObjectModelRendererS3O::DrawModel(const CUnit* u)
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	logOutput.Print("[WorldObjectModelRendererS3O::DrawModel(CUnit)] id=%d", u->id);
	#endif
}

void WorldObjectModelRendererS3O::DrawModel(const CFeature* f)
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	logOutput.Print("[WorldObjectModelRendererS3O::DrawModel(CFeature)] id=%d", f->id);
	#endif
}

void WorldObjectModelRendererS3O::DrawModel(const CProjectile* p)
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	logOutput.Print("[WorldObjectModelRendererS3O::DrawModel(CProjectile)] id=%d", p->id);
	#endif
}
