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
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/UnitDrawerState.hpp"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"

#define DRAW_QUAD_SIZE 32

CONFIG(bool, ShowRezBars).defaultValue(true).headlessValue(false);

CONFIG(float, FeatureDrawDistance)
.defaultValue(6000.0f)
.minimumValue(0.0f)
.description("Maximum distance at which features will be drawn.");

CONFIG(float, FeatureFadeDistance)
.defaultValue(4500.0f)
.minimumValue(0.0f)
.description("Distance at which features will begin to fade from view.");


CFeatureDrawer* featureDrawer = nullptr;

// can not be a CFeatureDrawer; destruction in global
// scope might happen after ~EventHandler (referenced
// by ~EventClient)
static uint8_t featureDrawerMem[sizeof(CFeatureDrawer)];


static bool SetFeatureDrawAlpha(
	CFeature* f,
	const CCamera* cam,
	float sqFadeDistMin = -1.0f,
	float sqFadeDistMax = -1.0f
) {
	// always reset outside ::Draw
	if (cam == nullptr)
		return (f->drawAlpha = 0.0f, false);

	const float sqrCamDist = (f->pos - cam->GetPos()).SqLength();
	const float farTexDist = Square(f->GetDrawRadius() * unitDrawer->unitDrawDist);

	// first test if feature should be rendered as a fartex
	if (sqrCamDist >= farTexDist)
		return false;

	// special case for non-fading features
	if (!f->alphaFade)
		return (f->drawAlpha = 1.0f, true);

	const float sqFadeDistBeg = mix(sqFadeDistMin, farTexDist * (sqFadeDistMin / sqFadeDistMax), (farTexDist < sqFadeDistMax));
	const float sqFadeDistEnd = mix(sqFadeDistMax, farTexDist                                  , (farTexDist < sqFadeDistMax));

	// draw feature as normal, no fading
	if (sqrCamDist < sqFadeDistBeg)
		return (f->drawAlpha = 1.0f, true);

	// otherwise save it for the fade-pass
	if (sqrCamDist < sqFadeDistEnd)
		return (f->drawAlpha = 1.0f - ((sqrCamDist - sqFadeDistBeg) / (sqFadeDistEnd - sqFadeDistBeg)), true);

	// do not draw at all, fully faded
	return false;
}




void CFeatureDrawer::InitStatic() {
	if (featureDrawer == nullptr)
		featureDrawer = new (featureDrawerMem) CFeatureDrawer();

	featureDrawer->Init();
}
void CFeatureDrawer::KillStatic(bool reload) {
	featureDrawer->Kill();

	if (reload)
		return;

	spring::SafeDestruct(featureDrawer);
	memset(featureDrawerMem, 0, sizeof(featureDrawerMem));
}

void CFeatureDrawer::Init()
{
	eventHandler.AddClient(this);
	configHandler->NotifyOnChange(this, {"FeatureDrawDistance", "FeatureFadeDistance"});

	LuaObjectDrawer::ReadLODScales(LUAOBJ_FEATURE);

	// shared with UnitDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();

	drawForward = true;
	drawDeferred = (geomBuffer->Valid());

	inAlphaPass = false;
	inShadowPass = false;

	drawQuadsX = mapDims.mapx / DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy / DRAW_QUAD_SIZE;
	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);

	modelRenderers.resize(drawQuadsX * drawQuadsY);
	camVisDrawFrames.fill(0);

	for (RdrContProxy& p: modelRenderers) {
		p.SetLastDrawFrame(0);
		(p.GetRenderer(MODELTYPE_3DO)).Init();
		(p.GetRenderer(MODELTYPE_S3O)).Init();
		(p.GetRenderer(MODELTYPE_ASS)).Init();
	}

	for (auto& camVisibleQuad: camVisibleQuads) {
		camVisibleQuad.clear();
		camVisibleQuad.reserve(256);
	}
}

