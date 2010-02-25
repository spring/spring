/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include "FeatureDrawer.h"

#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/myMath.h"


#define DRAW_QUAD_SIZE 32


using std::set;
using std::vector;


CFeatureDrawer* featureDrawer = NULL;

/******************************************************************************/

CR_BIND(CFeatureDrawer, );

CR_BIND(CFeatureDrawer::DrawQuad, );

CR_REG_METADATA(CFeatureDrawer, (
	CR_POSTLOAD(PostLoad)
));

/******************************************************************************/



CFeatureDrawer::CFeatureDrawer()
{
	drawQuadsX = gs->mapx/DRAW_QUAD_SIZE;
	drawQuadsY = gs->mapy/DRAW_QUAD_SIZE;
	drawQuads.resize(drawQuadsX * drawQuadsY);

	treeDrawer = CBaseTreeDrawer::GetTreeDrawer();

	showRezBars = !!configHandler->Get("ShowRezBars", 1);
}


CFeatureDrawer::~CFeatureDrawer()
{
	delete treeDrawer;
}


void CFeatureDrawer::FeatureCreated(CFeature* feature)
{
	if (feature->def->drawType == DRAWTYPE_MODEL) {
		feature->drawQuad = -1;
		UpdateDrawPos(feature);
	}
}


void CFeatureDrawer::FeatureDestroyed(CFeature* feature)
{
	{
		GML_STDMUTEX_LOCK(rfeat); // Update

		if (feature->drawQuad >= 0) {
			DrawQuad* dq = &drawQuads[feature->drawQuad];
			dq->features.erase(feature);
		}

		updateDrawFeatures.erase(feature);
	}

	fadeFeatures.erase(feature);
	fadeFeaturesS3O.erase(feature);
	fadeFeaturesSave.erase(feature);
	fadeFeaturesS3OSave.erase(feature);
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


void CFeatureDrawer::UpdateDraw()
{
	GML_STDMUTEX_LOCK(rfeat); // UpdateDraw

	for(set<CFeature *>::iterator i=updateDrawFeatures.begin(); i!= updateDrawFeatures.end(); ++i) {
		UpdateDrawQuad(*i);
	}

	updateDrawFeatures.clear();
}


void CFeatureDrawer::UpdateDrawPos(CFeature* feature)
{
	GML_STDMUTEX_LOCK(rfeat); // UpdateDrawPos

	updateDrawFeatures.insert(feature);
}


void CFeatureDrawer::SwapFadeFeatures()
{
	GML_RECMUTEX_LOCK(feat); // SwapFadeFeatures

	fadeFeatures.swap(fadeFeaturesSave);
	fadeFeaturesS3O.swap(fadeFeaturesS3OSave);
}


void CFeatureDrawer::Draw()
{
	drawFar.clear();

	if(gu->drawFog) {
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	}


	GML_RECMUTEX_LOCK(feat); // Draw

	fadeFeatures.clear();
	fadeFeaturesS3O.clear();

	CBaseGroundDrawer *gd = readmap->GetGroundDrawer();
	if (gd->DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);

		glBindTexture(GL_TEXTURE_2D, gd->infoTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	unitDrawer->SetupForUnitDrawing();
		unitDrawer->SetupFor3DO();
			DrawRaw(0, &drawFar);
		unitDrawer->CleanUp3DO();
		unitDrawer->DrawQuedS3O();
	unitDrawer->CleanUpUnitDrawing();

	if (drawFar.size()>0) {
		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);
		glBindTexture(GL_TEXTURE_2D, farTextureHandler->GetTextureID());
		glColor3f(1.0f, 1.0f, 1.0f);
		glNormal3fv((const GLfloat*) &unitDrawer->camNorm.x);

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(drawFar.size() * 4, 0, VA_SIZE_T);
		for (vector<CFeature*>::iterator it = drawFar.begin(); it != drawFar.end(); ++it) {
			farTextureHandler->DrawFarTexture(camera, (*it)->model, (*it)->pos, (*it)->radius, (*it)->heading, va);
		}
		va->DrawArrayT(GL_QUADS);

		glDisable(GL_ALPHA_TEST);
	}

	if (gd->DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);

	if(drawStat.size() > 0) {
		if(!water->drawReflection) {
			for (vector<CFeature *>::iterator fi = drawStat.begin(); fi != drawStat.end(); ++fi)
				DrawFeatureStats(*fi);
		}
		drawStat.clear();
	}
}


void CFeatureDrawer::DrawFadeFeatures(bool submerged, bool noAdvShading)
{
	if (fadeFeatures.empty() && fadeFeaturesS3O.empty())
		return;

	bool oldAdvShading = unitDrawer->advShading;
	unitDrawer->advShading = unitDrawer->advShading && !noAdvShading;

	if(unitDrawer->advShading)
		unitDrawer->SetupForUnitDrawing();
	else
		unitDrawer->SetupForGhostDrawing();

	glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_TRUE);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.5f);

	if(gu->drawFog) {
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
	}

	double plane[4]={0,submerged?-1:1,0,0};
	glClipPlane(GL_CLIP_PLANE3, plane);

	unitDrawer->SetupFor3DO();

	{
		GML_RECMUTEX_LOCK(feat); // DrawFadeFeatures

		for(set<CFeature *>::const_iterator i = fadeFeatures.begin(); i != fadeFeatures.end(); ++i) {
			glColor4f(1,1,1,(*i)->tempalpha);
			glAlphaFunc(GL_GREATER,(*i)->tempalpha/2.0f);
			unitDrawer->DrawFeatureStatic(*i);
		}

		unitDrawer->CleanUp3DO();

		for(set<CFeature *>::const_iterator i = fadeFeaturesS3O.begin(); i != fadeFeaturesS3O.end(); ++i) {
			float cols[]={1,1,1,(*i)->tempalpha};
			glColor4fv(cols);
			glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,cols);
			glAlphaFunc(GL_GREATER,(*i)->tempalpha/2.0f); // a hack, sorting objects by distance would look better

			texturehandlerS3O->SetS3oTexture((*i)->model->textureType);
			(*i)->DrawS3O();
		}
	}

	glDisable(GL_FOG);

	glPopAttrib();

	if (unitDrawer->advShading)
		unitDrawer->CleanUpUnitDrawing();
	else
		unitDrawer->CleanUpGhostDrawing();

	unitDrawer->advShading = oldAdvShading;
}


