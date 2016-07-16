/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDrawer.h"

#include "Game/Camera.h"
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
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/UnitDrawerState.hpp"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Util.h"

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



static bool SetFeatureDrawAlpha(
	CFeature* f,
	const CCamera* cam,
	float sqFadeDistMin = -1.0f,
	float sqFadeDistMax = -1.0f
) {
	// always reset
	f->drawAlpha = 0.0f;

	if (cam == nullptr)
		return false;

	// special case for non-fading features
	if (!f->alphaFade) {
		f->drawAlpha = 1.0f;
		return true;
	}

	const float sqDist = (f->pos - cam->GetPos()).SqLength();
	const float farLength = f->sqRadius * unitDrawer->unitDrawDistSqr;

	// if true, feature will become a fartex
	if (sqDist >= farLength)
		return false;

	float sqFadeDistB = sqFadeDistMin;
	float sqFadeDistE = sqFadeDistMax;

	if (farLength < sqFadeDistMax) {
		sqFadeDistE = farLength;
		sqFadeDistB = farLength * (sqFadeDistMin / sqFadeDistMax);
	}

	if (sqDist < sqFadeDistB) {
		// draw feature as normal, no fading
		f->drawAlpha = 1.0f;
		return true;
	}

	if (sqDist < sqFadeDistE) {
		// otherwise save it for the fade-pass
		f->drawAlpha = 1.0f - ((sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB));
		return true;
	}

	return false;
}



static const void SetFeatureAlphaMatSSP(const CFeature* f) { glAlphaFunc(GL_GREATER, f->drawAlpha * 0.5f); }
static const void SetFeatureAlphaMatFFP(const CFeature* f)
{
	const float cols[] = {1.0f, 1.0f, 1.0f, f->drawAlpha};

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
	glColor4fv(cols);

	// hack, sorting objects by distance would look better
	glAlphaFunc(GL_GREATER, f->drawAlpha * 0.5f);
}


typedef const void (*SetFeatureAlphaMatFunc)(const CFeature*);

static const SetFeatureAlphaMatFunc setFeatureAlphaMatFuncs[] = {
	SetFeatureAlphaMatSSP,
	SetFeatureAlphaMatFFP,
};



CFeatureDrawer* featureDrawer = nullptr;


/******************************************************************************/



CFeatureDrawer::CFeatureDrawer(): CEventClient("[CFeatureDrawer]", 313373, false)
{
	eventHandler.AddClient(this);

	LuaObjectDrawer::ReadLODScales(LUAOBJ_FEATURE);

	// shared with UnitDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();

	drawForward = true;
	drawDeferred = (geomBuffer->Valid());

	inAlphaPass = false;
	inShadowPass = false;
	ffpAlphaMat = false;

	drawQuadsX = mapDims.mapx / DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy / DRAW_QUAD_SIZE;
	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);

	modelRenderers.resize(drawQuadsX * drawQuadsY);
	camVisDrawFrames.fill(0);

	for (unsigned int n = 0; n < camVisibleQuads.size(); n++) {
		camVisibleQuads[n].reserve(256);
	}
}


CFeatureDrawer::~CFeatureDrawer()
{
	eventHandler.RemoveClient(this);

	modelRenderers.clear();
}



void CFeatureDrawer::RenderFeatureCreated(const CFeature* feature)
{
	if (feature->def->drawType != DRAWTYPE_MODEL)
		return;

	CFeature* f = const_cast<CFeature*>(feature);

	// otherwise UpdateDrawQuad returns early
	f->drawQuad = -1;

	SetFeatureDrawAlpha(f, nullptr);
	UpdateDrawQuad(f);

	unsortedFeatures.push_back(f);
}



void CFeatureDrawer::RenderFeatureDestroyed(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);

	if (f->def->drawType == DRAWTYPE_MODEL) {
		VectorErase(unsortedFeatures, f);
	}
	if (f->model && f->drawQuad >= 0) {
		modelRenderers[f->drawQuad].GetRenderer(MDL_TYPE(f))->DelFeature(f);
		f->drawQuad = -1;
	}

	LuaObjectDrawer::SetObjectLOD(f, LUAOBJ_FEATURE, 0);
}


void CFeatureDrawer::FeatureMoved(const CFeature* feature, const float3& oldpos)
{
	UpdateDrawQuad(const_cast<CFeature*>(feature));
}

