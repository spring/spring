/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDrawer.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaRules.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
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
#include "System/Util.h"

#define DRAW_QUAD_SIZE 32

CONFIG(bool, ShowRezBars).defaultValue(true);

CONFIG(float, FeatureDrawDistance)
.defaultValue(6000.0f)
.minimumValue(0.0f);

CONFIG(float, FeatureFadeDistance)
.defaultValue(4500.0f)
.minimumValue(0.0f);

CFeatureDrawer* featureDrawer = NULL;

/******************************************************************************/

CR_BIND(CFeatureDrawer, );
CR_BIND(CFeatureDrawer::DrawQuad, );

CR_REG_METADATA(CFeatureDrawer, (
	CR_POSTLOAD(PostLoad)
));

/******************************************************************************/



CFeatureDrawer::CFeatureDrawer(): CEventClient("[CFeatureDrawer]", 313373, false)
{
	eventHandler.AddClient(this);

	drawQuadsX = gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY = gs->mapy/DRAW_QUAD_SIZE;
	drawQuads.resize(drawQuadsX * drawQuadsY);
#ifdef USE_GML
	showRezBars = GML::Enabled() && configHandler->GetBool("ShowRezBars");
#endif
	featureDrawDistance = configHandler->GetFloat("FeatureDrawDistance");
	featureFadeDistance = std::min(configHandler->GetFloat("FeatureFadeDistance"), featureDrawDistance);
	opaqueModelRenderers.resize(MODELTYPE_OTHER, NULL);
	cloakedModelRenderers.resize(MODELTYPE_OTHER, NULL);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		opaqueModelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
		cloakedModelRenderers[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
	}
}


CFeatureDrawer::~CFeatureDrawer()
{
	eventHandler.RemoveClient(this);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		delete opaqueModelRenderers[modelType];
		delete cloakedModelRenderers[modelType];
	}

	opaqueModelRenderers.clear();
	cloakedModelRenderers.clear();
}



void CFeatureDrawer::RenderFeatureCreated(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);
	texturehandlerS3O->UpdateDraw();

	if (GML::SimEnabled() && !GML::ShareLists() && feature->model && TEX_TYPE(feature) < 0)
		TEX_TYPE(f) = texturehandlerS3O->LoadS3OTextureNow(feature->model);

	if (feature->def->drawType == DRAWTYPE_MODEL) {
		f->drawQuad = -1;
		UpdateDrawQuad(f);

		unsortedFeatures.insert(f);
	}
}

void CFeatureDrawer::RenderFeatureDestroyed(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);

	if (f->def->drawType == DRAWTYPE_MODEL) {
		unsortedFeatures.erase(f);
	}

	if (f->drawQuad >= 0) {
		DrawQuad* dq = &drawQuads[f->drawQuad];
		dq->features.erase(f);
	}

	if (f->model) {
		opaqueModelRenderers[MDL_TYPE(f)]->DelFeature(f);
		cloakedModelRenderers[MDL_TYPE(f)]->DelFeature(f);
	}

	if (feature->objectDef->decalDef.useGroundDecal)
		groundDecals->RemoveSolidObject(f, NULL);
}


void CFeatureDrawer::RenderFeatureMoved(const CFeature* feature, const float3& oldpos, const float3& newpos)
{
	CFeature* f = const_cast<CFeature*>(feature);

	UpdateDrawQuad(f);
}

void CFeatureDrawer::UpdateDrawQuad(CFeature* feature)
{
	const int oldDrawQuad = feature->drawQuad;
	if (oldDrawQuad >= -1) {
		int newDrawQuadX = feature->pos.x / DRAW_QUAD_SIZE / SQUARE_SIZE;
		int newDrawQuadY = feature->pos.z / DRAW_QUAD_SIZE / SQUARE_SIZE;
		newDrawQuadX = Clamp(newDrawQuadX, 0, drawQuadsX - 1);
		newDrawQuadY = Clamp(newDrawQuadY, 0, drawQuadsY - 1);
		const int newDrawQuad = newDrawQuadY * drawQuadsX + newDrawQuadX;

		if (oldDrawQuad != newDrawQuad) {
			//TODO check if out of map features get drawn, when the camera is outside of the map
			//     (q: does DrawGround render the border quads in such cases?)
			assert(oldDrawQuad < drawQuadsX * drawQuadsY);
			assert(newDrawQuad < drawQuadsX * drawQuadsY);

			if (oldDrawQuad >= 0)
				drawQuads[oldDrawQuad].features.erase(feature);
			drawQuads[newDrawQuad].features.insert(feature);
			feature->drawQuad = newDrawQuad;
		}
	}
}


