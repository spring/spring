/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ModelDrawer.h"
#include "Game/Game.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"

/// If set to true, additional stuff will be rendered
#define MODEL_DRAWER_DEBUG_RENDERING 0

#define LOG_SECTION_MODEL_DRAWER "ModelDrawer"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_MODEL_DRAWER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_MODEL_DRAWER


IModelDrawer* modelDrawer = NULL;

IModelDrawer* IModelDrawer::GetInstance()
{
	if (modelDrawer == NULL) {
		if (!globalRendering->haveARB && !globalRendering->haveGLSL) {
			modelDrawer = new CModelDrawerFFP("[CModelDrawerFFP]", 314596, false);
		} else {
			if (globalRendering->haveGLSL) {
				modelDrawer = new CModelDrawerGLSL("[CModelDrawerGLSL]", 314596, false);
			} else {
				if (globalRendering->haveARB) {
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

	LOG_L(L_DEBUG, "[%s] this=%p, name=%s, order=%d, synced=%d",
			__FUNCTION__, this, name.c_str(), order, synced);
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

	LOG_L(L_DEBUG, "[%s]", __FUNCTION__);
}



void IModelDrawer::RenderUnitCreated(const CUnit* u, int cloaked)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, u->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (u->model) {
		if (cloaked) {
			// units can start life cloaked
			cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		} else {
			opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
		}
	}
	#endif
}

void IModelDrawer::RenderUnitDestroyed(const CUnit* u)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, u->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (u->model) {
		cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
	}
	#endif
}


void IModelDrawer::RenderUnitCloakChanged(const CUnit* u, int cloaked)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, u->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (u->model) {
		if(cloaked) {
			cloakedModelRenderers[MDL_TYPE(u)]->AddUnit(u);
			opaqueModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		}
		else {
			opaqueModelRenderers[MDL_TYPE(u)]->AddUnit(u);
			cloakedModelRenderers[MDL_TYPE(u)]->DelUnit(u);
		}
	}
	#endif
}


void IModelDrawer::RenderFeatureCreated(const CFeature* f)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, f->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (f->model) {
		opaqueModelRenderers[MDL_TYPE(f)]->AddFeature(f);
	}
	#endif
}

void IModelDrawer::RenderFeatureDestroyed(const CFeature* f)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, f->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (f->model) {
		opaqueModelRenderers[MDL_TYPE(f)]->DelFeature(f);
	}
	#endif
}


void IModelDrawer::RenderProjectileCreated(const CProjectile* p)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, p->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (p->model) {
		opaqueModelRenderers[MDL_TYPE(p)]->AddProjectile(p);
	}
	#endif
}

void IModelDrawer::RenderProjectileDestroyed(const CProjectile* p)
{
	LOG_L(L_DEBUG, "[%s] id=%d", __FUNCTION__, p->id);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
	if (p->model) {
		opaqueModelRenderers[MDL_TYPE(p)]->DelProjectile(p);
	}
	#endif
}




void IModelDrawer::Draw()
{
	// TODO: write LuaUnitRendering bypass
	// (need to know gameDrawMode and if we
	// are drawing opaque or cloaked models)
	LOG_L(L_DEBUG, "[%s] frame=%d", __FUNCTION__, gs->frameNum);

	#if (MODEL_DRAWER_DEBUG_RENDERING)
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
	#endif
}






CModelDrawerFFP::CModelDrawerFFP(const std::string& name, int order, bool synced): IModelDrawer(name, order, synced)
{
	LOG_L(L_DEBUG, "[%s] name=%s, order=%d, synced=%d",
			__FUNCTION__, name.c_str(), order, synced);
}



CModelDrawerARB::CModelDrawerARB(const std::string& name, int order, bool synced): IModelDrawer(name, order, synced)
{
	LOG_L(L_DEBUG, "[%s] name=%s, order=%d, synced=%d",
			__FUNCTION__, name.c_str(), order, synced);

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
	LOG_L(L_DEBUG, "[%s]", __FUNCTION__);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType] = std::vector<Shader::IProgramObject*>();
		shaders[modelType].resize(CGame::gameRefractionDraw + 1, NULL);

		for (int drawMode = CGame::gameNotDrawing; drawMode < CGame::gameRefractionDraw + 1; drawMode++) {
			if (drawMode == CGame::gameShadowDraw && shadowHandler->shadowsLoaded) {
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
	LOG_L(L_DEBUG, "[%s] name=%s, order=%d, synced=%d",
			__FUNCTION__, name.c_str(), order, synced);

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
	LOG_L(L_DEBUG, "[%s]", __FUNCTION__);

	shaderHandler->CreateProgramObject(GetName(), "$DUMMY-GLSL$", false);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		shaders[modelType] = std::vector<Shader::IProgramObject*>();
		shaders[modelType].resize(CGame::gameRefractionDraw + 1, NULL);

		for (int drawMode = CGame::gameNotDrawing; drawMode < CGame::gameRefractionDraw + 1; drawMode++) {
			if (drawMode == CGame::gameShadowDraw && shadowHandler->shadowsLoaded) {
				shaders[modelType][drawMode] = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);
			} else {
				shaders[modelType][drawMode] = shaderHandler->GetProgramObject(GetName(), "$DUMMY-GLSL$");
			}
		}
	}

	return false;
}



void CModelDrawerGLSL::PushDrawState(int modelType)
{
	LOG_L(L_DEBUG, "[%s] modelType=%d, gameDrawMode=%d",
			__FUNCTION__, modelType, game->gameDrawMode);

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
	LOG_L(L_DEBUG, "[%s] modelType=%d, gameDrawMode=%d",
			__FUNCTION__, modelType, game->gameDrawMode);

	shaders[modelType][game->gameDrawMode]->Disable();
}
