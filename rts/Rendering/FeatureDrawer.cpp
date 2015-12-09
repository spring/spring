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
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Util.h"

#define DRAW_QUAD_SIZE 32

#define FD_ALPHA_OPAQUE  0.99f
#define FD_ALPHA_FARTEX -1.00f


CONFIG(bool, ShowRezBars).defaultValue(true).headlessValue(false);

CONFIG(float, FeatureDrawDistance)
.defaultValue(6000.0f)
.minimumValue(0.0f)
.description("Maximum distance at which features will be drawn.");

CONFIG(float, FeatureFadeDistance)
.defaultValue(4500.0f)
.minimumValue(0.0f)
.description("Distance at which features will begin to fade from view.");

CFeatureDrawer* featureDrawer = NULL;

/******************************************************************************/

CR_BIND(CFeatureDrawer, )

CR_REG_METADATA(CFeatureDrawer, (
	CR_IGNORED(unsortedFeatures),
	CR_IGNORED(drawQuadsX),
	CR_IGNORED(drawQuadsY),
	CR_IGNORED(farDist),
	CR_IGNORED(featureDrawDistance),
	CR_IGNORED(featureFadeDistance),
	CR_IGNORED(modelRenderers),

	CR_POSTLOAD(PostLoad)
))

/******************************************************************************/



CFeatureDrawer::CFeatureDrawer(): CEventClient("[CFeatureDrawer]", 313373, false)
{
	eventHandler.AddClient(this);

	LuaObjectDrawer::ReadLODScales(LUAOBJ_FEATURE);

	// shared with UnitDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();

	drawForward = true;
	drawDeferred = (geomBuffer->Valid());

	drawQuadsX = mapDims.mapx / DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy / DRAW_QUAD_SIZE;
	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);

	modelRenderers.resize(drawQuadsX * drawQuadsY);
}


CFeatureDrawer::~CFeatureDrawer()
{
	eventHandler.RemoveClient(this);
	modelRenderers.clear();
}



void CFeatureDrawer::RenderFeatureCreated(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);
	texturehandlerS3O->UpdateDraw();

	if (feature->def->drawType == DRAWTYPE_MODEL) {
		f->drawQuad = -1;
		f->drawAlpha = 0.0f;
		UpdateDrawQuad(f);
		unsortedFeatures.push_back(f);
	}
}



void CFeatureDrawer::RenderFeatureDestroyed(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);

	if (f->def->drawType == DRAWTYPE_MODEL) {
		VectorErase(unsortedFeatures, f);
	}
	if (f->model && f->drawQuad >= 0) {
		modelRenderers[f->drawQuad].rendererTypes[MDL_TYPE(f)]->DelFeature(f);
		f->drawQuad = -1;
	}

	if (feature->objectDef->decalDef.useGroundDecal)
		groundDecals->RemoveSolidObject(f, NULL);

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
			modelRenderers[oldDrawQuad].rendererTypes[MDL_TYPE(feature)]->DelFeature(feature);
		}

		modelRenderers[newDrawQuad].rendererTypes[MDL_TYPE(feature)]->AddFeature(feature);
	}

	feature->drawQuad = newDrawQuad;
}


void CFeatureDrawer::Update()
{
	for (CFeature* f: unsortedFeatures) {
		UpdateDrawPos(f);
		f->drawAlpha = 0.0f;
	}

	GetVisibleFeatures(0, true);
}


inline void CFeatureDrawer::UpdateDrawPos(CFeature* f)
{
	const float time = globalRendering->timeOffset;

	f->drawPos    =    f->pos + (f->speed * time);
	f->drawMidPos = f->midPos + (f->speed * time);
}


void CFeatureDrawer::Draw()
{
	ISky::SetupFog();

	if (infoTextureHandler->IsEnabled()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0.0f, 0.0f);

		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !water->DrawReflectionPass() && !water->DrawRefractionPass()) {
		LuaObjectDrawer::DrawDeferredPass(nullptr, LUAOBJ_FEATURE);
	}

	// now do the regular forward pass
	if (drawForward) {
		DrawOpaquePass(false, water->DrawReflectionPass(), water->DrawRefractionPass());
	}

	farTextureHandler->Draw();

	if (infoTextureHandler->IsEnabled()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
}

