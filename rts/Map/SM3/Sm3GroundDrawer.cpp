
#include "StdAfx.h"

#include "Game/Camera.h"
#include "Sm3GroundDrawer.h"
#include "Sm3Map.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/GroundDecalHandler.h"
#include <GL/glew.h>


CSm3GroundDrawer::CSm3GroundDrawer(CSm3ReadMap *m)
{
	map = m;
	tr = map->renderer;
	rc = tr->AddRenderContext (&cam, true);

	if (shadowHandler->drawShadows) {
		shadowrc = tr->AddRenderContext (&shadowCam,false);

		groundShadowVP=LoadVertexProgram("groundshadow.vp");
	}
	else  {
		shadowrc = 0;
		groundShadowVP = 0;
	}
	reflectrc = 0;
}

CSm3GroundDrawer::~CSm3GroundDrawer()
{
	if (groundShadowVP)
		glDeleteProgramsARB( 1, &groundShadowVP );
}

void CSm3GroundDrawer::Update()
{
	tr->Update();
	tr->CacheTextures();
}

void CSm3GroundDrawer::Draw(bool drawWaterReflection,bool drawUnitReflection,unsigned int overrideVP)
{
	if(drawWaterReflection || drawUnitReflection)
		return;

	// Copy camera settings
	cam.fov = PI * camera->fov / 180.0f;
	cam.front = camera->forward;
	cam.right = camera->right;
	cam.up = camera->up;
	cam.pos = camera->pos;
	
	tr->SetShaderParams(gs->sunVector, Vector3());
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

	tr->SetActiveContext (rc);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
/*
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION,gs->sunVector4);
	float d[4]={0.0f,0.0f,0.0f,1.0f};
	for (int a=0;a<3;a++) d[a]=map->sunColor[a];
	glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
	for (int a=0;a<3;a++) d[a]=map->ambientColor[a];
	glLightfv(GL_LIGHT0, GL_AMBIENT, d);
	for (int a=0;a<4;a++) d[a]=0.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, d);
	glDisable(GL_LIGHT1);
	glEnable(GL_LIGHT0);*/
//////////////////
	glEnable(GL_RESCALE_NORMAL);

//	glLightfv (GL_LIGHT0, GL_SPOT_DIRECTION,dir.getf());
//	glLightf (GL_LIGHT0, GL_SPOT_CUTOFF, 90.0f);
/*	const float ambient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	const float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);

	const float zero[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightfv (GL_LIGHT0, GL_SPECULAR, zero);
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, zero);

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
	shadowCam.fov = PI * camera->fov / 180.0f;
	shadowCam.front = camera->forward;
	shadowCam.right = camera->right;
	shadowCam.up = camera->up;
	shadowCam.pos = camera->pos;
	
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
	glDisable(GL_VERTEX_PROGRAM_ARB);
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


void CSm3GroundDrawer::UpdateCamRestraints()
{
}

