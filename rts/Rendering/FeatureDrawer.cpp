/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include "FeatureDrawer.h"

#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/myMath.h"


#define DRAW_QUAD_SIZE 32

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

	showRezBars = !!configHandler->Get("ShowRezBars", 1);

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
	delete treeDrawer;

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

	if (f->def->drawType == DRAWTYPE_MODEL) {
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
}


void CFeatureDrawer::RenderFeatureMoved(const CFeature* feature)
{
	CFeature* f = const_cast<CFeature*>(feature);

	UpdateDrawQuad(f);
}

void CFeatureDrawer::UpdateDrawQuad(CFeature* feature)
{
	const int oldDrawQuad = feature->drawQuad;
	if (oldDrawQuad >= -1) {
		const int newDrawQuad =
			int(feature->pos.z / DRAW_QUAD_SIZE / SQUARE_SIZE) * drawQuadsX +
			int(feature->pos.x / DRAW_QUAD_SIZE / SQUARE_SIZE);
		if (oldDrawQuad != newDrawQuad) {
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


void CFeatureDrawer::UpdateDrawPos(CFeature* f)
{
//#if defined(USE_GML) && GML_ENABLE_SIM
//	f->drawPos = f->pos + (f->speed * ((float)globalRendering->lastFrameStart - (float)f->lastUnitUpdate) * globalRendering->weightedSpeedFactor);
//#else
	f->drawPos = f->pos + (f->speed * globalRendering->timeOffset);
//#endif
	f->drawMidPos = f->drawPos + (f->midPos - f->pos);
}


void CFeatureDrawer::Draw()
{
	if(globalRendering->drawFog) {
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	}

	GML_RECMUTEX_LOCK(feat); // Draw

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

	if (gd->DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

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

	DrawFeatureStats();
}

void CFeatureDrawer::DrawOpaqueFeatures(int modelType)
{
	typedef std::set<CFeature*> FeatureSet;
	typedef std::map<int, FeatureSet> FeatureRenderBin;

	FeatureRenderBin& featureBin = opaqueModelRenderers[modelType]->GetFeatureBinMutable();

	FeatureRenderBin::iterator featureBinIt;
	FeatureSet::iterator featureSetIt;

	for (featureBinIt = featureBin.begin(); featureBinIt != featureBin.end(); ++featureBinIt) {
		if (modelType == MODELTYPE_S3O || modelType == MODELTYPE_OBJ) {
			texturehandlerS3O->SetS3oTexture(featureBinIt->first);
		}

		FeatureSet& featureSet = featureBinIt->second;

		for (featureSetIt = featureSet.begin(); featureSetIt != featureSet.end(); ) {
			FeatureSet::iterator featureSetItNext(featureSetIt); ++featureSetItNext;

			if (!DrawFeatureNow(*featureSetIt)) {
				featureSet.erase(*featureSetIt);
				featureSetIt = featureSetItNext;
			} else {
				++featureSetIt;
			}
		}
	}
}

void CFeatureDrawer::DrawFeatureStats()
{
	if (!drawStat.empty()) {
		if (!water->drawReflection) {
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
	glCallList(CCamera::billboardList);

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

bool CFeatureDrawer::DrawFeatureNow(const CFeature* feature)
{
	if (!camera->InView(feature->pos, feature->radius * 4.0f)) { return false; }
	if (!feature->IsInLosForAllyTeam(gu->myAllyTeam)) { return false; }

	glPushMatrix();
	glMultMatrixf(feature->transMatrix.m);

	unitDrawer->SetTeamColour(feature->team, feature->tempalpha);

	feature->model->DrawStatic();
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

		if (globalRendering->drawFog) {
			glEnable(GL_FOG);
			glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
		}

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
		typedef std::set<CFeature*> FeatureSet;
		typedef std::map<int, FeatureSet> FeatureRenderBin;
		typedef std::map<int, FeatureSet>::iterator FeatureRenderBinIt;

		FeatureRenderBin& featureBin = cloakedModelRenderers[modelType]->GetFeatureBinMutable();

		for (FeatureRenderBinIt it = featureBin.begin(); it != featureBin.end(); ++it) {
			if (modelType == MODELTYPE_S3O || modelType == MODELTYPE_OBJ) {
				texturehandlerS3O->SetS3oTexture(it->first);
			}

			DrawFadeFeaturesSet(it->second, modelType);
		}
	}
}

void CFeatureDrawer::DrawFadeFeaturesSet(std::set<CFeature*>& fadeFeatures, int modelType)
{
	for (std::set<CFeature*>::const_iterator fi = fadeFeatures.begin(); fi != fadeFeatures.end(); ) {
		std::set<CFeature*>::const_iterator fiNext(fi); fiNext++;

		const float cols[] = {1.0f, 1.0f, 1.0f, (*fi)->tempalpha};

		if (modelType == MODELTYPE_S3O) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
		}

		// hack, sorting objects by distance would look better
		glAlphaFunc(GL_GREATER, (*fi)->tempalpha / 2.0f);
		glColor4fv(cols);

		if (!DrawFeatureNow(*fi)) {
			fadeFeatures.erase(*fi);
			fi = fiNext;
		} else {
			fi++;
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

		if (!globalRendering->atiHacks) {
			// FIXME: why does texture alpha not work with shadows on ATI?
			// need the alpha-mask for transparent features
			glEnable(GL_TEXTURE_2D);
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.5f);
		}

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
			DrawOpaqueFeatures(modelType);
		}

		if (!globalRendering->atiHacks) {
			glPopAttrib();
			glDisable(GL_TEXTURE_2D);
		}
	}

	po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
}



class CFeatureQuadDrawer : public CReadMap::IQuadDrawer {
public:
	int drawQuadsX;
	bool drawReflection, drawRefraction;
	float unitDrawDist;
	float sqFadeDistBegin;
	float sqFadeDistEnd;
	bool farFeatures;

	std::vector<CFeatureDrawer::DrawQuad>* drawQuads;
	std::vector<CFeature*>* statFeatures;

	void DrawQuad(int x, int y)
	{
		std::vector<IWorldObjectModelRenderer*>& opaqueModelRenderers = featureDrawer->opaqueModelRenderers;
		std::vector<IWorldObjectModelRenderer*>& cloakedModelRenderers = featureDrawer->cloakedModelRenderers;

		const CFeatureDrawer::DrawQuad* dq = &(*drawQuads)[y * drawQuadsX + x];

		for (std::set<CFeature*>::const_iterator fi = dq->features.begin(); fi != dq->features.end(); ++fi) {
			CFeature* f = (*fi);

			if (f->def->drawType != DRAWTYPE_MODEL)
				continue;

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
					if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z) > f->radius) {
						continue;
					}
				}
				if (drawRefraction) {
					if (f->pos.y > 0.0f)
						continue;
				}

				const float sqDist = (f->pos - camera->pos).SqLength2D();
				const float farLength = f->sqRadius * unitDrawDist * unitDrawDist;

				if (statFeatures && (f->reclaimLeft < 1.0f || f->resurrectProgress > 0.0f))
					statFeatures->push_back(f);

				if (sqDist < farLength) {
					float sqFadeDistE;
					float sqFadeDistB;

					if (farLength < sqFadeDistEnd) {
						sqFadeDistE = farLength;
						sqFadeDistB = farLength * 0.75f * 0.75f;
					} else {
						sqFadeDistE = sqFadeDistEnd;
						sqFadeDistB = sqFadeDistBegin;
					}

					if (sqDist < sqFadeDistB) {
						f->tempalpha = 0.99f;
						cloakedModelRenderers[MDL_TYPE(f)]->DelFeature(f);
						opaqueModelRenderers[MDL_TYPE(f)]->AddFeature(f);
					} else if (sqDist < sqFadeDistE) {
						f->tempalpha = 1.0f - (sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB);
						opaqueModelRenderers[MDL_TYPE(f)]->DelFeature(f);
						cloakedModelRenderers[MDL_TYPE(f)]->AddFeature(f);
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
	float featureDist = 3000.0f;

	if (extraSize == 0) {
		// far-features are not drawn during shadowpass anyway
		featureDist *= 2.0f;
	}

	CFeatureQuadDrawer drawer;
	drawer.drawQuads = &drawQuads;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection = water->drawReflection;
	drawer.drawRefraction = water->drawRefraction;
	drawer.unitDrawDist = unitDrawer->unitDrawDist;
	drawer.sqFadeDistEnd = featureDist * featureDist;
	drawer.sqFadeDistBegin = 0.75f * 0.75f * featureDist * featureDist;
	drawer.farFeatures = drawFar;
	drawer.statFeatures = showRezBars ? &drawStat : NULL;

	readmap->GridVisibility(camera, DRAW_QUAD_SIZE, featureDist, &drawer, extraSize);
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