void CFeatureDrawer::DrawOpaquePass(bool deferredPass, bool, bool)
{
	unitDrawer->SetupForUnitDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		// arbitrarily pick first proxy to prepare state
		modelRenderers[0].rendererTypes[modelType]->PushRenderState();
		DrawOpaqueFeatures(modelType, LuaObjectDrawer::GetDrawPassOpaqueMat());
		modelRenderers[0].rendererTypes[modelType]->PopRenderState();
	}

	unitDrawer->CleanUpUnitDrawing(deferredPass);

	// draw all custom'ed features that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_FEATURE, deferredPass);
}

void CFeatureDrawer::DrawOpaqueFeatures(int modelType, int luaMatType)
{
	const bool shadowPass = (luaMatType == LuaObjectDrawer::GetDrawPassShadowMat());

	for (const auto& mdlRenderProxy: modelRenderers) {
		if (mdlRenderProxy.lastDrawFrame < globalRendering->drawFrame)
			continue;

		const auto& featureBin = mdlRenderProxy.rendererTypes[modelType]->GetFeatureBin();

		for (const auto& binElem: featureBin) {
			CUnitDrawer::BindModelTypeTexture(modelType, binElem.first);

			for (CFeature* f: binElem.second) {
				// if <f> is supposed to be drawn faded, skip it during this pass
				if (f->drawAlpha < FD_ALPHA_OPAQUE) {
					// if it's supposed to be drawn as a far texture and we're not
					// inside a shadow pass, queue it.
					if (f->drawAlpha <= FD_ALPHA_FARTEX && !shadowPass)
						farTextureHandler->Queue(f);

					continue;
				}

				// test this before the LOD calls (for consistency with UD)
				if (!CanDrawFeature(f))
					continue;

				if ( shadowPass && LuaObjectDrawer::AddShadowMaterialObject(f, LUAOBJ_FEATURE))
					continue;
				if (!shadowPass && LuaObjectDrawer::AddOpaqueMaterialObject(f, LUAOBJ_FEATURE))
					continue;

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

	if (feature->fade) {
		const float sqDist = (feature->pos - camera->GetPos()).SqLength();
		const float farLength = feature->sqRadius * unitDrawer->unitDrawDistSqr;
		const float sqFadeDistEnd = featureDrawDistance * featureDrawDistance;

		if (sqDist >= std::min(farLength, sqFadeDistEnd))
			return false;
	}

	return (camera->InView(feature->drawMidPos, feature->drawRadius));
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
	// TODO:
	//   move this out of UnitDrawer(State), maybe to ModelDrawer
	//   in general FeatureDrawer should make no UnitDrawer calls
	unitDrawer->SetTeamColour(feature->team, feature->drawAlpha);

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




bool CFeatureDrawer::DrawIndividualPreCommon(const CFeature* feature)
{
	const bool origDrawDebug = globalRendering->drawdebug;
	globalRendering->drawdebug = false;

	// set the full default state
	unitDrawer->SetupForUnitDrawing(false);
	modelRenderers[0].rendererTypes[MDL_TYPE(feature)]->PushRenderState();

	CUnitDrawer::BindModelTypeTexture(feature);
	unitDrawer->SetTeamColour(feature->team, feature->drawAlpha);

	return origDrawDebug;
}

void CFeatureDrawer::DrawIndividualPostCommon(const CFeature* feature, bool origDrawDebug)
{
	modelRenderers[0].rendererTypes[MDL_TYPE(feature)]->PopRenderState();
	unitDrawer->CleanUpUnitDrawing(false);

	globalRendering->drawdebug = origDrawDebug;
}


void CFeatureDrawer::DrawIndividual(const CFeature* feature, bool noLuaCall)
{
	const bool origDrawDebug = DrawIndividualPreCommon(feature);

	if (!LuaObjectDrawer::DrawSingleObject(feature, LUAOBJ_FEATURE /*, noLuaCall*/)) {
		DrawFeature(feature, 0, 0, false, noLuaCall);
	}

	DrawIndividualPostCommon(feature, origDrawDebug);
}

void CFeatureDrawer::DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall)
{
	const bool origDrawDebug = DrawIndividualPreCommon(feature);

	if (!LuaObjectDrawer::DrawSingleObjectNoTrans(feature, LUAOBJ_FEATURE /*, noLuaCall*/)) {
		DrawFeatureNoTrans(feature, 0, 0, false, noLuaCall);
	}

	DrawIndividualPostCommon(feature, origDrawDebug);
}




static void SetFeatureAlphaMaterialFFP(const CFeature* f, bool set)
{
	const float cols[] = {1.0f, 1.0f, 1.0f, f->drawAlpha};

	if (set) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
	}

	// hack, sorting objects by distance would look better
	glAlphaFunc(GL_GREATER, f->drawAlpha * 0.5f);
	glColor4fv(cols);
}


void CFeatureDrawer::DrawFadeFeatures(bool noAdvShading)
{
	const bool oldAdvShading = unitDrawer->UseAdvShading();

	{
		unitDrawer->SetUseAdvShading(unitDrawer->UseAdvShading() && !noAdvShading);

		if (unitDrawer->UseAdvShading()) {
			unitDrawer->SetupForUnitDrawing(false);
		} else {
			unitDrawer->SetupForGhostDrawing();
		}

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);

		ISky::SetupFog();

		{
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				modelRenderers[0].rendererTypes[modelType]->PushRenderState();
				DrawFadeFeaturesHelper(modelType, LuaObjectDrawer::GetDrawPassAlphaMat());
				modelRenderers[0].rendererTypes[modelType]->PopRenderState();
			}
		}

		glDisable(GL_FOG);

		glPopAttrib();

		if (unitDrawer->UseAdvShading()) {
			unitDrawer->CleanUpUnitDrawing(false);
		} else {
			unitDrawer->CleanUpGhostDrawing();
		}

		unitDrawer->SetUseAdvShading(oldAdvShading);
	}

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_FEATURE, false);
}