void CFeatureDrawer::Kill()
{
	configHandler->RemoveObserver(this);
	eventHandler.RemoveClient(this);
	autoLinkedEvents.clear();

	// reuse inner containers when reloading
	// modelRenderers.clear();
	for (RdrContProxy& p: modelRenderers) {
		(p.GetRenderer(MODELTYPE_3DO)).Kill();
		(p.GetRenderer(MODELTYPE_S3O)).Kill();
		(p.GetRenderer(MODELTYPE_ASS)).Kill();
	}

	unsortedFeatures.clear();

	geomBuffer = nullptr;
}


void CFeatureDrawer::ConfigNotify(const std::string& key, const std::string& value)
{
	switch (hashStringLower(key.c_str())) {
		case hashStringLower("FeatureDrawDistance"): {
			featureDrawDistance = std::strtof(value.c_str(), nullptr);
		} break;
		case hashStringLower("FeatureFadeDistance"): {
			featureFadeDistance = std::strtof(value.c_str(), nullptr);
		} break;
		default: {
		} break;
	}

	featureDrawDistance = std::max(               0.0f, featureDrawDistance);
	featureFadeDistance = std::max(               0.0f, featureFadeDistance);
	featureFadeDistance = std::min(featureDrawDistance, featureFadeDistance);

	LOG_L(L_INFO, "[FeatureDrawer::%s] {draw,fade}distance set to {%f,%f}", __func__, featureDrawDistance, featureFadeDistance);
}


void CFeatureDrawer::RenderFeatureCreated(const CFeature* feature)
{
	if (feature->def->drawType != DRAWTYPE_MODEL)
		return;

	CFeature* f = const_cast<CFeature*>(feature);

	SetFeatureDrawAlpha(f, nullptr);
	UpdateDrawQuad(f);

	unsortedFeatures.push_back(f);
}



void CFeatureDrawer::RenderFeatureDestroyed(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);

	if (f->def->drawType == DRAWTYPE_MODEL)
		spring::VectorErase(unsortedFeatures, f);

	if (f->model != nullptr && f->drawQuad >= 0)
		modelRenderers[f->drawQuad].GetRenderer(MDL_TYPE(f)).DelObject(f);

	LuaObjectDrawer::SetObjectLOD(f, LUAOBJ_FEATURE, 0);
}


void CFeatureDrawer::FeatureMoved(const CFeature* feature, const float3& oldpos)
{
	UpdateDrawQuad(const_cast<CFeature*>(feature));
}

void CFeatureDrawer::UpdateDrawQuad(CFeature* feature)
{
	const int newDrawQuadX = Clamp(int(feature->pos.x / SQUARE_SIZE / DRAW_QUAD_SIZE), 0, drawQuadsX - 1);
	const int newDrawQuadY = Clamp(int(feature->pos.z / SQUARE_SIZE / DRAW_QUAD_SIZE), 0, drawQuadsY - 1);
	const int newDrawQuad  = newDrawQuadY * drawQuadsX + newDrawQuadX;
	const int oldDrawQuad  = feature->drawQuad;

	if (oldDrawQuad == newDrawQuad)
		return;

	//TODO check if out of map features get drawn, when the camera is outside of the map
	//     (q: does DrawGround render the border quads in such cases?)
	assert(oldDrawQuad < drawQuadsX * drawQuadsY);
	assert(newDrawQuad < drawQuadsX * drawQuadsY && newDrawQuad >= 0);

	if (feature->model != nullptr) {
		if (oldDrawQuad >= 0)
			modelRenderers[oldDrawQuad].GetRenderer(MDL_TYPE(feature)).DelObject(feature);

		modelRenderers[newDrawQuad].GetRenderer(MDL_TYPE(feature)).AddObject(feature);
	}

	feature->drawQuad = newDrawQuad;
}


void CFeatureDrawer::Update()
{
	for (CFeature* f: unsortedFeatures) {
		UpdateDrawPos(f);
		SetFeatureDrawAlpha(f, nullptr);
	}
}


