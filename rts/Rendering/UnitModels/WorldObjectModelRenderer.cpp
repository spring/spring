#include "WorldObjectModelRenderer.h"
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
	//	case MODELTYPE_OBJ: { return (new WorldObjectModelRendererOBJ()); } break;
		default: { return (new IWorldObjectModelRenderer(MODELTYPE_OTHER)); } break;
	}
}

IWorldObjectModelRenderer::~IWorldObjectModelRenderer()
{
	for (UnitRenderBinIt uIt = units.begin(); uIt != units.end(); uIt++) {
		units[uIt->first].clear();
	}
	for (FeatureRenderBinIt fIt = features.begin(); fIt != features.end(); fIt++) {
		features[fIt->first].clear();
	}
	for (ProjectileRenderBinIt pIt = projectiles.begin(); pIt != projectiles.end(); pIt++) {
		projectiles[pIt->first].clear();
	}

	units.clear();
	features.clear();
	projectiles.clear();
}

void IWorldObjectModelRenderer::Draw()
{
	PushRenderState();

	for (UnitRenderBinIt uIt = units.begin(); uIt != units.end(); uIt++) {
		DrawModels(units[uIt->first]);
	}
	for (FeatureRenderBinIt fIt = features.begin(); fIt != features.end(); fIt++) {
		DrawModels(features[fIt->first]);
	}
	for (ProjectileRenderBinIt pIt = projectiles.begin(); pIt != projectiles.end(); pIt++) {
		DrawModels(projectiles[pIt->first]);
	}

	PopRenderState();
}



void IWorldObjectModelRenderer::DrawModels(const UnitSet& models) 
{
	for (UnitSetIt uIt = models.begin(); uIt != models.end(); uIt++) {
		DrawModel(*uIt);
	}
}

void IWorldObjectModelRenderer::DrawModels(const FeatureSet& models)
{
	for (FeatureSetIt fIt = models.begin(); fIt != models.end(); fIt++) {
		DrawModel(*fIt);
	}
}

void IWorldObjectModelRenderer::DrawModels(const ProjectileSet& models)
{
	for (ProjectileSetIt pIt = models.begin(); pIt != models.end(); pIt++) {
		DrawModel(*pIt);
	}
}




void IWorldObjectModelRenderer::AddUnit(const CUnit* u)
{
	if (units.find(TEX_TYPE(u)) == units.end()) {
		units[TEX_TYPE(u)] = UnitSet();
	}

	units[TEX_TYPE(u)].insert(u);
}

void IWorldObjectModelRenderer::DelUnit(const CUnit* u)
{
	units[TEX_TYPE(u)].erase(u);
}


void IWorldObjectModelRenderer::AddFeature(const CFeature* f)
{
	if (features.find(TEX_TYPE(f)) == features.end()) {
		features[TEX_TYPE(f)] = FeatureSet();
	}

	features[TEX_TYPE(f)].insert(f);
}

void IWorldObjectModelRenderer::DelFeature(const CFeature* f)
{
	features[TEX_TYPE(f)].erase(f);
}


void IWorldObjectModelRenderer::AddProjectile(const CProjectile* p)
{
	if (projectiles.find(TEX_TYPE(p)) == projectiles.end()) {
		projectiles[TEX_TYPE(p)] = ProjectileSet();
	}

	projectiles[TEX_TYPE(p)].insert(p);
}

void IWorldObjectModelRenderer::DelProjectile(const CProjectile* p)
{
	projectiles[TEX_TYPE(p)].erase(p);
}



void WorldObjectModelRenderer3DO::PushRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	/* SetupFor3DO */
}
void WorldObjectModelRenderer3DO::PopRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	/* Cleanup3DO */
}

void WorldObjectModelRendererS3O::PushRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	/* SetupForS3O */
}
void WorldObjectModelRendererS3O::PopRenderState()
{
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
	/* CleanupS3O */
}

void WorldObjectModelRendererOBJ::PushRenderState()
{
	// WRITEME
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
}
void WorldObjectModelRendererOBJ::PopRenderState()
{
	// WRITEME
	#if (WORLDOBJECT_MODEL_RENDERER_DEBUG == 1)
	#endif
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
