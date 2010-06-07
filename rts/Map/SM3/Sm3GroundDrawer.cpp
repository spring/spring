/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "Sm3GroundDrawer.h"
#include "Sm3Map.h"
#include "terrain/TerrainNode.h"

#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/myGL.h"
#include "System/GlobalUnsynced.h"
#include "System/ConfigHandler.h"

#include <SDL_keysym.h>
extern unsigned char *keys;


CSm3GroundDrawer::CSm3GroundDrawer(CSm3ReadMap *m)
{
	map = m;
	tr = map->renderer;
	rc = tr->AddRenderContext (&cam, true);

	tr->config.detailMod = configHandler->Get("SM3TerrainDetail", 200) / 100.0f;

	if (shadowHandler->canUseShadows) {
		shadowrc = tr->AddRenderContext(&shadowCam,false);
	} else  {
		shadowrc = 0;
	}
	reflectrc = 0;
}

CSm3GroundDrawer::~CSm3GroundDrawer()
{
	configHandler->Set("SM3TerrainDetail", int(tr->config.detailMod * 100));
}

static void SpringCamToTerrainCam(CCamera &sc, terrain::Camera& tc)
{
	// Copy camera settings
	tc.fov = sc.GetFov();
	tc.front = sc.forward;
	tc.right = sc.right;
	tc.up = sc.up;
	tc.pos = sc.pos;
	tc.aspect = globalRendering->aspectRatio;

	tc.right = tc.front.cross(tc.up);
	tc.right.ANormalize();

	tc.up=tc.right.cross(tc.front);
	tc.up.ANormalize();
}

void CSm3GroundDrawer::Update()
{
	SpringCamToTerrainCam(*camera, cam);

	tr->Update();
	tr->CacheTextures();
}

void CSm3GroundDrawer::Draw(bool drawWaterReflection, bool drawUnitReflection)
{
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	terrain::RenderContext* currc = rc;

	tr->SetShaderParams(mapInfo->light.sunDir, currc->cam->pos);

	if (shadowHandler->drawShadows) {
		terrain::ShadowMapParams params;

		params.mid[0] = shadowHandler->GetShadowParams().x;
		params.mid[1] = shadowHandler->GetShadowParams().y;
		params.f_a    = shadowHandler->GetShadowParams().z;
		params.f_b    = shadowHandler->GetShadowParams().w;
		params.shadowMap = shadowHandler->shadowTexture;

		for (int a = 0; a < 16; a++)
			params.shadowMatrix[a] = shadowHandler->shadowMatrix[a];

		tr->SetShadowParams(&params);
	}

	tr->SetActiveContext(currc);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, mapInfo->light.sunDir);

	float d[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	const float z[] = {0.0f, 0.0f, 0.0f, 1.0f};

	for (int a = 0; a < 3; a++)
		d[a] = mapInfo->light.groundSunColor[a];
	glLightfv(GL_LIGHT0, GL_DIFFUSE, d);

	for (int a = 0; a < 3; a++)
		d[a] = mapInfo->light.groundAmbientColor[a];
	glLightfv(GL_LIGHT0, GL_AMBIENT, d);

	for (int a = 0; a < 3; a++)
		d[a] = mapInfo->light.groundSpecularColor[a];
	glLightfv(GL_LIGHT0, GL_SPECULAR, d);

	for (int a = 0; a < 4; a++)
		d[a] = 0.0f;
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, d);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, z);
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

	tr->Draw();
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (drawMode != drawNormal) {
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0f, -10.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		tr->DrawOverlayTexture(infoTex);
		glDisable(GL_POLYGON_OFFSET_FILL);

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);

	glColor3ub(255, 255, 255);

	DrawObjects(drawWaterReflection, drawUnitReflection);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
}


void CSm3GroundDrawer::DrawShadowPass()
{
	if (!shadowrc)
		return;

	shadowCam.fov = camera->GetHalfFov()*2.0f; // Why *2?
	shadowCam.front = camera->forward;
	shadowCam.right = camera->right;
	shadowCam.up = camera->up;
	shadowCam.pos = camera->pos;
	shadowCam.aspect = 1.0f;

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	po->Enable();
	tr->SetActiveContext(shadowrc);
	tr->DrawSimple();
	tr->SetActiveContext(rc);
	po->Disable();

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glDisable(GL_POLYGON_OFFSET_FILL);
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

	if (groundDecals && !(drawWaterReflection || drawUnitReflection)) {
		groundDecals->Draw();
		projectileDrawer->DrawGroundFlashes();
		glDepthMask(1);
	}
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

