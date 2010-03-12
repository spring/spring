/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ModelDrawer.hpp"
#include "Game/Game.h"
#include "Rendering/UnitModels/3DModel.h"
#include "Rendering/Shaders/ShaderHandler.hpp"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "System/EventHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"

#define MODEL_DRAWER_DEBUG 0

IModelDrawer* modelDrawer = NULL;

IModelDrawer* IModelDrawer::GetInstance()
{
	if (modelDrawer == NULL) {
		if (!gu->haveARB && !gu->haveGLSL) {
			modelDrawer = new CModelDrawerFFP("[CModelDrawerFFP]", 31459, false);
		} else {
			if (gu->haveGLSL) {
				modelDrawer = new CModelDrawerGLSL("[CModelDrawerGLSL]", 31459, false);
			} else {
				if (gu->haveARB) {
					modelDrawer = new CModelDrawerARB("[CModelDrawerARB]", 31459, false);
				}
			}
		}
	}

	return modelDrawer;
}


IModelDrawer::IModelDrawer(const std::string& name, int order, bool synced): CEventClient(name, order, synced)
{
	eventHandler.AddClient(this);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType] = std::vector<Shader::IProgramObject*>();
		shaders[modelType].resize(CGame::gameRefractionDraw + 1, NULL);

		renderableUnits[modelType] = std::set<const CUnit*>();
		renderableFeatures[modelType] = std::set<const CFeature*>();
		renderableProjectiles[modelType] = std::set<const CProjectile*>();
	}

	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::IModelDrawer] this=%p, name=%s, order=%d, synced=%d", this, name.c_str(), order, synced);
	#endif
}

IModelDrawer::~IModelDrawer()
{
	eventHandler.RemoveClient(this);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType].clear();

		renderableUnits[modelType].clear();
		renderableFeatures[modelType].clear();
		renderableProjectiles[modelType].clear();
	}

	shaderHandler->ReleaseProgramObjects("[ModelDrawer]");
	shaders.clear();

	renderableUnits.clear();
	renderableFeatures.clear();
	renderableProjectiles.clear();

	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::~IModelDrawer]");
	#endif
}




void IModelDrawer::UnitCreated(const CUnit* u, const CUnit*)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitCreated] id=%d", u->id);
	#endif

	if (u->model) {
		renderableUnits[u->model->type].insert(u);
	}
}
void IModelDrawer::UnitDestroyed(const CUnit* u, const CUnit*)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitDestroyed] id=%d", u->id);
	#endif

	if (u->model) {
		renderableUnits[u->model->type].erase(u);
	}
}

void IModelDrawer::FeatureCreated(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::FeatureCreated] id=%d", f->id);
	#endif

	if (f->model) {
		renderableFeatures[f->model->type].insert(f);
	}
}
void IModelDrawer::FeatureDestroyed(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::FeatureDestroyed] id=%d", f->id);
	#endif

	if (f->model) {
		renderableFeatures[f->model->type].erase(f);
	}
}

void IModelDrawer::ProjectileCreated(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::ProjectileCreated] id=%d", p->id);
	#endif

	if (p->model) {
		renderableProjectiles[p->model->type].insert(p);
	}
}
void IModelDrawer::ProjectileDestroyed(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::ProjectileDestroyed] id=%d", p->id);
	#endif

	if (p->model) {
		renderableProjectiles[p->model->type].erase(p);
	}
}




void IModelDrawer::Draw()
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::Draw] frame=%d", gs->frameNum);
	#endif

	// TODO: LuaUnitRendering bypass
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		PushRenderState(modelType);
		DrawModels(renderableUnits[modelType]);
		DrawModels(renderableFeatures[modelType]);
		DrawModels(renderableProjectiles[modelType]);
		PopRenderState(modelType);
	}
}

void IModelDrawer::DrawModels(const std::set<const CUnit*>& models) 
{
	std::set<const CUnit*>::const_iterator uIt;

	for (uIt = models.begin(); uIt != models.end(); uIt++) {
		DrawModel(*uIt);
	}
}

void IModelDrawer::DrawModels(const std::set<const CFeature*>& models)
{
	std::set<const CFeature*>::const_iterator fIt;

	for (fIt = models.begin(); fIt != models.end(); fIt++) {
		DrawModel(*fIt);
	}
}

void IModelDrawer::DrawModels(const std::set<const CProjectile*>& models)
{
	std::set<const CProjectile*>::const_iterator pIt;

	for (pIt = models.begin(); pIt != models.end(); pIt++) {
		DrawModel(*pIt);
	}
}