void CFeatureDrawer::UpdateDrawQuad(CFeature* feature)
{
	const int oldDrawQuad = feature->drawQuad;

	if (oldDrawQuad < -1)
		return;

	const int newDrawQuadX = Clamp(int(feature->pos.x / SQUARE_SIZE / DRAW_QUAD_SIZE), 0, drawQuadsX - 1);
	const int newDrawQuadY = Clamp(int(feature->pos.z / SQUARE_SIZE / DRAW_QUAD_SIZE), 0, drawQuadsY - 1);
	const int newDrawQuad = newDrawQuadY * drawQuadsX + newDrawQuadX;

	if (oldDrawQuad == newDrawQuad)
		return;

	//TODO check if out of map features get drawn, when the camera is outside of the map
	//     (q: does DrawGround render the border quads in such cases?)
	assert(oldDrawQuad < drawQuadsX * drawQuadsY);
	assert(newDrawQuad < drawQuadsX * drawQuadsY && newDrawQuad >= 0);

	if (feature->model) {
		if (oldDrawQuad >= 0) {
			modelRenderers[oldDrawQuad].GetRenderer(MDL_TYPE(feature))->DelFeature(feature);
		}

		modelRenderers[newDrawQuad].GetRenderer(MDL_TYPE(feature))->AddFeature(feature);
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
	SCOPED_GMARKER("CFeatureDrawer::Draw");

	sky->SetupFog();

	// mark all features (in the quads we can see) with a FD_*_FLAG value
	// the passes below will ignore any features whose marker is not valid
	GetVisibleFeatures(CCamera::GetActiveCamera(), 0, true);

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !water->DrawReflectionPass() && !water->DrawRefractionPass()) {
		LuaObjectDrawer::DrawDeferredPass(LUAOBJ_FEATURE);
	}

	// now do the regular forward pass
	if (drawForward) {
		DrawOpaquePass(false, water->DrawReflectionPass(), water->DrawRefractionPass());
	}

	farTextureHandler->Draw();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
}

void CFeatureDrawer::DrawOpaquePass(bool deferredPass, bool, bool)
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
	const auto& quads = camVisibleQuads[(CCamera::GetActiveCamera())->GetCamType()];

	for (int quad: quads) {
		const auto& mdlRenderProxy = modelRenderers[quad];

		if (mdlRenderProxy.GetLastDrawFrame() < globalRendering->drawFrame)
			continue;

		const auto mdlRenderer = mdlRenderProxy.GetRenderer(modelType);
		const auto& featureBin = mdlRenderer->GetFeatureBin();

		for (const auto& binElem: featureBin) {
			CUnitDrawer::BindModelTypeTexture(modelType, binElem.first);

			for (CFeature* f: binElem.second) {
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

				DrawFeature(f, 0, 0, false, false);
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
	const CCamera* cam = CCamera::GetActiveCamera();

	return (cam->InView(feature->drawMidPos, feature->GetDrawRadius()));
}



inline void CFeatureDrawer::DrawFeatureModel(const CFeature* feature, bool noLuaCall) {
	if (!noLuaCall && feature->luaDraw && eventHandler.DrawFeature(feature))
		return;

	feature->localModel.Draw();
}

void CFeatureDrawer::DrawFeatureNoTrans(
	const CFeature* feature,
	unsigned int preList,
	unsigned int postList,
	bool /*lodCall*/,
	bool noLuaCall
) {
	if (preList != 0) {
		glCallList(preList);
	}

	DrawFeatureModel(feature, noLuaCall);

	if (postList != 0) {
		glCallList(postList);
	}
}

void CFeatureDrawer::DrawFeature(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall)
{
	glPushMatrix();
	glMultMatrixf(feature->GetTransformMatrixRef());

	DrawFeatureNoTrans(feature, preList, postList, lodCall, noLuaCall);

	glPopMatrix();
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


void CFeatureDrawer::DrawIndividual(const CFeature* feature, bool noLuaCall)
{
	const bool origDrawDebug = globalRendering->GetSetDrawDebug(false);

	if (!LuaObjectDrawer::DrawSingleObject(feature, LUAOBJ_FEATURE /*, noLuaCall*/)) {
		// set the full default state
		PushIndividualState(feature, false);
		DrawFeature(feature, 0, 0, false, noLuaCall);
		PopIndividualState(feature, false);
	}

	globalRendering->GetSetDrawDebug(origDrawDebug);
}

void CFeatureDrawer::DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall)
{
	const bool origDrawDebug = globalRendering->GetSetDrawDebug(false);

	if (!LuaObjectDrawer::DrawSingleObjectNoTrans(feature, LUAOBJ_FEATURE /*, noLuaCall*/)) {
		PushIndividualState(feature, false);
		DrawFeatureNoTrans(feature, 0, 0, false, noLuaCall);
		PopIndividualState(feature, false);
	}

	globalRendering->GetSetDrawDebug(origDrawDebug);
}



void CFeatureDrawer::DrawAlphaPass()
{
	SCOPED_GMARKER("CFeatureDrawer::DrawAlphaPass");

	inAlphaPass = true;
	ffpAlphaMat = !(unitDrawer->GetWantedDrawerState(true))->CanDrawAlpha();

	{
		unitDrawer->SetupAlphaDrawing(false);

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);

		sky->SetupFog();

		// needed for now; not always called directly after Draw()
		GetVisibleFeatures(CCamera::GetActiveCamera(), 0, true);

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			unitDrawer->PushModelRenderState(modelType);
			DrawAlphaFeatures(modelType);
			unitDrawer->PopModelRenderState(modelType);
		}

		glDisable(GL_FOG);
		glPopAttrib();

		unitDrawer->ResetAlphaDrawing(false);
	}

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_FEATURE, false);

	inAlphaPass = false;
}

