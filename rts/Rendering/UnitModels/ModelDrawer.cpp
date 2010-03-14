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

#define MODEL_DRAWER_DEBUG 2
#define MDL_TYPE(o) (o->model->type)
#define TEX_TYPE(o) (o->model->textureType)

IModelDrawer* modelDrawer = NULL;

IModelDrawer* IModelDrawer::GetInstance()
{
	if (modelDrawer == NULL) {
		if (!gu->haveARB && !gu->haveGLSL) {
			modelDrawer = new CModelDrawerFFP("[CModelDrawerFFP]", 314596, false);
		} else {
			if (gu->haveGLSL) {
				modelDrawer = new CModelDrawerGLSL("[CModelDrawerGLSL]", 314596, false);
			} else {
				if (gu->haveARB) {
					modelDrawer = new CModelDrawerARB("[CModelDrawerARB]", 314596, false);
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

		opaqueUnits[modelType] = UnitRenderBin();
		opaqueFeatures[modelType] = FeatureRenderBin();
		opaqueProjectiles[modelType] = ProjectileRenderBin();

		cloakedUnits[modelType] = UnitRenderBin();
		cloakedFeatures[modelType] = FeatureRenderBin();
		cloakedProjectiles[modelType] = ProjectileRenderBin();
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

		UnitRenderBinIt uIt = opaqueUnits[modelType].begin();
		FeatureRenderBinIt fIt = opaqueFeatures[modelType].begin();
		ProjectileRenderBinIt pIt = opaqueProjectiles[modelType].begin();

		for (; uIt != opaqueUnits[modelType].end(); uIt++) { opaqueUnits[modelType][uIt->first].clear(); }
		for (; fIt != opaqueFeatures[modelType].end(); fIt++) { opaqueFeatures[modelType][fIt->first].clear(); }
		for (; pIt != opaqueProjectiles[modelType].end(); pIt++) { opaqueProjectiles[modelType][pIt->first].clear(); }

		uIt = cloakedUnits[modelType].begin();
		fIt = cloakedFeatures[modelType].begin();
		pIt = cloakedProjectiles[modelType].begin();

		for (; uIt != cloakedUnits[modelType].end(); uIt++) { cloakedUnits[modelType][uIt->first].clear(); }
		for (; fIt != cloakedFeatures[modelType].end(); fIt++) { cloakedFeatures[modelType][fIt->first].clear(); }
		for (; pIt != cloakedProjectiles[modelType].end(); pIt++) { cloakedProjectiles[modelType][pIt->first].clear(); }

		opaqueUnits[modelType].clear();
		opaqueFeatures[modelType].clear();
		opaqueProjectiles[modelType].clear();

		cloakedUnits[modelType].clear();
		cloakedFeatures[modelType].clear();
		cloakedProjectiles[modelType].clear();
	}

	shaderHandler->ReleaseProgramObjects("[ModelDrawer]");
	shaders.clear();

	opaqueUnits.clear();
	opaqueFeatures.clear();
	opaqueProjectiles.clear();

	cloakedUnits.clear();
	cloakedFeatures.clear();
	cloakedProjectiles.clear();

	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::~IModelDrawer]");
	#endif
}




void IModelDrawer::UnitCreated(const CUnit* u, const CUnit*)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitCreated] id=%d", u->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (u->model) {
		if (u->isCloaked) {
			// units can start life cloaked
			if (cloakedUnits[MDL_TYPE(u)].find(TEX_TYPE(u)) == cloakedUnits[MDL_TYPE(u)].end())
				cloakedUnits[MDL_TYPE(u)][TEX_TYPE(u)] = UnitSet();

			cloakedUnits[MDL_TYPE(u)][TEX_TYPE(u)].insert(u);
		} else {
			if (opaqueUnits[MDL_TYPE(u)].find(TEX_TYPE(u)) == opaqueUnits[MDL_TYPE(u)].end())
				opaqueUnits[MDL_TYPE(u)][TEX_TYPE(u)] = UnitSet();

			opaqueUnits[MDL_TYPE(u)][TEX_TYPE(u)].insert(u);
		}
	}
}

