
#include "StdAfx.h"

#include "Game/Camera.h"
#include "Sm3GroundDrawer.h"
#include "Sm3Map.h"
#include "terrain/TerrainNode.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/GroundDecalHandler.h"
#include <GL/glew.h>
#include "Platform/ConfigHandler.h"

#include <SDL_keysym.h>
extern unsigned char *keys;


CSm3GroundDrawer::CSm3GroundDrawer(CSm3ReadMap *m)
{
	map = m;
	tr = map->renderer;
	rc = tr->AddRenderContext (&cam, true);

	tr->config.detailMod = configHandler.GetInt("SM3TerrainDetail", 200) / 100.0f;

	/*if (shadowHandler->drawShadows) {
		shadowrc = tr->AddRenderContext (&shadowCam,false);

		groundShadowVP=LoadVertexProgram("groundshadow.vp");
	}
	else  {*/
		shadowrc = 0;
		groundShadowVP = 0;
	//}
	reflectrc = 0;
}

CSm3GroundDrawer::~CSm3GroundDrawer()
{
	if (groundShadowVP)
		glDeleteProgramsARB( 1, &groundShadowVP );

	configHandler.SetInt("SM3TerrainDetail", int(tr->config.detailMod * 100));
}

void CSm3GroundDrawer::Update()
{
	// Copy camera settings
	cam.fov = camera->fov;
	cam.front = camera->forward;
	cam.right = camera->right;
	cam.up = camera->up;
	cam.pos = camera->pos;
	cam.aspect = gu->screenx / (float)gu->screeny;

	cam.right = cam.front.cross(cam.up);
	cam.right.Normalize();

	cam.up=cam.right.cross(cam.front);
	cam.up.Normalize();

	tr->Update();
	tr->CacheTextures();

	// Update visibility nodes
	for (unsigned int i=0;i<rc->quads.size();i++) {
		terrain::TQuad *q = rc->quads[i].quad;

	//	if (q

	//	for (unsigned int j=0;j<q->nodeLinks.size();j++)
	//		q->nodeLinks[j]->callback->Draw(
	}
}

void CSm3GroundDrawer::Draw(bool drawWaterReflection,bool drawUnitReflection,unsigned int overrideVP)
{
	if(drawUnitReflection || drawWaterReflection)
		return;

	terrain::RenderContext *currc;
	if (drawWaterReflection)
	{
		if (!reflectrc) {
			reflectrc = tr->AddRenderContext(&reflectCam, true);
			tr->Update();
		}

		reflectCam.fov = PI * camera->fov / 180.0f;
		reflectCam.front = camera->forward;
		reflectCam.right = camera->right;
		reflectCam.up = camera->up;
		reflectCam.pos = camera->pos;
		reflectCam.aspect = 1.0f;
		currc = reflectrc;
	} else 
		currc = rc;

	tr->SetShaderParams(gs->sunVector, currc->cam->pos);
/*
	if (shadowHandler->drawShadows) {
		terrain::ShadowMapParams params;

		shadowHandler->GetShadowMapSizeFactors (params.f_a, params.f_b);
		params.mid [0] = shadowHandler->xmid;
		params.mid [1] = shadowHandler->ymid;
		params.shadowMap = shadowHandler->shadowTexture;
		for (int a=0;a<16;a++) 
			params.shadowMatrix[a] = shadowHandler->shadowMatrix[a];

		tr->SetShadowParams (&params);
	}*/

	tr->SetActiveContext (currc);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION,gs->sunVector4);
	float d[4]={0.0f,0.0f,0.0f,1.0f};
	for (int a=0;a<3;a++) d[a]=map->sunColor[a];
	glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
	for (int a=0;a<3;a++) d[a]=map->ambientColor[a];
	glLightfv(GL_LIGHT0, GL_AMBIENT, d);
	for (int a=0;a<3;a++) d[a]=map->specularColor[a];
	glLightfv (GL_LIGHT0, GL_SPECULAR, d);
	for (int a=0;a<4;a++) d[a]=0.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, d);
	const float zero[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, zero);
	glDisable(GL_LIGHT1);
	glEnable(GL_LIGHT0);
	glEnable(GL_RESCALE_NORMAL);

//	glLightfv (GL_LIGHT0, GL_SPOT_DIRECTION,dir.getf());
//	glLightf (GL_LIGHT0, GL_SPOT_CUTOFF, 90.0f);
/*	const float ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	const float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

	const float md[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, md);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, md);
	glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 10.0f);*/
	/////////////////////

	tr->Draw ();
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);


	if (drawMode != drawNormal)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f,1.0f,1.0f,0.5f);
		tr->DrawOverlayTexture (infoTex);
		glDisable(GL_BLEND);
	}

	glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);

	glColor3ub(255,255,255);
	glDisable(GL_TEXTURE_2D);

	DrawObjects(drawWaterReflection, drawUnitReflection);
}


void CSm3GroundDrawer::DrawShadowPass()
{
	/*shadowCam.fov = PI * camera->fov / 180.0f;
	shadowCam.front = camera->forward;
	shadowCam.right = camera->right;
	shadowCam.up = camera->up;
	shadowCam.pos = camera->pos;
	shadowCam.aspect = 1.0f;
	
	glPolygonOffset(1,1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glBindProgramARB (GL_VERTEX_PROGRAM_ARB, groundShadowVP);
	glEnable (GL_VERTEX_PROGRAM_ARB);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	tr->SetActiveContext (shadowrc);
	tr->DrawSimple ();
	tr->SetActiveContext(rc);

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_VERTEX_PROGRAM_ARB);*/
}

void CSm3GroundDrawer::DrawObjects(bool drawWaterReflection,bool drawUnitReflection)
{
/*	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
*/
//	sky->SetCloudShadow(1);
//	if(drawWaterReflection)
//		treeDistance*=0.5;

	if(groundDecals && !(drawWaterReflection || drawUnitReflection))
		groundDecals->Draw();
	ph->DrawGroundFlashes();
	if(treeDrawer->drawTrees){
		if(DrawExtraTex()){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glBindTexture(GL_TEXTURE_2D, infoTex);
			SetTexGen(1.0/(gs->pwr2mapx*SQUARE_SIZE),1.0/(gs->pwr2mapy*SQUARE_SIZE),0,0);
		  glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		treeDrawer->Draw(drawWaterReflection || drawUnitReflection);
		if(DrawExtraTex()){
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_2D);
		  glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
	
	glDisable(GL_ALPHA_TEST);
}

const int maxQuadDepth = 4;

void CSm3GroundDrawer::IncreaseDetail()
{
	tr->config.detailMod *= 1.1f;
	if (tr->config.detailMod > 12.0f)
		tr->config.detailMod = 12.0f;
	info->AddLine("Terrain detail changed to: %2.2f", tr->config.detailMod);
}

void CSm3GroundDrawer::DecreaseDetail()
{
	tr->config.detailMod /= 1.1f;
	if (tr->config.detailMod < 0.25f)
		tr->config.detailMod = 0.25f;

	info->AddLine("Terrain detail changed to: %2.2f", tr->config.detailMod);
}