void CFeatureDrawer::DrawAlphaFeatures(int modelType)
{
	const auto& quads = camVisibleQuads[(CCamera::GetActiveCamera())->GetCamType()];

	for (int quad: quads) {
		const auto& mdlRenderProxy = modelRenderers[quad];

		if (mdlRenderProxy.GetLastDrawFrame() < globalRendering->drawFrame)
			continue;

		const auto mdlRenderer = mdlRenderProxy.GetRenderer(modelType);
		const auto& featureBin = mdlRenderer->GetFeatureBin();

		for (const auto& binElem: featureBin) {
			CUnitDrawer::BindModelTypeTexture(modelType, binElem.first);

			for (CFeature* f: binElem.second) {
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

				setFeatureAlphaMatFuncs[ffpAlphaMat](f);
				DrawFeature(f, 0, 0, false, false);
			}
		}
	}
}




void CFeatureDrawer::DrawShadowPass()
{
	inShadowPass = true;

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Enable();

	{
		assert((CCamera::GetActiveCamera())->GetCamType() == CCamera::CAMTYPE_SHADOW);

		// mark all features (in the quads we can see) with FD_SHADOW_FLAG
		// the pass below will ignore any features whose tag != this value
		GetVisibleFeatures(CCamera::GetActiveCamera(), 0, false);

		// need the alpha-mask for transparent features
		glEnable(GL_TEXTURE_2D);
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);

		// needed for 3do models (else they will use any currently bound texture)
		// note: texture0 is by default a 1x1 texture with rgba(0,0,0,255)
		// (we are just interested in the 255 alpha here)
		glBindTexture(GL_TEXTURE_2D, 0);

		// 3DO's have clockwise-wound faces and
		// (usually) holes, so disable backface
		// culling for them
		glDisable(GL_CULL_FACE);
		DrawOpaqueFeatures(MODELTYPE_3DO);
		glEnable(GL_CULL_FACE);

		for (int modelType = MODELTYPE_S3O; modelType < MODELTYPE_OTHER; modelType++) {
			DrawOpaqueFeatures(modelType);
		}

		glPopAttrib();
		glDisable(GL_TEXTURE_2D);
	}

	po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawShadowMaterialObjects(LUAOBJ_FEATURE, false);

	inShadowPass = false;
}



class CFeatureQuadDrawer : public CReadMap::IQuadDrawer {
public:
	CFeatureQuadDrawer(int _numQuadsX): numQuadsX(_numQuadsX) {}

	void ResetState() { numQuadsX = 0; }

	void DrawQuad(int x, int y) {
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

	const CCamera* playerCam = CCamera::GetCamera(CCamera::CAMTYPE_PLAYER);

	for (unsigned int n = 0; n < quads.size(); n++) {
		auto& mdlRenderProxy = featureDrawer->modelRenderers[ quads[n] ];

		for (int i = 0; i < MODELTYPE_OTHER; ++i) {
			auto mdlRenderer = mdlRenderProxy.GetRenderer(i);
			auto& featureBin = mdlRenderer->GetFeatureBinMutable();

			for (auto& binElem: featureBin) {
				for (CFeature* f: binElem.second) {
					assert(quads[n] == f->drawQuad);

					// clear marker; will be set at most once below
					f->drawFlag = CFeature::FD_NODRAW_FLAG;

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
							f->drawFlag = CFeature::FD_SHADOW_FLAG;
						}
						continue;
					}

					if (drawRefraction && !f->IsInWater())
						continue;

					if (drawReflection && !CUnitDrawer::ObjectVisibleReflection(f->drawMidPos, cam->GetPos(), f->GetDrawRadius()))
						continue;


					if (SetFeatureDrawAlpha(f, cam, sqFadeDistBegin, sqFadeDistEnd)) {
						f->UpdateTransform(f->drawPos, false);
						f->drawFlag += (CFeature::FD_OPAQUE_FLAG * (f->drawAlpha == 1.0f));
						f->drawFlag += (CFeature::FD_ALPHAF_FLAG * (f->drawAlpha <  1.0f));
						continue;
					}

					// note: it looks pretty bad to first alpha-fade and then
					// draw a fully *opaque* fartex, so restrict impostors to
					// non-fading features
					f->drawFlag = CFeature::FD_FARTEX_FLAG * drawFarFeatures * (!f->alphaFade);
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

		cam->GetFrustumSides(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
		readMap->GridVisibility(cam, &drawer, featureDrawDistance, DRAW_QUAD_SIZE, extraSize);

		(drawer.GetCamQuads()).swap(camVisibleQuads[cam->GetCamType()]);
		(drawer.GetRdrProxies()).swap(featureDrawer->modelRenderers);
	}

	FlagVisibleFeatures(cam, inShadowPass, water->DrawReflectionPass(), water->DrawRefractionPass(), drawFar);
}
