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

#define ALPHA_OPAQUE 0.99f
#define ALPHA_FAR -1.0f


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


static void erase(std::vector<CFeature*>& v, CFeature* f)
{
	auto it = std::find(v.begin(), v.end(), f);
	if (it != v.end()) {
		*it = v.back();
		v.pop_back();
	}
}


void CFeatureDrawer::RenderFeatureDestroyed(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);

	if (f->def->drawType == DRAWTYPE_MODEL) {
		erase(unsortedFeatures, f);
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

	unitDrawer->SetupForUnitDrawing(false);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		// arbitrarily pick first proxy to prepare state
		// TODO: also add a deferred pass for features
		modelRenderers[0].rendererTypes[modelType]->PushRenderState();
		DrawOpaqueFeatures(modelType, (water->DrawReflectionPass())? LUAMAT_OPAQUE_REFLECT: LUAMAT_OPAQUE);
		modelRenderers[0].rendererTypes[modelType]->PopRenderState();
	}

	unitDrawer->CleanUpUnitDrawing(false);

	LuaObjectDrawer::SetGlobalDrawPassLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_FEATURE, false);

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

void CFeatureDrawer::DrawOpaqueFeatures(int modelType, int luaMatType)
{
	for (const auto& mdlRenderProxy: modelRenderers) {
		if (mdlRenderProxy.lastDrawFrame < globalRendering->drawFrame)
			continue;

		const auto& featureBin = mdlRenderProxy.rendererTypes[modelType]->GetFeatureBin();

		for (const auto& binElem: featureBin) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(binElem.first);
			}

			for (CFeature* f: binElem.second) {
				LuaObjectMaterialData* matData = f->GetLuaMaterialData();

				// if <f> is supposed to be drawn faded, skip it during this pass
				if (f->drawAlpha < ALPHA_OPAQUE) {
					// if it's supposed to be drawn as a far texture and we're not
					// during a shadow pass, queue it.
					if (f->drawAlpha < 0.0f && luaMatType != LUAMAT_SHADOW) {
						farTextureHandler->Queue(f);
					}
					continue;
				}


				if (matData->AddObjectForLOD(f, LUAOBJ_FEATURE, LuaMatType(luaMatType), camera->ProjectedDistance(f->pos)))
					continue;

				DrawFeatureNoLists(f, f->drawAlpha);
			}
		}
	}
}

bool CFeatureDrawer::CanDrawFeature(const CFeature* feature) const
{
	if (feature->IsInVoid())
		return false;
	if (!feature->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return false;

	const float sqDist = (feature->pos - camera->GetPos()).SqLength();
	const float farLength = feature->sqRadius * unitDrawer->unitDrawDistSqr;
	const float sqFadeDistEnd = featureDrawDistance * featureDrawDistance;

	if (sqDist >= std::min(farLength, sqFadeDistEnd))
		return false;

	return (camera->InView(feature->drawMidPos, feature->drawRadius));
}


void CFeatureDrawer::DrawFeatureNoLists(const CFeature* feature, float alpha)
{
	DrawFeatureWithLists(feature, 0, 0, alpha);
}

void CFeatureDrawer::DrawFeatureWithLists(const CFeature* feature, unsigned int preList, unsigned int postList, float alpha)
{
	if (!CanDrawFeature(feature))
		return;

	glPushMatrix();
	glMultMatrixf(feature->GetTransformMatrixRef());

	// TODO: move this out of UnitDrawer(State)
	unitDrawer->SetTeamColour(feature->team, alpha);

	if (preList != 0) {
		glCallList(preList);
	}

	if (!(feature->luaDraw && eventHandler.DrawFeature(feature))) {
		feature->model->DrawStatic();
	}

	if (postList != 0) {
		glCallList(postList);
	}

	glPopMatrix();
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
				DrawFadeFeaturesHelper(modelType, (water->DrawReflectionPass())? LUAMAT_ALPHA_REFLECT: LUAMAT_ALPHA);
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

	LuaObjectDrawer::SetGlobalDrawPassLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_FEATURE, false);
}

void CFeatureDrawer::DrawFadeFeaturesHelper(int modelType, int luaMatType)
{
	for (const auto& mdlRenderProxy: modelRenderers) {
		if (mdlRenderProxy.lastDrawFrame < globalRendering->drawFrame)
			continue;

		const auto& featureBin = mdlRenderProxy.rendererTypes[modelType]->GetFeatureBin();

		for (const auto& binElem: featureBin) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(binElem.first);
			}

			DrawFadeFeaturesSet(binElem.second, modelType, luaMatType);
		}
	}
}

void CFeatureDrawer::DrawFadeFeaturesSet(const FeatureSet& fadeFeatures, int modelType, int luaMatType)
{
	for (CFeature* f: fadeFeatures) {
		LuaObjectMaterialData* matData = f->GetLuaMaterialData();

		// if <f> is not supposed to be drawn faded, skip it during this pass
		if (f->drawAlpha >= ALPHA_OPAQUE || f->drawAlpha <= (1.0f - ALPHA_OPAQUE))
			continue;

		if (matData->AddObjectForLOD(f, LUAOBJ_FEATURE, LuaMatType(luaMatType), camera->ProjectedDistance(f->pos)))
			continue;

		const float cols[] = {1.0f, 1.0f, 1.0f, f->drawAlpha};

		if (modelType != MODELTYPE_3DO) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
		}

		// hack, sorting objects by distance would look better
		glAlphaFunc(GL_GREATER, f->drawAlpha / 2.0f);
		glColor4fv(cols);

		DrawFeatureNoLists(f, f->drawAlpha);
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
		DrawOpaqueFeatures(MODELTYPE_3DO, LUAMAT_SHADOW);
		glEnable(GL_CULL_FACE);

		for (int modelType = MODELTYPE_S3O; modelType < MODELTYPE_OTHER; modelType++) {
			DrawOpaqueFeatures(modelType, LUAMAT_SHADOW);
		}

		glPopAttrib();
		glDisable(GL_TEXTURE_2D);
	}

	po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);

	LuaObjectDrawer::SetGlobalDrawPassLODFactor(LUAOBJ_FEATURE);
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
		const int dq = y * drawQuadsX + x;
		const float3 cameraPos = camera->GetPos();

		auto& mdlRenderProxy = featureDrawer->modelRenderers[dq];

		if (mdlRenderProxy.lastDrawFrame >= globalRendering->drawFrame)
			return;

		mdlRenderProxy.lastDrawFrame = globalRendering->drawFrame;

		for (int i = 0; i < MODELTYPE_OTHER; ++i) {
			auto& featureBin = mdlRenderProxy.rendererTypes[i]->GetFeatureBinMutable();

			for (auto& binElem: featureBin) {
				for (CFeature* f: binElem.second) {
					assert(dq == f->drawQuad);

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
							f->drawAlpha = ALPHA_OPAQUE;
						} else if (sqDist < sqFadeDistE) {
							// otherwise save it for the fade-pass
							f->drawAlpha = 1.0f - (sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB);
						}
					} else if (farFeatures) {
						f->drawAlpha = ALPHA_FAR;
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

	readMap->GridVisibility(camera, DRAW_QUAD_SIZE, featureDrawDistance, &drawer, extraSize);
}

void CFeatureDrawer::PostLoad()
{
	drawQuadsX = mapDims.mapx/DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy/DRAW_QUAD_SIZE;
}
