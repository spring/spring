/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/ModelDrawer.hpp"
#include "Rendering/Shaders/ShaderHandler.hpp"
#include "System/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/Matrix44f.h"
#include "System/LogOutput.h"

#define DEFAULT_SHADOWMAPSIZE 2048

CShadowHandler* shadowHandler = 0;

bool CShadowHandler::canUseShadows = false;
bool CShadowHandler::useFPShadows  = false;
bool CShadowHandler::firstInstance = true;


CShadowHandler::CShadowHandler(void)
{
	const bool tmpFirstInstance = firstInstance;
	firstInstance = false;

	drawShadows   = false;
	inShadowPass  = false;
	showShadowMap = false;
	shadowTexture = 0;
	drawTerrainShadow = true;

	if (!tmpFirstInstance && !canUseShadows) {
		return;
	}

	const bool haveShadowExts =
		GLEW_ARB_vertex_program &&
		GLEW_ARB_shadow &&
		GLEW_ARB_depth_texture &&
		GLEW_ARB_texture_env_combine;
	//! Shadows possible values:
	//! -1 : disable and don't try to initialize
	//!  0 : disable, but still check if the hardware is able to run them
	//!  1 : enable (full detail)
	//!  2 : enable (no terrain)
	const int configValue = configHandler->Get("Shadows", 0);

	if (configValue >= 2)
		drawTerrainShadow = false;

	if (configValue < 0 || !haveShadowExts) {
		logOutput.Print("shadows disabled or required OpenGL extension missing");
		return;
	}

	shadowMapSize = configHandler->Get("ShadowMapSize", DEFAULT_SHADOWMAPSIZE);

	if (tmpFirstInstance) {
		// this already checks for GLEW_ARB_fragment_program
		if (!ProgramStringIsNative(GL_FRAGMENT_PROGRAM_ARB, "ARB/units3o.fp")) {
			logOutput.Print("Your GFX card does not support the fragment programs needed for shadows");
			return;
		}

		// this was previously set to true (redundantly since
		// it was actually never made false anywhere) if either
		//      1. (!GLEW_ARB_texture_env_crossbar && haveShadowExts)
		//      2. (!GLEW_ARB_shadow_ambient && GLEW_ARB_shadow)
		// but the non-FP result isn't nice anyway so just always
		// use the program if we are guaranteed of shadow support
		useFPShadows = true;

		if (!GLEW_ARB_shadow_ambient) {
			// can't use arbitrary texvals in case the depth comparison op fails (only 0)
			logOutput.Print("You are missing the \"ARB_shadow_ambient\" extension (this will probably make shadows darker than they should be)");
		}
	}

	if (!InitDepthTarget()) {
		return;
	}

	if (tmpFirstInstance) {
		canUseShadows = true;
	}

	if (configValue == 0) {
		// free any resources allocated by InitDepthTarget()
		glDeleteTextures(1, &shadowTexture);
		shadowTexture = 0;
		return; // drawShadows is still false
	}

	LoadShadowGenShaderProgs();
}

CShadowHandler::~CShadowHandler(void)
{
	if (drawShadows) {
		glDeleteTextures(1, &shadowTexture);
	}

	shaderHandler->ReleaseProgramObjects("[ShadowHandler]");
	shadowGenProgs.clear();
}



