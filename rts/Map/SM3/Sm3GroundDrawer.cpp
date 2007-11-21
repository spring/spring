
#include "StdAfx.h"

#include "Game/Camera.h"
#include "Sm3GroundDrawer.h"
#include "Sm3Map.h"
#include "terrain/TerrainNode.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/GroundDecalHandler.h"
#define GLEW_STATIC
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
	glSafeDeleteProgram( groundShadowVP );
	configHandler.SetInt("SM3TerrainDetail", int(tr->config.detailMod * 100));
}

static void SpringCamToTerrainCam(CCamera &sc, terrain::Camera& tc)
{
	// Copy camera settings
	tc.fov = sc.fov;
	tc.front = sc.forward;
	tc.right = sc.right;
	tc.up = sc.up;
	tc.pos = sc.pos;
	tc.aspect = gu->viewSizeX / (float)gu->viewSizeY;

	tc.right = tc.front.cross(tc.up);
	tc.right.Normalize();

	tc.up=tc.right.cross(tc.front);
	tc.up.Normalize();
}

void CSm3GroundDrawer::Update()
{
	SpringCamToTerrainCam(*camera, cam);

	tr->Update();
	tr->CacheTextures();
}

void CSm3GroundDrawer::Draw(bool drawWaterReflection,bool drawUnitReflection,unsigned int overrideVP)
{
	if(drawUnitReflection || drawWaterReflection)
		return;

	terrain::RenderContext *currc = rc;

	tr->SetShaderParams(gs->sunVector, currc->cam->pos);

	if (shadowHandler->drawShadows)
	{
		terrain::ShadowMapParams params;

		shadowHandler->GetShadowMapSizeFactors (params.f_a, params.f_b);
		params.mid [0] = shadowHandler->xmid;
		params.mid [1] = shadowHandler->ymid;
		params.shadowMap = shadowHandler->shadowTexture;
		for (int a=0;a<16;a++) 
			params.shadowMatrix[a] = shadowHandler->shadowMatrix[a];

		tr->SetShadowParams (&params);
	}

	tr->SetActiveContext (currc);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION,gs->sunVector4);
	float d[4]={0.0f,0.0f,0.0f,1.0f};
	for (int a=0;a<3;a++)
		d[a]=map->sunColor[a];
	glLightfv(GL_LIGHT0, GL_DIFFUSE, d);
	for (int a=0;a<3;a++)
		d[a]=map->ambientColor[a];
	glLightfv(GL_LIGHT0, GL_AMBIENT, d);
	for (int a=0;a<3;a++)
		d[a]=map->specularColor[a];
	glLightfv (GL_LIGHT0, GL_SPECULAR, d);
	for (int a=0;a<4;a++)
		d[a]=0.0f;
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
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f,- 10.0f);
		glColor4f(1.0f,1.0f,1.0f,0.5f);
		tr->DrawOverlayTexture (infoTex);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDepthMask(GL_FALSE);
		glDisable(GL_BLEND);
	}

	glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);

	glColor3ub(255,255,255);

	DrawObjects(drawWaterReflection, drawUnitReflection);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
}


void CSm3GroundDrawer::DrawShadowPass()
{
	if (!shadowrc)
		return;

	shadowCam.fov = PI * camera->fov / 180.0f;
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
//		treeDistance*=0.5f;

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
			SetTexGen(1.0f/(gs->pwr2mapx*SQUARE_SIZE),1.0f/(gs->pwr2mapy*SQUARE_SIZE),0,0);
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
	logOutput.Print("Terrain detail changed to: %2.2f", tr->config.detailMod);
}

void CSm3GroundDrawer::DecreaseDetail()
{
	tr->config.detailMod /= 1.1f;
	if (tr->config.detailMod < 0.25f)
		tr->config.detailMod = 0.25f;

	logOutput.Print("Terrain detail changed to: %2.2f", tr->config.detailMod);
}