void CFeatureDrawer::DrawShadowPass()
{
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	shadowHandler->GetMdlShadowGenShader()->Enable();

	{
		GML_RECMUTEX_LOCK(feat); // DrawShadowPass

		DrawRaw(1, NULL);

		if (!gu->atiHacks) { // FIXME: Why does texture alpha not work with shadows on ATI?
			glEnable(GL_TEXTURE_2D); // Need the alpha mask for transparent features
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER,0.5f);
		}

		unitDrawer->DrawQuedS3O();
	}

	if (!gu->atiHacks) {
		glPopAttrib();
		glDisable(GL_TEXTURE_2D);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	shadowHandler->GetMdlShadowGenShader()->Disable();
}

class CFeatureQuadDrawer : public CReadMap::IQuadDrawer
{
public:
	void DrawQuad(int x,int y);

	vector<CFeatureDrawer::DrawQuad>* drawQuads;
	int drawQuadsX;
	bool drawReflection, drawRefraction;
	float unitDrawDist;
	float sqFadeDistBegin;
	float sqFadeDistEnd;
	vector<CFeature*>* farFeatures;
	vector<CFeature*>* statFeatures;
};


void CFeatureQuadDrawer::DrawQuad(int x, int y)
{
	CFeatureDrawer::DrawQuad* dq = &(*drawQuads)[y * drawQuadsX + x];

	for (set<CFeature*>::iterator fi = dq->features.begin(); fi != dq->features.end(); ++fi) {
		CFeature* f = (*fi);
		const FeatureDef* def = f->def;

		if (def->drawType == DRAWTYPE_MODEL
				&& (gu->spectatingFullView || f->IsInLosForAllyTeam(gu->myAllyTeam))) {
			if (drawReflection) {
				float3 zeroPos;
				if (f->midPos.y < 0) {
					zeroPos = f->midPos;
				} else {
					float dif = f->midPos.y - camera->pos.y;
					zeroPos = camera->pos * (f->midPos.y / dif) + f->midPos * (-camera->pos.y / dif);
				}
				if (ground->GetApproximateHeight(zeroPos.x, zeroPos.z) > f->radius) {
					continue;
				}
			}
			if (drawRefraction) {
				if (f->pos.y > 0)
					continue;
			}

			float sqDist = (f->pos - camera->pos).SqLength2D();
			float farLength = f->sqRadius * unitDrawDist * unitDrawDist;

			if(statFeatures && (f->reclaimLeft < 1.0f || f->resurrectProgress > 0.0f))
				statFeatures->push_back(f);

			if (sqDist < farLength) {
				float sqFadeDistE;
				float sqFadeDistB;
				if(farLength < sqFadeDistEnd) {
					sqFadeDistE = farLength;
					sqFadeDistB = farLength * 0.75f * 0.75f;
				} else {
					sqFadeDistE = sqFadeDistEnd;
					sqFadeDistB = sqFadeDistBegin;
				}
				if(sqDist < sqFadeDistB) {
					f->tempalpha = 0.99f;
					if (f->model->type == MODELTYPE_3DO) {
						unitDrawer->DrawFeatureStatic(f);
					} else {
						unitDrawer->QueS3ODraw(f, f->model->textureType);
					}
				} else if(sqDist < sqFadeDistE) {
					f->tempalpha = 1.0f - (sqDist - sqFadeDistB) / (sqFadeDistE - sqFadeDistB);
					if (f->model->type == MODELTYPE_3DO) {
						featureDrawer->fadeFeatures.insert(f);
					} else {
						featureDrawer->fadeFeaturesS3O.insert(f);
					}
				}
			} else {
				if (farFeatures)
					farFeatures->push_back(f);
			}
		}
	}
}