void CFeatureDrawer::DrawFadeFeaturesHelper(int modelType, int luaMatType)
{
	for (const auto& mdlRenderProxy: modelRenderers) {
		if (mdlRenderProxy.lastDrawFrame < globalRendering->drawFrame)
			continue;

		const auto& featureBin = mdlRenderProxy.rendererTypes[modelType]->GetFeatureBin();

		for (const auto& binElem: featureBin) {
			CUnitDrawer::BindModelTypeTexture(modelType, binElem.first);
			DrawFadeFeaturesSet(binElem.second, modelType, luaMatType);
		}
	}
}

void CFeatureDrawer::DrawFadeFeaturesSet(const FeatureSet& fadeFeatures, int modelType, int luaMatType)
{
	for (CFeature* f: fadeFeatures) {
		// if <f> is not supposed to be drawn faded, skip it during this pass
		if (f->drawAlpha >= FD_ALPHA_OPAQUE || f->drawAlpha <= (1.0f - FD_ALPHA_OPAQUE))
			continue;

		if (!CanDrawFeature(f))
			continue;

		if (LuaObjectDrawer::AddAlphaMaterialObject(f, LUAOBJ_FEATURE))
			continue;

		SetFeatureAlphaMaterialFFP(f, modelType != MODELTYPE_3DO);
		DrawFeature(f, 0, 0, false, false);
	}
}




void CFeatureDrawer::DrawShadowPass()
{
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL);

	po->Enable();

	{
		// note: for the shadow-pass, we want to make sure
		// out-of-view features casting frustum-intersecting
		// shadows are still rendered, but this is expensive
		// and does not make much difference
		//
		// GetVisibleFeatures(1, false);

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
		DrawOpaqueFeatures(MODELTYPE_3DO, LuaObjectDrawer::GetDrawPassShadowMat());
		glEnable(GL_CULL_FACE);

		for (int modelType = MODELTYPE_S3O; modelType < MODELTYPE_OTHER; modelType++) {
			DrawOpaqueFeatures(modelType, LuaObjectDrawer::GetDrawPassShadowMat());
		}

		glPopAttrib();
		glDisable(GL_TEXTURE_2D);
	}

	po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawShadowMaterialObjects(LUAOBJ_FEATURE, false);
}