void IModelDrawer::UnitDestroyed(const CUnit* u, const CUnit*)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitDestroyed] id=%d", u->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (u->model) {
		if (u->isCloaked) {
			cloakedUnits[MDL_TYPE(u)][TEX_TYPE(u)].erase(u);
		} else {
			opaqueUnits[MDL_TYPE(u)][TEX_TYPE(u)].erase(u);
		}
	}
}


void IModelDrawer::UnitCloaked(const CUnit* u)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitCloaked] id=%d", u->id);
	#endif

	if (cloakedUnits[MDL_TYPE(u)].find(TEX_TYPE(u)) == cloakedUnits[MDL_TYPE(u)].end())
		cloakedUnits[MDL_TYPE(u)][TEX_TYPE(u)] = UnitSet();

	cloakedUnits[MDL_TYPE(u)][TEX_TYPE(u)].insert(u);
	opaqueUnits[MDL_TYPE(u)][TEX_TYPE(u)].erase(u);
}

void IModelDrawer::UnitDecloaked(const CUnit* u)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitDecloaked] id=%d", u->id);
	#endif

	if (opaqueUnits[MDL_TYPE(u)].find(TEX_TYPE(u)) == opaqueUnits[MDL_TYPE(u)].end())
		opaqueUnits[MDL_TYPE(u)][TEX_TYPE(u)] = UnitSet();

	opaqueUnits[MDL_TYPE(u)][TEX_TYPE(u)].insert(u);
	cloakedUnits[MDL_TYPE(u)][TEX_TYPE(u)].erase(u);
}


void IModelDrawer::FeatureCreated(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::FeatureCreated] id=%d", f->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (f->model) {
		if (opaqueFeatures[MDL_TYPE(f)].find(TEX_TYPE(f)) == opaqueFeatures[MDL_TYPE(f)].end())
			opaqueFeatures[MDL_TYPE(f)][TEX_TYPE(f)] = FeatureSet();

		opaqueFeatures[MDL_TYPE(f)][TEX_TYPE(f)].insert(f);
	}
}

void IModelDrawer::FeatureDestroyed(const CFeature* f)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::FeatureDestroyed] id=%d", f->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (f->model) {
		opaqueFeatures[MDL_TYPE(f)][TEX_TYPE(f)].erase(f);
	}
}


void IModelDrawer::ProjectileCreated(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::ProjectileCreated] id=%d", p->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (p->model) {
		if (opaqueProjectiles[MDL_TYPE(p)].find(TEX_TYPE(p)) == opaqueProjectiles[MDL_TYPE(p)].end())
			opaqueProjectiles[MDL_TYPE(p)][TEX_TYPE(p)] = ProjectileSet();

		opaqueProjectiles[MDL_TYPE(p)][TEX_TYPE(p)].insert(p);
	}
}

void IModelDrawer::ProjectileDestroyed(const CProjectile* p)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::ProjectileDestroyed] id=%d", p->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (p->model) {
		opaqueProjectiles[MDL_TYPE(p)][TEX_TYPE(p)].erase(p);
	}
}