inline void CFeatureDrawer::UpdateDrawPos(CFeature* f)
{
	f->drawPos    = f->GetDrawPos(globalRendering->timeOffset);
	f->drawMidPos = f->GetMdlDrawMidPos();
}


void CFeatureDrawer::Draw()
{
	// mark all features (in the quads we can see) with a FD_*_FLAG value
	// the passes below will ignore any features whose marker is not valid
	GetVisibleFeatures(CCameraHandler::GetActiveCamera(), 0, true);

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !water->DrawReflectionPass() && !water->DrawRefractionPass())
		LuaObjectDrawer::DrawDeferredPass(LUAOBJ_FEATURE);

	// now do the regular forward pass
	if (drawForward)
		DrawOpaquePass(false);

	farTextureHandler->Draw();
}

void CFeatureDrawer::DrawOpaquePass(bool deferredPass)
{
	unitDrawer->SetupOpaqueDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		unitDrawer->PushModelRenderState(modelType);
		DrawOpaqueFeatures(modelType);
		unitDrawer->PopModelRenderState(modelType);
	}

	unitDrawer->ResetOpaqueDrawing(deferredPass);

	// draw all custom'ed features that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_FEATURE, deferredPass);
}

void CFeatureDrawer::DrawOpaqueFeatures(int modelType)
{
	const auto& quads = camVisibleQuads[(CCameraHandler::GetActiveCamera())->GetCamType()];

	for (int quad: quads) {
		const auto& mdlRenderProxy = modelRenderers[quad];

		if (mdlRenderProxy.GetLastDrawFrame() < globalRendering->drawFrame)
			continue;

		const auto& mdlRenderer = mdlRenderProxy.GetRenderer(modelType);
		// const auto& featureBinKeys = mdlRenderer.GetObjectBinKeys();

		for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
			CUnitDrawer::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

			for (CFeature* f: mdlRenderer.GetObjectBin(i)) {
				// fartex, opaque, shadow are allowed here
				switch (f->drawFlag) {
					case CFeature::FD_NODRAW_FLAG: {                              continue; } break;
					case CFeature::FD_ALPHAF_FLAG: {                              continue; } break;
					case CFeature::FD_FARTEX_FLAG: { farTextureHandler->Queue(f); continue; } break;
					default: {} break;
				}

				// test this before the LOD calls (for consistency with UD)
				if (!CanDrawFeature(f))
					continue;

				if ( inShadowPass && LuaObjectDrawer::AddShadowMaterialObject(f, LUAOBJ_FEATURE))
					continue;
				if (!inShadowPass && LuaObjectDrawer::AddOpaqueMaterialObject(f, LUAOBJ_FEATURE))
					continue;

				unitDrawer->SetTeamColour(f->team);

				DrawFeatureDefTrans(f, false, false);
			}
		}
	}
}