void CShadowHandler::LoadShadowGenShaderProgs()
{
	shadowGenProgs.resize(SHADOWGEN_PROGRAM_LAST);

	static const std::string shadowGenProgNames[SHADOWGEN_PROGRAM_LAST] = {
		"ARB/unit_genshadow.vp",
		"ARB/groundshadow.vp",
		"ARB/treeShadow.vp",
		"ARB/treeFarShadow.vp",
		"ARB/projectileshadow.vp",
	};
	static const std::string shadowGenProgHandles[SHADOWGEN_PROGRAM_LAST] = {
		"ShadowGenShaderProgModel",
		"ShadowGenshaderProgMap",
		"ShadowGenshaderProgTreeNear",
		"ShadowGenshaderProgTreeDist",
		"ShadowGenshaderProgProjectile",
	};
	static const std::string shadowGenProgDefines[SHADOWGEN_PROGRAM_LAST] = {
		"#define SHADOWGEN_PROGRAM_MODEL\n",
		"#define SHADOWGEN_PROGRAM_MAP\n",
		"#define SHADOWGEN_PROGRAM_TREE_NEAR\n",
		"#define SHADOWGEN_PROGRAM_TREE_DIST\n",
		"#define SHADOWGEN_PROGRAM_PROJECTILE\n",
	};

	CShaderHandler* sh = shaderHandler;

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "GLSL", false);
			Shader::IShaderObject* so = sh->CreateShaderObject("GLSL/ShadowGenVertProg.glsl", shadowGenProgDefines[i], GL_VERTEX_SHADER);

			po->AttachShaderObject(so);
			po->Link();
			po->SetUniformLocation("shadowParams");
			po->SetUniformLocation("cameraDirX");    // used by SHADOWGEN_PROGRAM_TREE_NEAR
			po->SetUniformLocation("cameraDirY");    // used by SHADOWGEN_PROGRAM_TREE_NEAR
			po->SetUniformLocation("treeOffset");    // used by SHADOWGEN_PROGRAM_TREE_NEAR

			shadowGenProgs[i] = po;

		}
	} else {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "ARB", true);
			Shader::IShaderObject* so = sh->CreateShaderObject(shadowGenProgNames[i], "", GL_VERTEX_PROGRAM_ARB);

			po->AttachShaderObject(so);
			po->Link();

			shadowGenProgs[i] = po;
		}
	}

	drawShadows = true;
}



bool CShadowHandler::InitDepthTarget()
{
	// this can be enabled for debugging
	// it turns the shadow render buffer in a buffer with color
	bool useColorTexture = false;
	if (!fb.IsValid()) {
		logOutput.Print("framebuffer not valid!");
		return false;
	}
	glGenTextures(1,&shadowTexture);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (useColorTexture) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	fb.Bind();
	if (useColorTexture)
		fb.AttachTexture(shadowTexture);
	else
		fb.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);
	int buffer = useColorTexture ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE;
	glDrawBuffer(buffer);
	glReadBuffer(buffer);
	bool status = fb.CheckStatus("SHADOW");
	fb.Unbind();
	return status;
}

void CShadowHandler::DrawShadowPasses(void)
{
	inShadowPass = true;

	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
		glEnable(GL_CULL_FACE);
	//	glCullFace(GL_FRONT);

		if (drawTerrainShadow)
			readmap->GetGroundDrawer()->DrawShadowPass();

		unitDrawer->DrawShadowPass();
		modelDrawer->Draw();
		featureDrawer->DrawShadowPass();
		treeDrawer->DrawShadowPass();
		eventHandler.DrawWorldShadow();
		projectileDrawer->DrawShadowPass();
	glPopAttrib();

	inShadowPass = false;
}

void CShadowHandler::SetShadowMapSizeFactors()
{
	if (shadowMapSize == 2048) {
		p17 =  0.01f;
		p18 = -0.1f;
	} else {
		p17 =  0.0025f;
		p18 = -0.05f;
	}
}

