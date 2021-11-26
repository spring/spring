/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDrawer.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Models/3DModelVAO.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

void CFeatureDrawer::InitStatic()
{
	CModelDrawerBase<CFeatureDrawerData, CFeatureDrawer>::InitStatic();

	LuaObjectDrawer::ReadLODScales(LUAOBJ_FEATURE);

	CFeatureDrawer::InitInstance<CFeatureDrawerFFP >(MODEL_DRAWER_FFP);
	CFeatureDrawer::InitInstance<CFeatureDrawerARB >(MODEL_DRAWER_ARB);
	CFeatureDrawer::InitInstance<CFeatureDrawerGLSL>(MODEL_DRAWER_GLSL);
	CFeatureDrawer::InitInstance<CFeatureDrawerGL4 >(MODEL_DRAWER_GL4);

	SelectImplementation();
}

bool CFeatureDrawer::ShouldDrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction)
{
	assert(f);
	assert(f->model);

	if (f->drawFlag == 0)
		return false;

	if (f->HasDrawFlag(DrawFlags::SO_ALPHAF_FLAG))
		return false;

	if (drawReflection && !f->HasDrawFlag(DrawFlags::SO_REFLEC_FLAG))
		return false;

	if (drawRefraction && !f->HasDrawFlag(DrawFlags::SO_REFRAC_FLAG))
		return false;

	if (f->HasDrawFlag(DrawFlags::SO_FARTEX_FLAG)) {
		farTextureHandler->Queue(f);
		return false;
	}

	if (LuaObjectDrawer::AddOpaqueMaterialObject(f, LUAOBJ_FEATURE))
		return false;

	if (f->noEngineDraw)
		return false;

	return true;
}

bool CFeatureDrawer::ShouldDrawAlphaFeature(CFeature* f, bool drawReflection, bool drawRefraction)
{
	assert(f);
	assert(f->model);

	if (f->drawFlag == 0)
		return false;

	if (f->HasDrawFlag(DrawFlags::SO_OPAQUE_FLAG))
		return false;

	if (drawReflection && !f->HasDrawFlag(DrawFlags::SO_REFLEC_FLAG))
		return false;

	if (drawRefraction && !f->HasDrawFlag(DrawFlags::SO_REFRAC_FLAG))
		return false;

	if (LuaObjectDrawer::AddAlphaMaterialObject(f, LUAOBJ_FEATURE))
		return false;

	if (f->noEngineDraw)
		return false;

	return true;
}

bool CFeatureDrawer::ShouldDrawFeatureShadow(CFeature* f)
{
	assert(f);
	assert(f->model);

	if (!f->HasDrawFlag(DrawFlags::SO_SHADOW_FLAG))
		return false;

	if (LuaObjectDrawer::AddShadowMaterialObject(f, LUAOBJ_FEATURE))
		return false;

	if (f->noEngineDraw)
		return false;

	return true;
}

void CFeatureDrawer::PushIndividualState(const CFeature* feature, bool deferredPass) const
{
	SetupOpaqueDrawing(false);
	CModelDrawerHelper::PushModelRenderState(feature);
	SetTeamColor(feature->team);
}

void CFeatureDrawer::PopIndividualState(const CFeature* feature, bool deferredPass) const
{
	CModelDrawerHelper::PopModelRenderState(feature);
	ResetOpaqueDrawing(false);
}

void CFeatureDrawerBase::Update() const
{
	SCOPED_TIMER("CFeatureDrawerBase::Update");
	modelDrawerData->Update();
}

void CFeatureDrawerLegacy::DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const
{
	if (preList != 0) {
		glCallList(preList);
	}

	DrawFeatureModel(feature, noLuaCall);

	if (postList != 0) {
		glCallList(postList);
	}
}

void CFeatureDrawerLegacy::DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const
{
	glPushMatrix();
	glMultMatrixf(feature->GetTransformMatrixRef());

	DrawFeatureNoTrans(feature, preList, postList, lodCall, noLuaCall);

	glPopMatrix();
}

void CFeatureDrawerLegacy::DrawIndividual(const CFeature* feature, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObject(feature, LUAOBJ_FEATURE /*, noLuaCall*/))
		return;

	// set the full default state
	PushIndividualState(feature, false);
	DrawFeatureTrans(feature, 0, 0, false, noLuaCall);
	PopIndividualState(feature, false);
}

void CFeatureDrawerLegacy::DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObjectNoTrans(feature, LUAOBJ_FEATURE /*, noLuaCall*/))
		return;

	PushIndividualState(feature, false);
	DrawFeatureNoTrans(feature, 0, 0, false, noLuaCall);
	PopIndividualState(feature, false);
}

void CFeatureDrawerLegacy::DrawObjectsShadow(int modelType) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		// only need to bind the atlas once for 3DO's, but KISS
		assert((modelType != MODELTYPE_3DO) || (mdlRenderer.GetObjectBinKey(i) == 0));

		//shadowTexBindFuncs[modelType](textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i)));
		const auto* texMat = textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i));
		CModelDrawerHelper::modelDrawerHelpers[modelType]->BindShadowTex(texMat);

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawFeatureShadow(o);
		}

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}
}

void CFeatureDrawerLegacy::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawOpaqueFeature(o, drawReflection, drawRefraction);
		}
	}
}

void CFeatureDrawerLegacy::DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawAlphaFeature(o, drawReflection, drawRefraction);
		}
	}
}

void CFeatureDrawerLegacy::DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const
{
	if (!ShouldDrawOpaqueFeature(f, drawReflection, drawRefraction))
		return;

	// draw the unit with the default (non-Lua) material
	SetTeamColor(f->team);
	DrawFeatureTrans(f, 0, 0, false, false);
}

void CFeatureDrawerLegacy::DrawAlphaFeature(CFeature* f, bool drawReflection, bool drawRefraction) const
{
	if (!ShouldDrawAlphaFeature(f, drawReflection, drawRefraction))
		return;

	SetTeamColor(f->team, IModelDrawerState::alphaValues.x);
	DrawFeatureTrans(f, 0, 0, false, false);
}

void CFeatureDrawerLegacy::DrawFeatureShadow(CFeature* f) const
{
	if (ShouldDrawFeatureShadow(f))
		DrawFeatureTrans(f, 0, 0, false, false);
}

void CFeatureDrawerLegacy::DrawFeatureModel(const CFeature* feature, bool noLuaCall) const
{
	if (!noLuaCall && feature->luaDraw && eventHandler.DrawFeature(feature))
		return;

	feature->localModel.Draw();
}

void CFeatureDrawerGL4::DrawObjectsShadow(int modelType) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		const auto* texMat = textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i));
		CModelDrawerHelper::modelDrawerHelpers[modelType]->BindShadowTex(texMat);

		const auto& bin = mdlRenderer.GetObjectBin(i);

		for (auto* o : bin) {
			if (!ShouldDrawFeatureShadow(o))
				continue;

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}

	smv.Unbind();
}

void CFeatureDrawerGL4::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	modelDrawerState->SetColorMultiplier();

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			if (!ShouldDrawOpaqueFeature(o, drawReflection, drawRefraction))
				continue;

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);
	}
	smv.Unbind();
}

void CFeatureDrawerGL4::DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	modelDrawerState->SetColorMultiplier(IModelDrawerState::alphaValues.x);
	//main cloaked alpha pass
	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		const auto& bin = mdlRenderer.GetObjectBin(i);

		for (auto* o : bin) {
			if (!ShouldDrawAlphaFeature(o, drawReflection, drawRefraction))
				continue;

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);
	}
	smv.Unbind();
}