class CFeatureQuadDrawer : public CReadMap::IQuadDrawer {
public:
	int drawQuadsX;

	bool drawReflection;
	bool drawRefraction;
	bool farFeatures;

	float sqFadeDistBegin;
	float sqFadeDistEnd;

public:
	void ResetState() {
		drawQuadsX = 0;

		drawReflection = false;
		drawRefraction = false;
		farFeatures = false;

		sqFadeDistBegin = 0.0f;
		sqFadeDistEnd   = 0.0f;
	}

	void DrawQuad(int x, int y) {
		const float3 cameraPos = camera->GetPos();

		auto& mdlRenderProxy = featureDrawer->modelRenderers[y * drawQuadsX + x];

		if (mdlRenderProxy.lastDrawFrame >= globalRendering->drawFrame)
			return;

		mdlRenderProxy.lastDrawFrame = globalRendering->drawFrame;

		for (int i = 0; i < MODELTYPE_OTHER; ++i) {
			auto& featureBin = mdlRenderProxy.rendererTypes[i]->GetFeatureBinMutable();

			for (auto& binElem: featureBin) {
				for (CFeature* f: binElem.second) {
					assert((y * drawQuadsX + x) == f->drawQuad);

					if (f->noDraw)
						continue;
					if (f->IsInVoid())
						continue;

					assert(f->def->drawType == DRAWTYPE_MODEL);

					if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
						continue;

					if (drawReflection) {
						float3 zeroPos;

						if (f->midPos.y < 0.0f) {
							zeroPos = f->midPos;
						} else {
							const float dif = f->midPos.y - cameraPos.y;
							zeroPos =
								cameraPos * (f->midPos.y / dif) +
								f->midPos * (-cameraPos.y / dif);
						}
						if (CGround::GetApproximateHeight(zeroPos.x, zeroPos.z, false) > f->drawRadius) {
							continue;
						}
					}
					if (drawRefraction) {
						if (f->pos.y > 0.0f)
							continue;
					}
					if (!f->fade) {
						f->drawAlpha = FD_ALPHA_OPAQUE;
						continue;
					}

					const float sqDist = (f->pos - cameraPos).SqLength();
					const float farLength = f->sqRadius * unitDrawer->unitDrawDistSqr;

					if (sqDist < farLength) {
						float sqFadeDistE;
						float sqFadeDistB;

						if (farLength < sqFadeDistEnd) {
							sqFadeDistE = farLength;
							sqFadeDistB = farLength * sqFadeDistBegin / sqFadeDistEnd;
						} else {
							sqFadeDistE = sqFadeDistEnd;
							sqFadeDistB = sqFadeDistBegin;
						}

						if (sqDist < sqFadeDistB) {
							// draw feature as normal, no fading
							f->drawAlpha = FD_ALPHA_OPAQUE;
						} else if (sqDist < sqFadeDistE) {
							// otherwise save it for the fade-pass
							f->drawAlpha = 1.0f - (sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB);
						}
					} else if (farFeatures) {
						f->drawAlpha = FD_ALPHA_FARTEX;
					}
				}
			}
		}
	}
};



void CFeatureDrawer::GetVisibleFeatures(int extraSize, bool drawFar)
{
	CFeatureQuadDrawer drawer;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection = water->DrawReflectionPass();
	drawer.drawRefraction = water->DrawRefractionPass();
	drawer.sqFadeDistEnd = featureDrawDistance * featureDrawDistance;
	drawer.sqFadeDistBegin = featureFadeDistance * featureFadeDistance;
	drawer.farFeatures = drawFar;

	readMap->GridVisibility(nullptr, DRAW_QUAD_SIZE, featureDrawDistance, &drawer, extraSize);
}

void CFeatureDrawer::PostLoad()
{
	drawQuadsX = mapDims.mapx/DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy/DRAW_QUAD_SIZE;
}