bool CFeatureDrawer::CanDrawFeature(const CFeature* feature) const
{
	if (feature->noDraw)
		return false;
	if (feature->IsInVoid())
		return false;
	if (!feature->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return false;

	// same cutoff as AT; set during SP too
	if (feature->drawAlpha <= 0.1f)
		return false;

	// either PLAYER or SHADOW or UWREFL
	const CCamera* cam = CCameraHandler::GetActiveCamera();

	return (cam->InView(feature->drawMidPos, feature->GetDrawRadius()));
}



inline void CFeatureDrawer::DrawFeatureModel(const CFeature* feature, bool noLuaCall) {
	if (!noLuaCall && feature->luaDraw && eventHandler.DrawFeature(feature))
		return;

	feature->localModel.Draw();
}



void CFeatureDrawer::SetFeatureLuaTrans(const CFeature* feature, bool lodCall) {
	IUnitDrawerState* state = unitDrawer->GetDrawerState(DRAWER_STATE_SEL);
	LocalModel* model = const_cast<LocalModel*>(&feature->localModel);

	// use whatever model transform is on the stack
	model->UpdatePieceMatrices(gs->frameNum);
	state->SetMatrices(GL::GetMatrix(GL_MODELVIEW), model->GetPieceMatrices());
}

void CFeatureDrawer::SetFeatureDefTrans(const CFeature* feature, bool lodCall) {
	IUnitDrawerState* state = unitDrawer->GetDrawerState(DRAWER_STATE_SEL);
	LocalModel* model = const_cast<LocalModel*>(&feature->localModel);

	model->UpdatePieceMatrices(gs->frameNum);
	state->SetMatrices(feature->GetTransformMatrixRef(), model->GetPieceMatrices());
}



void CFeatureDrawer::DrawFeatureLuaTrans(const CFeature* feature, bool lodCall, bool noLuaCall) {
	SetFeatureLuaTrans(feature, lodCall);
	DrawFeatureModel(feature, noLuaCall);
}

void CFeatureDrawer::DrawFeatureDefTrans(const CFeature* feature, bool lodCall, bool noLuaCall) {
	SetFeatureDefTrans(feature, lodCall);
	DrawFeatureModel(feature, noLuaCall);
}




void CFeatureDrawer::PushIndividualState(const CFeature* feature, bool deferredPass)
{
	unitDrawer->SetupOpaqueDrawing(false);
	unitDrawer->PushModelRenderState(feature);
	unitDrawer->SetTeamColour(feature->team);
}

void CFeatureDrawer::PopIndividualState(const CFeature* feature, bool deferredPass)
{
	unitDrawer->PopModelRenderState(feature);
	unitDrawer->ResetOpaqueDrawing(false);
}


void CFeatureDrawer::DrawIndividualDefTrans(const CFeature* feature, bool noLuaCall)
{
	if (LuaObjectDrawer::DrawSingleObject(feature, LUAOBJ_FEATURE /*, noLuaCall*/))
		return;

	// set the full default state
	PushIndividualState(feature, false);
	DrawFeatureDefTrans(feature, false, noLuaCall);
	PopIndividualState(feature, false);
}

void CFeatureDrawer::DrawIndividualLuaTrans(const CFeature* feature, bool noLuaCall)
{
	if (LuaObjectDrawer::DrawSingleObjectLuaTrans(feature, LUAOBJ_FEATURE /*, noLuaCall*/))
		return;

	PushIndividualState(feature, false);
	DrawFeatureLuaTrans(feature, false, noLuaCall);
	PopIndividualState(feature, false);
}



void CFeatureDrawer::DrawAlphaPass(bool aboveWater)
{
	inAlphaPass = true;

	{
		assert((unitDrawer->GetWantedDrawerState(true))->CanDrawAlpha());
		unitDrawer->SetupAlphaDrawing(false, aboveWater);
		glAttribStatePtr->PushBits(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glAttribStatePtr->EnableDepthMask();

		// needed for now; not always called directly after Draw()
		GetVisibleFeatures(CCameraHandler::GetActiveCamera(), 0, true);

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			unitDrawer->PushModelRenderState(modelType);
			DrawAlphaFeatures(modelType);
			unitDrawer->PopModelRenderState(modelType);
		}

		glAttribStatePtr->PopBits();
		unitDrawer->ResetAlphaDrawing(false);
	}

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_FEATURE, false);

	inAlphaPass = false;
}