CModelDrawerFFP::CModelDrawerFFP(const std::string& name, int order, bool synced): IModelDrawer(name, order, synced)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerFFP::CModelDrawerFFP] name=%s, order=%d, synced=%d", name.c_str(), order, synced);
	#endif
}






CModelDrawerARB::CModelDrawerARB(const std::string& name, int order, bool synced): IModelDrawer(name, order, synced)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerARB::CModelDrawerARB] name=%s, order=%d, synced=%d", name.c_str(), order, synced);
	#endif

	LoadModelShaders();
}






CModelDrawerGLSL::CModelDrawerGLSL(const std::string& name, int order, bool synced): IModelDrawer(name, order, synced)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::CModelDrawerGLSL] name=%s, order=%d, synced=%d", name.c_str(), order, synced);
	#endif

	LoadModelShaders();
}

bool CModelDrawerGLSL::LoadModelShaders()
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::LoadModelShaders]");
	#endif

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		for (int drawMode = CGame::gameNotDrawing; drawMode < CGame::gameRefractionDraw + 1; drawMode++) {
			if (drawMode == CGame::gameShadowDraw) {
				shaders[modelType][drawMode] = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
			} else {
				shaders[modelType][drawMode] = shaderHandler->CreateProgramObject(GetName(), "$DUMMY$", false);
			}
		}
	}

	return false;
}


void CModelDrawerGLSL::UnitCreated(const CUnit* u, const CUnit*)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::UnitCreated] id=%d", u->id);
	#endif

	IModelDrawer::UnitCreated(u, NULL);
}
void CModelDrawerGLSL::UnitDestroyed(const CUnit* u, const CUnit*)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::UnitDestroyed] id=%d", u->id);
	#endif

	IModelDrawer::UnitDestroyed(u, NULL);
}

void CModelDrawerGLSL::FeatureCreated(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::FeatureCreated] id=%d", f->id);
	#endif

	IModelDrawer::FeatureCreated(f);
}
void CModelDrawerGLSL::FeatureDestroyed(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::FeatureDestroyed] id=%d", f->id);
	#endif

	IModelDrawer::FeatureDestroyed(f);
}

void CModelDrawerGLSL::ProjectileCreated(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::ProjectileCreated] id=%d", p->id);
	#endif

	IModelDrawer::ProjectileCreated(p);
}
void CModelDrawerGLSL::ProjectileDestroyed(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::ProjectileDestroyed] id=%d", p->id);
	#endif

	IModelDrawer::ProjectileDestroyed(p);
}


void CModelDrawerGLSL::Draw()
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::Draw] frame=%d", gs->frameNum);
	#endif

	IModelDrawer::Draw();
}


void CModelDrawerGLSL::DrawModels(const std::set<const CUnit*>& units)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModels(units)]");
	#endif

	IModelDrawer::DrawModels(units);
}

void CModelDrawerGLSL::DrawModels(const std::set<const CFeature*>& features)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModels(features)]");
	#endif

	IModelDrawer::DrawModels(features);
}

void CModelDrawerGLSL::DrawModels(const std::set<const CProjectile*>& projectiles)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModels(projectiles)]");
	#endif

	IModelDrawer::DrawModels(projectiles);
}



void CModelDrawerGLSL::PushRenderState(int modelType)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::PushRenderState] modelType=%d, gameDrawMode=%d", modelType, game->gameDrawMode);
	#endif

	switch (modelType) {
		case MODELTYPE_3DO: {
		} break;
		case MODELTYPE_S3O: {
		} break;
		default: {
		} break;
	}

	shaders[modelType][game->gameDrawMode]->Enable();
}

void CModelDrawerGLSL::PopRenderState(int modelType)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::PopRenderState] modelType=%d, gameDrawMode=%d", modelType, game->gameDrawMode);
	#endif

	shaders[modelType][game->gameDrawMode]->Disable();

	switch (modelType) {
		case MODELTYPE_3DO: {
		} break;
		case MODELTYPE_S3O: {
		} break;
		default: {
		} break;
	}
}


void CModelDrawerGLSL::DrawModel(const CUnit* u)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModel(CUnit)] id=%d", u->id);
	#endif
}
void CModelDrawerGLSL::DrawModel(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModel(CFeature)] id=%d", f->id);
	#endif
}
void CModelDrawerGLSL::DrawModel(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModel(CProjectile)] id=%d", p->id);
	#endif
}