void CFeatureDrawer::DrawRaw(int extraSize, vector<CFeature*>* farFeatures)
{
	float featureDist = 3000;
	if (!extraSize) {
		featureDist = 6000; //farfeatures wont be drawn for shadowpass anyway
	}
	CFeatureQuadDrawer drawer;
	drawer.drawQuads = &drawQuads;
	drawer.drawQuadsX = drawQuadsX;
	drawer.drawReflection=water->drawReflection;
	drawer.drawRefraction=water->drawRefraction;
	drawer.unitDrawDist=unitDrawer->unitDrawDist;
	drawer.sqFadeDistEnd = featureDist * featureDist;
	drawer.sqFadeDistBegin = 0.75f * 0.75f * featureDist * featureDist;
	drawer.farFeatures = farFeatures;
	drawer.statFeatures = showRezBars ? &drawStat : NULL;

	readmap->GridVisibility(camera, DRAW_QUAD_SIZE, featureDist, &drawer, extraSize);
}



void CFeatureDrawer::DrawFeatureStats(CFeature* feature)
{
	float3 interPos = feature->pos;
	interPos.y += feature->height + 5.0f;

	glPushMatrix();
	glTranslatef(interPos.x, interPos.y, interPos.z);
	glCallList(CCamera::billboardList);

	float recl = feature->reclaimLeft;
	float rezp = feature->resurrectProgress;

	// black background for the bar
	glColor3f(0.0f, 0.0f, 0.0f);
	glRectf(-5.0f, 4.0f, +5.0f, 6.0f);

	// rez/metalbar
	float rmin = std::min(recl, rezp) * 10.0f;
	if(rmin > 0.0f) {
		glColor3f(1.0f, 0.0f, 1.0f);
		glRectf(-5.0f, 4.0f, rmin - 5.0f, 6.0f);
	}
	if(recl > rezp) {
		float col = 0.8 - 0.3 * recl;
		glColor3f(col, col, col);
		glRectf(rmin - 5.0f, 4.0f, recl * 10.0f - 5.0f, 6.0f);
	}
	if(recl < rezp) {
		glColor3f(0.5f, 0.0f, 1.0f);
		glRectf(rmin - 5.0f, 4.0f, rezp * 10.0f - 5.0f, 6.0f);
	}

	glPopMatrix();
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
