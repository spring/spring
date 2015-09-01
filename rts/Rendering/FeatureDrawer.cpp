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
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"

#define DRAW_QUAD_SIZE 32

#define ALPHA_OPAQUE 0.99f


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

	drawQuadsX = mapDims.mapx/DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy/DRAW_QUAD_SIZE;
	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);
	modelRenderers.resize(drawQuadsX * drawQuadsY);
	for (auto &qmr: modelRenderers) {
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			qmr.first[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
		}
		qmr.second = false;
	}
}


CFeatureDrawer::~CFeatureDrawer()
{
	eventHandler.RemoveClient(this);

	for (auto &qmr: modelRenderers) {
		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			delete qmr.first[modelType];
		}
	}

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

	if (feature->objectDef->decalDef.useGroundDecal)
		groundDecals->RemoveSolidObject(f, NULL);
}


void CFeatureDrawer::FeatureMoved(const CFeature* feature, const float3& oldpos)
{
	CFeature* f = const_cast<CFeature*>(feature);

	UpdateDrawQuad(f);
}

void CFeatureDrawer::UpdateDrawQuad(CFeature* feature)
{
	const int oldDrawQuad = feature->drawQuad;
	if (oldDrawQuad < -1)
		return;

	int newDrawQuadX = feature->pos.x / DRAW_QUAD_SIZE / SQUARE_SIZE;
	int newDrawQuadY = feature->pos.z / DRAW_QUAD_SIZE / SQUARE_SIZE;
	newDrawQuadX = Clamp(newDrawQuadX, 0, drawQuadsX - 1);
	newDrawQuadY = Clamp(newDrawQuadY, 0, drawQuadsY - 1);
	const int newDrawQuad = newDrawQuadY * drawQuadsX + newDrawQuadX;

	if (oldDrawQuad == newDrawQuad)
		return;

	//TODO check if out of map features get drawn, when the camera is outside of the map
	//     (q: does DrawGround render the border quads in such cases?)
	assert(oldDrawQuad < drawQuadsX * drawQuadsY);
	assert(newDrawQuad < drawQuadsX * drawQuadsY && newDrawQuad >= 0);

	if (feature->model) {
		if (oldDrawQuad >= 0) {
			modelRenderers[oldDrawQuad].first[MDL_TYPE(feature)]->DelFeature(feature);
		}

		modelRenderers[newDrawQuad].first[MDL_TYPE(feature)]->AddFeature(feature);
	}

	feature->drawQuad = newDrawQuad;
}


void CFeatureDrawer::Update()
{
	for (CFeature* f: unsortedFeatures) {
		UpdateDrawPos(f);
	}
}


inline void CFeatureDrawer::UpdateDrawPos(CFeature* f)
{
	const float time = globalRendering->timeOffset;

	f->drawPos    =    f->pos + (f->speed * time);
	f->drawMidPos = f->midPos + (f->speed * time);
}


void CFeatureDrawer::Draw()
{
	SCOPED_TIMER("FeatureDrawer::Draw");
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
	GetVisibleFeatures(0, true);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		modelRenderers[0].first[modelType]->PushRenderState();
		DrawOpaqueFeatures(modelType);
		modelRenderers[0].first[modelType]->PopRenderState();
	}

	unitDrawer->CleanUpUnitDrawing(false);

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

void CFeatureDrawer::DrawOpaqueFeatures(int modelType)
{
	SCOPED_TIMER("FeatureDrawer::DrawOpaqueFeatures");
	for (auto &qmr: modelRenderers) {
		if (!qmr.second)
			continue;

		auto &featureBin = qmr.first[modelType]->GetFeatureBin();

		for (auto &fs: featureBin) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(fs.first);
			}

			for (CFeature* f: fs.second) {
				if (f->drawAlpha != ALPHA_OPAQUE)
					continue;

				DrawFeatureNow(f, f->drawAlpha);
			}
		}
	}
}