void CFeatureDrawer::Update()
{
	eventHandler.UpdateDrawFeatures();

	{
		GML_RECMUTEX_LOCK(feat); // Update

		for (std::set<CFeature*>::iterator fsi = unsortedFeatures.begin(); fsi != unsortedFeatures.end(); ++fsi) {
			UpdateDrawPos(*fsi);
		}
	}
}


inline void CFeatureDrawer::UpdateDrawPos(CFeature* f)
{
	const float time = /*!GML::SimEnabled() ?*/ globalRendering->timeOffset /*:
		((float)spring_tomsecs(globalRendering->lastFrameStart) - (float)f->lastFeatUpdate) * globalRendering->weightedSpeedFactor*/;

	f->drawPos    =    f->pos + (f->speed * time);
	f->drawMidPos = f->midPos + (f->speed * time);
}


void CFeatureDrawer::Draw()
{
	ISky::SetupFog();

	GML_RECMUTEX_LOCK(feat); // Draw

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

	if (gd->DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0.0f, 0.0f);

		glBindTexture(GL_TEXTURE_2D, gd->infoTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	unitDrawer->SetupForUnitDrawing();
	GetVisibleFeatures(0, true);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
		opaqueModelRenderers[modelType]->PushRenderState();
		DrawOpaqueFeatures(modelType);
		opaqueModelRenderers[modelType]->PopRenderState();
	}

	unitDrawer->CleanUpUnitDrawing();

	farTextureHandler->Draw();

	if (gd->DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
#ifdef USE_GML
	DrawFeatureStats();
#endif
}

void CFeatureDrawer::DrawOpaqueFeatures(int modelType)
{
	FeatureRenderBin& featureBin = opaqueModelRenderers[modelType]->GetFeatureBinMutable();

	FeatureRenderBin::iterator featureBinIt;
	FeatureSet::iterator featureSetIt;

	for (featureBinIt = featureBin.begin(); featureBinIt != featureBin.end(); ++featureBinIt) {
		if (modelType != MODELTYPE_3DO) {
			texturehandlerS3O->SetS3oTexture(featureBinIt->first);
		}

		FeatureSet& featureSet = featureBinIt->second;

		for (featureSetIt = featureSet.begin(); featureSetIt != featureSet.end(); ) {
			if (!DrawFeatureNow(featureSetIt->first)) {
				featureSetIt = set_erase(featureSet, featureSetIt);
			} else {
				++featureSetIt;
			}
		}
	}
}

#ifdef USE_GML
void CFeatureDrawer::DrawFeatureStats()
{
	if (!drawStat.empty()) {
		if (!water->IsDrawReflection()) {
			for (std::vector<CFeature *>::iterator fi = drawStat.begin(); fi != drawStat.end(); ++fi) {
				DrawFeatureStatBars(*fi);
			}
		}

		drawStat.clear();
	}
}

void CFeatureDrawer::DrawFeatureStatBars(const CFeature* feature)
{
	float3 interPos = feature->pos;
	interPos.y += feature->height + 5.0f;

	glPushMatrix();
	glTranslatef(interPos.x, interPos.y, interPos.z);
	glMultMatrixf(camera->GetBillBoardMatrix());

	const float recl = feature->reclaimLeft;
	const float rezp = feature->resurrectProgress;

	// black background for the bar
	glColor3f(0.0f, 0.0f, 0.0f);
	glRectf(-5.0f, 4.0f, +5.0f, 6.0f);

	// rez/metalbar
	const float rmin = std::min(recl, rezp) * 10.0f;
	if (rmin > 0.0f) {
		glColor3f(1.0f, 0.0f, 1.0f);
		glRectf(-5.0f, 4.0f, rmin - 5.0f, 6.0f);
	}
	if (recl > rezp) {
		float col = 0.8 - 0.3 * recl;
		glColor3f(col, col, col);
		glRectf(rmin - 5.0f, 4.0f, recl * 10.0f - 5.0f, 6.0f);
	}
	if (recl < rezp) {
		glColor3f(0.5f, 0.0f, 1.0f);
		glRectf(rmin - 5.0f, 4.0f, rezp * 10.0f - 5.0f, 6.0f);
	}

	glPopMatrix();
}
#endif

bool CFeatureDrawer::DrawFeatureNow(const CFeature* feature, float alpha)
{
	if (!camera->InView(feature->drawMidPos, feature->drawRadius)) { return false; }
	if (!feature->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView) { return false; }

	const float sqDist = (feature->pos - camera->pos).SqLength();
	const float farLength = feature->sqRadius * unitDrawer->unitDrawDistSqr;
	const float sqFadeDistEnd = featureDrawDistance * featureDrawDistance;

	if (sqDist >= std::min(farLength, sqFadeDistEnd)) return false;

	glPushMatrix();
	glMultMatrixf(feature->GetTransformMatrixRef());

	unitDrawer->SetTeamColour(feature->team, alpha);

	if (!(feature->luaDraw && luaRules && luaRules->DrawFeature(feature))) {
		feature->model->DrawStatic();
	}

	glPopMatrix();

	return true;
}






void CFeatureDrawer::DrawFadeFeatures(bool noAdvShading)
{
	const bool oldAdvShading = unitDrawer->advShading;

	{
		unitDrawer->advShading = unitDrawer->advShading && !noAdvShading;

		if (unitDrawer->advShading) {
			unitDrawer->SetupForUnitDrawing();
		} else {
			unitDrawer->SetupForGhostDrawing();
		}

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);

		ISky::SetupFog();

		{
			GML_RECMUTEX_LOCK(feat); // DrawFadeFeatures

			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				cloakedModelRenderers[modelType]->PushRenderState();
				DrawFadeFeaturesHelper(modelType);
				cloakedModelRenderers[modelType]->PopRenderState();
			}
		}

		glDisable(GL_FOG);

		glPopAttrib();

		if (unitDrawer->advShading) {
			unitDrawer->CleanUpUnitDrawing();
		} else {
			unitDrawer->CleanUpGhostDrawing();
		}

		unitDrawer->advShading = oldAdvShading;
	}
}