void CFeatureDrawer::DrawAlphaFeatures(int modelType)
{
	const auto& quads = camVisibleQuads[(CCameraHandler::GetActiveCamera())->GetCamType()];

	for (int quad: quads) {
		const auto& mdlRenderProxy = modelRenderers[quad];

		if (mdlRenderProxy.GetLastDrawFrame() < globalRendering->drawFrame)
			continue;

		const auto& mdlRenderer = mdlRenderProxy.GetRenderer(modelType);
		// const auto& featureBinKeys = mdlRenderer.GetObjectBinKeys();

		for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
			CUnitDrawer::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

			for (CFeature* f: mdlRenderer.GetObjectBin(i)) {
				// only alpha is allowed here
				switch (f->drawFlag) {
					case CFeature::FD_NODRAW_FLAG: { continue; } break;
					case CFeature::FD_OPAQUE_FLAG: { continue; } break;
					case CFeature::FD_SHADOW_FLAG: { continue; } break;
					case CFeature::FD_FARTEX_FLAG: { continue; } break;
					default: {} break;
				}

				if (!CanDrawFeature(f))
					continue;

				if (LuaObjectDrawer::AddAlphaMaterialObject(f, LUAOBJ_FEATURE))
					continue;

				unitDrawer->SetTeamColour(f->team, float2(f->drawAlpha, 1.0f));
				unitDrawer->SetAlphaTest({f->drawAlpha * 0.5f, 1.0f, 0.0f, 0.0f}); // test > (alpha/2)
				DrawFeatureDefTrans(f, false, false);
			}
		}
	}

	unitDrawer->SetAlphaTest({0.0f, 0.0f, 0.0f, 1.0f}); // no test
}




void CFeatureDrawer::DrawShadowPass()
{
	inShadowPass = true;

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Enable();
	po->SetUniformMatrix4fv(1, false, shadowHandler.GetShadowViewMatrix());
	po->SetUniformMatrix4fv(2, false, shadowHandler.GetShadowProjMatrix());

	glAttribStatePtr->PolygonOffset(1.0f, 1.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);

	{
		assert((CCameraHandler::GetActiveCamera())->GetCamType() == CCamera::CAMTYPE_SHADOW);

		// mark all features (in the quads we can see) with FD_SHADOW_FLAG
		// the pass below will ignore any features whose tag != this value
		GetVisibleFeatures(CCameraHandler::GetActiveCamera(), 0, false);

		// needed for 3do models (else they will use any currently bound texture)
		// note: texture0 is by default a 1x1 texture with rgba(0,0,0,255)
		// (we are just interested in the 255 alpha here)
		glBindTexture(GL_TEXTURE_2D, 0);

		// need the alpha-mask for transparent features; threshold set in ShadowHandler
		glAttribStatePtr->PushColorBufferBit();

		// 3DO's have clockwise-wound faces and
		// (usually) holes, so disable backface
		// culling for them
		glAttribStatePtr->DisableCullFace();
		DrawOpaqueFeatures(MODELTYPE_3DO);
		glAttribStatePtr->EnableCullFace();

		for (int modelType = MODELTYPE_S3O; modelType < MODELTYPE_OTHER; modelType++) {
			DrawOpaqueFeatures(modelType);
		}

		glAttribStatePtr->PopBits();
	}

	po->Disable();
	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawShadowMaterialObjects(LUAOBJ_FEATURE, false);

	inShadowPass = false;
}



class CFeatureQuadDrawer : public CReadMap::IQuadDrawer {
public:
	CFeatureQuadDrawer(int _numQuadsX): numQuadsX(_numQuadsX) {}

	void ResetState() override { numQuadsX = 0; }

	void DrawQuad(int x, int y) override {
		camQuads.push_back(y * numQuadsX + x);

		// used so we do not iterate over non-visited renderers (in any pass)
		rdrProxies[y * numQuadsX + x].SetLastDrawFrame(globalRendering->drawFrame);
	}

	std::vector<int>& GetCamQuads() { return camQuads; }
	std::vector<CFeatureDrawer::RdrContProxy>& GetRdrProxies() { return rdrProxies; }

private:
	std::vector<int> camQuads;
	std::vector<CFeatureDrawer::RdrContProxy> rdrProxies;

	int numQuadsX;
};