bool CFeatureDrawer::DrawFeatureNow(const CFeature* feature, float alpha)
{
	if (feature->IsInVoid()) { return false; }
	if (!camera->InView(feature->drawMidPos, feature->drawRadius)) { return false; }
	if (!feature->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView) { return false; }

	const float sqDist = (feature->pos - camera->GetPos()).SqLength();
	const float farLength = feature->sqRadius * unitDrawer->unitDrawDistSqr;
	const float sqFadeDistEnd = featureDrawDistance * featureDrawDistance;

	if (sqDist >= std::min(farLength, sqFadeDistEnd)) return false;

	glPushMatrix();
	glMultMatrixf(feature->GetTransformMatrixRef());

	unitDrawer->SetTeamColour(feature->team, alpha);

	if (!(feature->luaDraw && eventHandler.DrawFeature(feature))) {
		feature->model->DrawStatic();
	}

	glPopMatrix();

	return true;
}






void CFeatureDrawer::DrawFadeFeatures(bool noAdvShading)
{
	SCOPED_TIMER("FeatureDrawer::DrawFadeFeatures");
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
				modelRenderers[0].first[modelType]->PushRenderState();
				DrawFadeFeaturesHelper(modelType);
				modelRenderers[0].first[modelType]->PopRenderState();
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
}

void CFeatureDrawer::DrawFadeFeaturesHelper(int modelType)
{
	for (auto &qmr: modelRenderers) {
		if (!qmr.second)
			continue;

		auto &featureBin = qmr.first[modelType]->GetFeatureBin();

		for (auto &fs: featureBin) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(fs.first);
			}

			DrawFadeFeaturesSet(fs.second, modelType);
		}
	}
}

void CFeatureDrawer::DrawFadeFeaturesSet(const FeatureSet& fadeFeatures, int modelType)
{
	for (CFeature* f: fadeFeatures) {
		if (f->drawAlpha == ALPHA_OPAQUE)
			continue;

		const float cols[] = {1.0f, 1.0f, 1.0f, f->drawAlpha};

		if (modelType != MODELTYPE_3DO) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
		}

		// hack, sorting objects by distance would look better
		glAlphaFunc(GL_GREATER, f->drawAlpha / 2.0f);
		glColor4fv(cols);

		DrawFeatureNow(f, f->drawAlpha);
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
}



class CFeatureQuadDrawer : public CReadMap::IQuadDrawer {
public:
	int drawQuadsX;

	bool drawReflection, drawRefraction;
	bool farFeatures;

	float sqFadeDistBegin;
	float sqFadeDistEnd;

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
		auto &qmr = featureDrawer->modelRenderers[dq];
		qmr.second = true;
		const float3 cameraPos = camera->GetPos();
		for(int i = 0; i < MODELTYPE_OTHER; ++i) {
			auto &featureBin = qmr.first[i]->GetFeatureBinMutable();
			for (auto &fs: featureBin) {
				for (CFeature* f: fs.second) {
					assert(dq == f->drawQuad);

					if (f->IsInVoid())
						continue;

					assert(f->def->drawType == DRAWTYPE_MODEL);

					if (gu->spectatingFullView || f->IsInLosForAllyTeam(gu->myAllyTeam)) {
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
								if (camera->InView(f->drawMidPos, f->drawRadius))
									f->drawAlpha = ALPHA_OPAQUE;
							} else if (sqDist < sqFadeDistE) {
								if (camera->InView(f->drawMidPos, f->drawRadius))
									f->drawAlpha = 1.0f - (sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB);
							}
						} else {
							if (farFeatures) {
								farTextureHandler->Queue(f);
							}
						}
					}
				}
			}
		}
	}
};



void CFeatureDrawer::GetVisibleFeatures(int extraSize, bool drawFar)
{
	SCOPED_TIMER("FeatureDrawer::GetVisibleFeatures");
	CFeatureQuadDrawer drawer;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection = water->DrawReflectionPass();
	drawer.drawRefraction = water->DrawRefractionPass();
	drawer.sqFadeDistEnd = featureDrawDistance * featureDrawDistance;
	drawer.sqFadeDistBegin = featureFadeDistance * featureFadeDistance;
	drawer.farFeatures = drawFar;

	for (auto &qmr: modelRenderers) {
		qmr.second = false;
	}

	readMap->GridVisibility(camera, DRAW_QUAD_SIZE, featureDrawDistance, &drawer, extraSize);
}

void CFeatureDrawer::PostLoad()
{
	drawQuadsX = mapDims.mapx/DRAW_QUAD_SIZE;
	drawQuadsY = mapDims.mapy/DRAW_QUAD_SIZE;
}
