/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "ModelDrawer.hpp"
#include "Game/Game.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"
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

	opaqueModelRenderers.resize(MODELTYPE_OTHER, NULL);
	cloakedModelRenderers.resize(MODELTYPE_OTHER, NULL);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		opaqueModelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
		cloakedModelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
	}

	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::IModelDrawer] this=%p, name=%s, order=%d, synced=%d", this, name.c_str(), order, synced);
	#endif
}

IModelDrawer::~IModelDrawer()
{
	eventHandler.RemoveClient(this);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		delete opaqueModelRenderers[modelType];
		delete cloakedModelRenderers[modelType];
	}

	opaqueModelRenderers.clear();
	cloakedModelRenderers.clear();

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
			cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		} else {
			opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
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
			cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		} else {
			opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		}
	}
}


void IModelDrawer::UnitCloaked(const CUnit* u)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitCloaked] id=%d", u->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (u->model) {
		cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}
}

void IModelDrawer::UnitDecloaked(const CUnit* u)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::UnitDecloaked] id=%d", u->id);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	if (u->model) {
		opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}
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
		opaqueModelRenderers[MDL_TYPE(f)]->AddFeature(f);
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
		opaqueModelRenderers[MDL_TYPE(f)]->DelFeature(f);
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
		opaqueModelRenderers[MDL_TYPE(p)]->AddProjectile(p);
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
		opaqueModelRenderers[MDL_TYPE(p)]->DelProjectile(p);
	}
}




void IModelDrawer::Draw()
{
	// TODO: write LuaUnitRendering bypass
	// (need to know gameDrawMode and if we
	// are drawing opaque or cloaked models)
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[IModelDrawer::Draw] frame=%d", gs->frameNum);
	#endif
	#if (MODEL_DRAWER_DEBUG == 2)
	return;
	#endif

	// opaque objects by <modelType, textureType>
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		PushDrawState(modelType);
		opaqueModelRenderers[modelType]->Draw();
		PopDrawState(modelType);
	}

	// cloaked objects by <modelType, textureType> (TODO: above-/below-water separation, etc.)
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		PushDrawState(modelType);
		cloakedModelRenderers[modelType]->Draw();
		PopDrawState(modelType);
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

CModelDrawerARB::~CModelDrawerARB() {
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType].clear();
	}

	shaderHandler->ReleaseProgramObjects(GetName());
	shaders.clear();
}

bool CModelDrawerARB::LoadModelShaders()
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerARB::LoadModelShaders]");
	#endif

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType] = std::vector<Shader::IProgramObject*>();
		shaders[modelType].resize(CGame::gameRefractionDraw + 1, NULL);

		for (int drawMode = CGame::gameNotDrawing; drawMode < CGame::gameRefractionDraw + 1; drawMode++) {
			if (drawMode == CGame::gameShadowDraw && shadowHandler->drawShadows) {
				shaders[modelType][drawMode] = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
			} else {
				shaders[modelType][drawMode] = shaderHandler->CreateProgramObject(GetName(), "$DUMMY-ARB$", true);
			}
		}
	}

	return false;
}



CModelDrawerGLSL::CModelDrawerGLSL(const std::string& name, int order, bool synced): IModelDrawer(name, order, synced)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::CModelDrawerGLSL] name=%s, order=%d, synced=%d", name.c_str(), order, synced);
	#endif

	LoadModelShaders();
}

CModelDrawerGLSL::~CModelDrawerGLSL() {
	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType].clear();
	}

	shaderHandler->ReleaseProgramObjects(GetName());
	shaders.clear();
}

bool CModelDrawerGLSL::LoadModelShaders()
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::LoadModelShaders]");
	#endif

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType] = std::vector<Shader::IProgramObject*>();
		shaders[modelType].resize(CGame::gameRefractionDraw + 1, NULL);

		for (int drawMode = CGame::gameNotDrawing; drawMode < CGame::gameRefractionDraw + 1; drawMode++) {
			if (drawMode == CGame::gameShadowDraw && shadowHandler->drawShadows) {
				shaders[modelType][drawMode] = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
			} else {
				shaders[modelType][drawMode] = shaderHandler->CreateProgramObject(GetName(), "$DUMMY-GLSL$", false);
			}
		}
	}

	return false;
}



void CModelDrawerGLSL::PushDrawState(int modelType)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::PushDrawState] modelType=%d, gameDrawMode=%d", modelType, game->gameDrawMode);
	#endif

	// shadowHandler may have been deleted, so
	// update the program if in the shadow pass
	if (game->gameDrawMode == CGame::gameShadowDraw) {
		shaders[modelType][game->gameDrawMode] =
			shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
	}

	shaders[modelType][game->gameDrawMode]->Enable();
}

void CModelDrawerGLSL::PopDrawState(int modelType)
{
	#if (MODEL_DRAWER_DEBUG == 1)
	logOutput.Print("[CModelDrawerGLSL::PopDrawState] modelType=%d, gameDrawMode=%d", modelType, game->gameDrawMode);
	#endif

	shaders[modelType][game->gameDrawMode]->Disable();
}