void CShadowHandler::CreateShadows(void)
{
	fb.Bind();

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColor4f(1, 1, 1, 1);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glViewport(0, 0, shadowMapSize, shadowMapSize);

	// glClearColor(0, 0, 0, 0);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, 0, -1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	cross1 = (mapInfo->light.sunDir.cross(UpVector)).ANormalize();
	cross2 = cross1.cross(mapInfo->light.sunDir);
	centerPos = camera->pos;

	//! derive the size of the shadow-map from the
	//! intersection points of the camera frustum
	//! with the xz-plane
	CalcMinMaxView();
	SetShadowMapSizeFactors();

	// it should be possible to tweak a bit more shadow map resolution from this
	const float maxLength = 12000.0f;
	const float maxLengthX = (x2 - x1) * 1.5f;
	const float maxLengthY = (y2 - y1) * 1.5f;

	xmid = 1.0f - (sqrt(fabs(x2)) / (sqrt(fabs(x2)) + sqrt(fabs(x1))));
	ymid = 1.0f - (sqrt(fabs(y2)) / (sqrt(fabs(y2)) + sqrt(fabs(y1))));

	shadowMatrix[ 0] =   cross1.x / maxLengthX;
	shadowMatrix[ 4] =   cross1.y / maxLengthX;
	shadowMatrix[ 8] =   cross1.z / maxLengthX;
	shadowMatrix[12] = -(cross1.dot(centerPos)) / maxLengthX;
	shadowMatrix[ 1] =   cross2.x / maxLengthY;
	shadowMatrix[ 5] =   cross2.y / maxLengthY;
	shadowMatrix[ 9] =   cross2.z / maxLengthY;
	shadowMatrix[13] = -(cross2.dot(centerPos)) / maxLengthY;
	shadowMatrix[ 2] = -mapInfo->light.sunDir.x / maxLength;
	shadowMatrix[ 6] = -mapInfo->light.sunDir.y / maxLength;
	shadowMatrix[10] = -mapInfo->light.sunDir.z / maxLength;
	shadowMatrix[14] = ((centerPos.x * mapInfo->light.sunDir.x + centerPos.z * mapInfo->light.sunDir.z) / maxLength) + 0.5f;

	glLoadMatrixf(shadowMatrix.m);

	//! set the shadow-parameter registers
	//! NOTE: so long as any part of Spring rendering still uses
	//! ARB programs at run-time, these lines can not be removed
	//! (all ARB programs share the same environment)
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, xmid, ymid, 0.0f, 0.0f);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17,  p17,  p17, 0.0f, 0.0f);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 18,  p18,  p18, 0.0f, 0.0f);

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			shadowGenProgs[i]->Enable();
			shadowGenProgs[i]->SetUniform4f(0, xmid, ymid, p17, p18);
			shadowGenProgs[i]->Disable();
		}
	}

	//! move view into sun-space
	float3 oldup = camera->up;
	camera->right = cross1;
	camera->up = cross2;
	camera->pos2 = camera->pos + mapInfo->light.sunDir * 8000;

	DrawShadowPasses();

	camera->up = oldup;
	camera->pos2 = camera->pos;

	shadowMatrix[14] -= 0.00001f;

	glShadeModel(GL_SMOOTH);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//we do this later to save render context switches (this is one of the slowest opengl operations!)
	//fb.Unbind();
	//glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
}


void CShadowHandler::DrawShadowTex(void)
{
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glColor3f(1,1,1);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,shadowTexture);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	float3 verts[] = {
		float3(0,0,1),
		float3(0, 0.5f, 1),
		float3(0.5f, 0.5f, 1.f),
		float3(0.5f, 0, 1),
	};

	GLfloat texcoords[] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 1.f,
		1.f, 0.f,
	};

	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glVertexPointer(3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}