void CFeatureDrawer::DrawFadeFeaturesHelper(int modelType) {
	{
		FeatureRenderBin& featureBin = cloakedModelRenderers[modelType]->GetFeatureBinMutable();

		for (FeatureRenderBinIt it = featureBin.begin(); it != featureBin.end(); ++it) {
			if (modelType != MODELTYPE_3DO) {
				texturehandlerS3O->SetS3oTexture(it->first);
			}

			DrawFadeFeaturesSet(it->second, modelType);
		}
	}
}

void CFeatureDrawer::DrawFadeFeaturesSet(FeatureSet& fadeFeatures, int modelType)
{
	for (FeatureSet::iterator fi = fadeFeatures.begin(); fi != fadeFeatures.end(); ) {
		const float cols[] = {1.0f, 1.0f, 1.0f, fi->second};

		if (modelType != MODELTYPE_3DO) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
		}

		// hack, sorting objects by distance would look better
		glAlphaFunc(GL_GREATER, fi->second / 2.0f);
		glColor4fv(cols);

		if (!DrawFeatureNow(fi->first, fi->second)) {
			fi = set_erase(fadeFeatures, fi);
		} else {
			++fi;
		}
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
		GML_RECMUTEX_LOCK(feat); // DrawShadowPass

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
	float sqFadeDistBegin;
	float sqFadeDistEnd;
	bool farFeatures;
#ifdef USE_GML
	std::vector<CFeature*>* statFeatures;
#endif

	std::vector<CFeatureDrawer::DrawQuad>* drawQuads;

	void DrawQuad(int x, int y)
	{
		std::vector<IWorldObjectModelRenderer*>& opaqueModelRenderers = featureDrawer->opaqueModelRenderers;
		std::vector<IWorldObjectModelRenderer*>& cloakedModelRenderers = featureDrawer->cloakedModelRenderers;

		const CFeatureDrawer::DrawQuad* dq = &(*drawQuads)[y * drawQuadsX + x];

		for (std::set<CFeature*>::const_iterator fi = dq->features.begin(); fi != dq->features.end(); ++fi) {
			CFeature* f = (*fi);

			assert(f->def->drawType == DRAWTYPE_MODEL);

			if (gu->spectatingFullView || f->IsInLosForAllyTeam(gu->myAllyTeam)) {
				if (drawReflection) {
					float3 zeroPos;

					if (f->midPos.y < 0.0f) {
						zeroPos = f->midPos;
					} else {
						const float dif = f->midPos.y - camera->pos.y;
						zeroPos =
							camera->pos * (f->midPos.y / dif) +
							f->midPos * (-camera->pos.y / dif);
					}
					if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z, false) > f->drawRadius) {
						continue;
					}
				}
				if (drawRefraction) {
					if (f->pos.y > 0.0f)
						continue;
				}

				const float sqDist = (f->pos - camera->pos).SqLength();
				const float farLength = f->sqRadius * unitDrawer->unitDrawDistSqr;
#ifdef USE_GML
				if (statFeatures && (f->reclaimLeft < 1.0f || f->resurrectProgress > 0.0f))
					statFeatures->push_back(f);
#endif

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
						cloakedModelRenderers[MDL_TYPE(f)]->DelFeature(f);
						if (camera->InView(f->drawMidPos, f->drawRadius))
							opaqueModelRenderers[MDL_TYPE(f)]->AddFeature(f);
					} else if (sqDist < sqFadeDistE) {
						const float falpha = 1.0f - (sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB);
						opaqueModelRenderers[MDL_TYPE(f)]->DelFeature(f);
						if (camera->InView(f->drawMidPos, f->drawRadius))
							cloakedModelRenderers[MDL_TYPE(f)]->AddFeature(f, falpha);
					}
				} else {
					if (farFeatures) {
						farTextureHandler->Queue(f);
					}
				}
			}
		}
	}
};