void CFeatureDrawer::FlagVisibleFeatures(
	const CCamera* cam,
	bool drawShadowPass,
	bool drawReflection,
	bool drawRefraction,
	bool drawFarFeatures
) {
	const auto& quads = camVisibleQuads[cam->GetCamType()];

	const float sqFadeDistBegin = featureFadeDistance * featureFadeDistance;
	const float sqFadeDistEnd = featureDrawDistance * featureDrawDistance;

	const CCamera* playerCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	for (int quad: quads) {
		auto& mdlRenderProxy = featureDrawer->modelRenderers[quad];

		for (int i = 0; i < MODELTYPE_OTHER; ++i) {
			const auto& mdlRenderer = mdlRenderProxy.GetRenderer(i);
			// const auto& featureBinKeys = mdlRenderer.GetObjectBinKeys();

			for (unsigned int j = 0, n = mdlRenderer.GetNumObjectBins(); j < n; j++) {
				for (CFeature* f: mdlRenderer.GetObjectBin(j)) {
					assert(quad == f->drawQuad);

					// clear marker; will be set at most once below
					f->SetDrawFlag(CFeature::FD_NODRAW_FLAG);

					if (f->noDraw)
						continue;
					if (f->IsInVoid())
						continue;

					assert(f->def->drawType == DRAWTYPE_MODEL);

					if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
						continue;


					if (drawShadowPass) {
						if (SetFeatureDrawAlpha(f, playerCam, sqFadeDistBegin, sqFadeDistEnd)) {
							// no shadows for fully alpha-faded features from player's POV
							f->UpdateTransform(f->drawPos, false);
							f->SetDrawFlag(CFeature::FD_SHADOW_FLAG);
						}
						continue;
					}

					if (drawRefraction && !f->IsInWater())
						continue;

					if (drawReflection && !CUnitDrawer::ObjectVisibleReflection(f->drawMidPos, cam->GetPos(), f->GetDrawRadius()))
						continue;


					if (SetFeatureDrawAlpha(f, cam, sqFadeDistBegin, sqFadeDistEnd)) {
						f->UpdateTransform(f->drawPos, false);
						f->SetDrawFlag(mix(int(CFeature::FD_OPAQUE_FLAG), int(CFeature::FD_ALPHAF_FLAG), f->drawAlpha < 1.0f));
						continue;
					}

					// note: it looks pretty bad to first alpha-fade and then
					// draw a fully *opaque* fartex, so restrict impostors to
					// non-fading features
					f->SetDrawFlag(CFeature::FD_FARTEX_FLAG * drawFarFeatures * (!f->alphaFade));
				}
			}
		}
	}
}

void CFeatureDrawer::GetVisibleFeatures(CCamera* cam, int extraSize, bool drawFar)
{
	// should only ever be called for the first three types
	assert(cam->GetCamType() < camVisDrawFrames.size());

	// check if we already did a pass for this camera-type
	// (e.g. water refraction and the standard opaque pass
	// use CAMTYPE_PLAYER with equal state)
	// if so, just re-flag those features (fast) since the
	// refraction pass needs to skip features that are not
	// in water, etc.
	if (camVisDrawFrames[cam->GetCamType()] >= globalRendering->drawFrame) {
		FlagVisibleFeatures(cam, inShadowPass, water->DrawReflectionPass(), water->DrawRefractionPass(), drawFar);
		return;
	}

	camVisDrawFrames[cam->GetCamType()] = globalRendering->drawFrame;

	camVisibleQuads[cam->GetCamType()].clear();
	camVisibleQuads[cam->GetCamType()].reserve(256);

	{
		CFeatureQuadDrawer drawer(drawQuadsX);

		(drawer.GetCamQuads()).swap(camVisibleQuads[cam->GetCamType()]);
		(drawer.GetRdrProxies()).swap(featureDrawer->modelRenderers);

		cam->CalcFrustumLines(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
		readMap->GridVisibility(cam, &drawer, featureDrawDistance, DRAW_QUAD_SIZE, extraSize);

		(drawer.GetCamQuads()).swap(camVisibleQuads[cam->GetCamType()]);
		(drawer.GetRdrProxies()).swap(featureDrawer->modelRenderers);
	}

	FlagVisibleFeatures(cam, inShadowPass, water->DrawReflectionPass(), water->DrawRefractionPass(), drawFar);
}