void IModelDrawer::Draw()
{
	// TODO: LuaUnitRendering bypass
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::Draw] frame=%d", gs->frameNum);
	#endif

	// opaque objects by <modelType, textureType>
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		PushRenderState(modelType);

		UnitRenderBinIt uIt = opaqueUnits[modelType].begin();
		FeatureRenderBinIt fIt = opaqueFeatures[modelType].begin();
		ProjectileRenderBinIt pIt = opaqueProjectiles[modelType].begin();

		for (; uIt != opaqueUnits[modelType].end(); uIt++) { DrawModels(opaqueUnits[modelType][uIt->first]); }
		for (; fIt != opaqueFeatures[modelType].end(); fIt++) { DrawModels(opaqueFeatures[modelType][fIt->first]); }
		for (; pIt != opaqueProjectiles[modelType].end(); pIt++) { DrawModels(opaqueProjectiles[modelType][pIt->first]); }

		uIt = cloakedUnits[modelType].begin();
		fIt = cloakedFeatures[modelType].begin();
		pIt = cloakedProjectiles[modelType].begin();

		for (; uIt != cloakedUnits[modelType].end(); uIt++) { DrawModels(cloakedUnits[modelType][uIt->first]); }
		for (; fIt != cloakedFeatures[modelType].end(); fIt++) { DrawModels(cloakedFeatures[modelType][fIt->first]); }
		for (; pIt != cloakedProjectiles[modelType].end(); pIt++) { DrawModels(cloakedProjectiles[modelType][pIt->first]); }

		PopRenderState(modelType);
	}

	// cloaked objects by <modelType, textureType> (TODO: above-/below-water separation, etc.)
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		PushRenderState(modelType);

		UnitRenderBinIt uIt = cloakedUnits[modelType].begin();
		FeatureRenderBinIt fIt = cloakedFeatures[modelType].begin();
		ProjectileRenderBinIt pIt = cloakedProjectiles[modelType].begin();

		for (; uIt != cloakedUnits[modelType].end(); uIt++) { DrawModels(cloakedUnits[modelType][uIt->first]); }
		for (; fIt != cloakedFeatures[modelType].end(); fIt++) { DrawModels(cloakedFeatures[modelType][fIt->first]); }
		for (; pIt != cloakedProjectiles[modelType].end(); pIt++) { DrawModels(cloakedProjectiles[modelType][pIt->first]); }

		uIt = cloakedUnits[modelType].begin();
		fIt = cloakedFeatures[modelType].begin();
		pIt = cloakedProjectiles[modelType].begin();

		for (; uIt != cloakedUnits[modelType].end(); uIt++) { DrawModels(cloakedUnits[modelType][uIt->first]); }
		for (; fIt != cloakedFeatures[modelType].end(); fIt++) { DrawModels(cloakedFeatures[modelType][fIt->first]); }
		for (; pIt != cloakedProjectiles[modelType].end(); pIt++) { DrawModels(cloakedProjectiles[modelType][pIt->first]); }

		PopRenderState(modelType);
	}
}


void IModelDrawer::DrawModels(const UnitSet& models) 
{
	for (UnitSetIt uIt = models.begin(); uIt != models.end(); uIt++) {
		DrawModel(*uIt);
	}
}

void IModelDrawer::DrawModels(const FeatureSet& models)
{
	for (FeatureSetIt fIt = models.begin(); fIt != models.end(); fIt++) {
		DrawModel(*fIt);
	}
}

void IModelDrawer::DrawModels(const ProjectileSet& models)
{
	for (ProjectileSetIt pIt = models.begin(); pIt != models.end(); pIt++) {
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
			if (drawMode == CGame::gameShadowDraw && shadowHandler->drawShadows) {
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



void CModelDrawerGLSL::DrawModels(const UnitSet& units)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModels(units)]");
	#endif

	IModelDrawer::DrawModels(units);
}

void CModelDrawerGLSL::DrawModels(const FeatureSet& features)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::DrawModels(features)]");
	#endif

	IModelDrawer::DrawModels(features);
}

void CModelDrawerGLSL::DrawModels(const ProjectileSet& projectiles)
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
		// case MODELTYPE_OBJ: {
		// } break;
		default: {
		} break;
	}


	// shadowHandler may have been deleted, so
	// update the program if in the shadow pass
	if (game->gameDrawMode == CGame::gameShadowDraw) {
		shaders[modelType][game->gameDrawMode] =
			shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
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
		// case MODELTYPE_OBJ: {
		// } break;
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