void CFeatureDrawer::GetVisibleFeatures(int extraSize, bool drawFar)
{
	CFeatureQuadDrawer drawer;
	drawer.drawQuads = &drawQuads;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection = water->IsDrawReflection();
	drawer.drawRefraction = water->IsDrawRefraction();
	drawer.sqFadeDistEnd = featureDrawDistance * featureDrawDistance;
	drawer.sqFadeDistBegin = featureFadeDistance * featureFadeDistance;
	drawer.farFeatures = drawFar;
#ifdef USE_GML
	drawer.statFeatures = showRezBars ? &drawStat : NULL;
#endif

	readmap->GridVisibility(camera, DRAW_QUAD_SIZE, featureDrawDistance, &drawer, extraSize);
}

void CFeatureDrawer::SwapFeatures() {
	GML_RECMUTEX_LOCK(feat); // SwapFeatures

	for(int i = 0; i < MODELTYPE_OTHER; ++i) {
		opaqueModelRenderers[i]->SwapFeatures();
		cloakedModelRenderers[i]->SwapFeatures();
	}
}

void CFeatureDrawer::PostLoad()
{
	drawQuadsX = gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY = gs->mapy/DRAW_QUAD_SIZE;
	drawQuads.clear();
	drawQuads.resize(drawQuadsX * drawQuadsY);

	const CFeatureSet& fs = featureHandler->GetActiveFeatures();
	for (CFeatureSet::const_iterator it = fs.begin(); it != fs.end(); ++it)
		if ((*it)->drawQuad >= 0)
			drawQuads[(*it)->drawQuad].features.insert(*it);
}