void CShadowHandler::CalcMinMaxView(void)
{
	left.clear();

	// add restraints for camera frustum planes
	GetFrustumSide(cam2->bottom, false);
	GetFrustumSide(cam2->top, true);
	GetFrustumSide(cam2->rightside, false);
	GetFrustumSide(cam2->leftside, false);

	std::vector<fline>::iterator fli,fli2;
	for (fli = left.begin(); fli != left.end(); fli++) {
		for (fli2 = left.begin(); fli2 != left.end(); fli2++) {
			if (fli == fli2)
				continue;
			if (fli->dir - fli2->dir == 0.0f)
				continue;

			// z-intersection distance
			const float colz = -(fli->base - fli2->base) / (fli->dir - fli2->dir);

			if (fli2->left * (fli->dir - fli2->dir) > 0.0f) {
				if ((colz > fli->minz) && (colz < gs->mapy * SQUARE_SIZE + 20000.0f))
					fli->minz = colz;
			} else {
				if ((colz < fli->maxz) && (colz > -20000.0f))
					fli->maxz = colz;
			}
		}
	}

	x1 = -100;
	x2 =  100;
	y1 = -100;
	y2 =  100;

	//if someone could figure out how the frustum and nonlinear shadow transform really works (and not use the SJan trial and error method)
	//so that we can skip this sort of fudge factors it would be good
	float borderSize = 270;
	float maxSize = globalRendering->viewRange * 0.75f;

	if (shadowMapSize == 1024) {
		borderSize *= 1.5f;
		maxSize *= 1.2f;
	}

	if (!left.empty()) {
		std::vector<fline>::iterator fli;
		for (fli = left.begin(); fli != left.end(); fli++) {
			if (fli->minz < fli->maxz) {
				float3 p[5];
				p[0] = float3(fli->base + fli->dir * fli->minz, 0.0, fli->minz);
				p[1] = float3(fli->base + fli->dir * fli->maxz, 0.0f, fli->maxz);
				p[2] = float3(fli->base + fli->dir * fli->minz, readmap->maxheight + 200, fli->minz);
				p[3] = float3(fli->base + fli->dir * fli->maxz, readmap->maxheight + 200, fli->maxz);
				p[4] = float3(camera->pos.x, 0.0f, camera->pos.z);

				for (int a = 0; a < 5; ++a) {
					float xd = (p[a] - centerPos).dot(cross1);
					float yd = (p[a] - centerPos).dot(cross2);
					if (xd + borderSize > x2) { x2 = xd + borderSize; }
					if (xd - borderSize < x1) { x1 = xd - borderSize; }
					if (yd + borderSize > y2) { y2 = yd + borderSize; }
					if (yd - borderSize < y1) { y1 = yd - borderSize; }
				}
			}
		}

		if (x1 < -maxSize) { x1 = -maxSize; }
		if (x2 >  maxSize) { x2 =  maxSize; }
		if (y1 < -maxSize) { y1 = -maxSize; }
		if (y2 >  maxSize) { y2 =  maxSize; }
	} else {
		x1 = -maxSize;
		x2 =  maxSize;
		y1 = -maxSize;
		y2 =  maxSize;
	}
}



//maybe standardize all these things in one place sometime (and maybe one day i should try to understand how i made them work)
void CShadowHandler::GetFrustumSide(float3& side, bool upside)
{
	fline temp;

	// get vector for collision between frustum and horizontal plane
	float3 b = UpVector.cross(side);

	if (fabs(b.z) < 0.0001f)
		b.z = 0.00011f;
	if (fabs(b.z) > 0.0001f) {
		temp.dir=b.x/b.z;				//set direction to that
		float3 c=b.cross(side);			//get vector from camera to collision line
		c.ANormalize();
		float3 colpoint;				//a point on the collision line

		if(side.y>0){
			if(b.dot(UpVector.cross(cam2->forward))<0 && upside){
				colpoint=cam2->pos+cam2->forward*20000;
				//logOutput.Print("upward frustum");
			}else
				colpoint=cam2->pos-c*((cam2->pos.y)/c.y);
		}else
			colpoint=cam2->pos-c*((cam2->pos.y)/c.y);

		temp.base=colpoint.x-colpoint.z*temp.dir;	//get intersection between colpoint and z axis
		temp.left=-1;
		if(b.z>0)
			temp.left=1;
		if(side.y>0 && (b.dot(UpVector.cross(cam2->forward))<0 && upside))
			temp.left*=-1;
		temp.maxz=gs->mapy*SQUARE_SIZE+500;
		temp.minz=-500;
		left.push_back(temp);
	}
}
