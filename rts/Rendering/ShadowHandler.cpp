/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include <cfloat>

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/ModelDrawer.hpp"
#include "Rendering/Shaders/ShaderHandler.hpp"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/LogOutput.h"

#define DEFAULT_SHADOWMAPSIZE 2048
#define SHADOWMATRIX_NONLINEAR 0

CONFIG(Shadows, 0);
CONFIG(ShadowMapSize, DEFAULT_SHADOWMAPSIZE);

CShadowHandler* shadowHandler = NULL;

bool CShadowHandler::shadowsSupported = false;
bool CShadowHandler::firstInstance = true;


CShadowHandler::CShadowHandler()
{
	const bool tmpFirstInstance = firstInstance;
	firstInstance = false;

	shadowsLoaded = false;
	inShadowPass = false;
	shadowTexture = 0;
	dummyColorTexture = 0;
	drawTerrainShadow = true;

	if (!tmpFirstInstance && !shadowsSupported) {
		return;
	}

	//! Shadows possible values:
	//! -1 : disable and don't try to initialize
	//!  0 : disable, but still check if the hardware is able to run them
	//!  1 : enable (full detail)
	//!  2 : enable (no terrain)
	const int configValue = configHandler->GetInt("Shadows");

	if (configValue >= 2)
		drawTerrainShadow = false;

	if (configValue < 0) {
		logOutput.Print("[%s] shadow rendering is disabled (config-value %d)", __FUNCTION__, configValue);
		return;
	}

	if (!globalRendering->haveARB && !globalRendering->haveGLSL) {
		logOutput.Print("[%s] GPU does not support either ARB or GLSL shaders for shadow rendering", __FUNCTION__);
		return;
	}

	if (!globalRendering->haveGLSL) {
		if (!GLEW_ARB_shadow || !GLEW_ARB_depth_texture || !GLEW_ARB_texture_env_combine) {
			logOutput.Print("[%s] required OpenGL ARB-extensions missing for shadow rendering", __FUNCTION__);
			// NOTE: these should only be relevant for FFP shadows
			// return;
		}
		if (!GLEW_ARB_shadow_ambient) {
			// can't use arbitrary texvals in case the depth comparison op fails (only 0)
			logOutput.Print("[%s] \"ARB_shadow_ambient\" extension missing (will probably make shadows darker than they should be)", __FUNCTION__);
		}
	}

	shadowMapSize = configHandler->GetInt("ShadowMapSize");

	if (!InitDepthTarget()) {
		logOutput.Print("[%s] failed to initialize depth-texture FBO", __FUNCTION__);
		return;
	}

	if (tmpFirstInstance) {
		shadowsSupported = true;
	}

	if (configValue == 0) {
		// free any resources allocated by InitDepthTarget()
		glDeleteTextures(1, &shadowTexture);
		shadowTexture = 0;
		glDeleteTextures(1, &dummyColorTexture);
		dummyColorTexture = 0;
		return; // shadowsLoaded is still false
	}

	LoadShadowGenShaderProgs();
}

CShadowHandler::~CShadowHandler()
{
	if (shadowsLoaded) {
		glDeleteTextures(1, &shadowTexture);
		glDeleteTextures(1, &dummyColorTexture);
	}

	shaderHandler->ReleaseProgramObjects("[ShadowHandler]");
	shadowGenProgs.clear();
}



void CShadowHandler::LoadShadowGenShaderProgs()
{
	#define sh shaderHandler
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

	static const std::string extraDef =
	#if (SHADOWMATRIX_NONLINEAR == 1)
		"#define SHADOWMATRIX_NONLINEAR 0\n";
	#else
		"#define SHADOWMATRIX_NONLINEAR 1\n";
	#endif

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "GLSL", false);
			Shader::IShaderObject* so = sh->CreateShaderObject("GLSL/ShadowGenVertProg.glsl", shadowGenProgDefines[i] + extraDef, GL_VERTEX_SHADER);

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

	shadowsLoaded = true;
	#undef sh
}



bool CShadowHandler::InitDepthTarget()
{
	// this can be enabled for debugging
	// it turns the shadow render buffer in a buffer with color
	bool useColorTexture = false;
	if (!fb.IsValid()) {
		logOutput.Print("[%s] framebuffer not valid!", __FUNCTION__);
		return false;
	}
	glGenTextures(1,&shadowTexture);

	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	float one[4] = {1.0f,1.0f,1.0f,1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (useColorTexture) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	} else {
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}

	glGenTextures(1,&dummyColorTexture);
	if (globalRendering->atiHacks) {
		// ATI shadows fail without an attached color texture
		glBindTexture(GL_TEXTURE_2D, dummyColorTexture);
		// this dummy should be as small as possible not to waste memory
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA4, shadowMapSize, shadowMapSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	fb.Bind();
	if (useColorTexture) {
		fb.AttachTexture(shadowTexture);
	}
	else {
		if (globalRendering->atiHacks)
			fb.AttachTexture(dummyColorTexture);
		fb.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);
	}
	int buffer = (useColorTexture || globalRendering->atiHacks) ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE;
	glDrawBuffer(buffer);
	glReadBuffer(buffer);
	bool status = fb.CheckStatus("SHADOW");
	fb.Unbind();
	return status;
}

void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		if (drawTerrainShadow) {
			readmap->GetGroundDrawer()->DrawShadowPass();
		}

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
	#if (SHADOWMATRIX_NONLINEAR == 1)
	if (shadowMapSize >= 2048) {
		shadowProjCenter.z =  0.01f;
		shadowProjCenter.w = -0.1f;
	} else {
		shadowProjCenter.z =  0.0025f;
		shadowProjCenter.w = -0.05f;
	}
	#else
	shadowProjCenter.z = FLT_MAX;
	shadowProjCenter.w = 1.0f;
	#endif
}

void CShadowHandler::CreateShadows()
{
	fb.Bind();

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColor4f(1, 1, 1, 1);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	glViewport(0, 0, shadowMapSize, shadowMapSize);

	// glClearColor(0, 0, 0, 0);
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, 0, -1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	const ISkyLight* L = sky->GetLight();

	// sun direction is in world-space, invert it
	sunDirZ = -L->GetLightDir();
	sunDirX = (sunDirZ.cross(UpVector)).ANormalize();
	sunDirY = sunDirX.cross(sunDirZ);
	centerPos = camera->pos;

	//! derive the size of the shadow-map from the
	//! intersection points of the camera frustum
	//! with the xz-plane
	CalcMinMaxView();
	SetShadowMapSizeFactors();

	// it should be possible to tweak a bit more shadow map resolution from this
	static const float Z_DIVIDE = 12000.0f;
	static const float Z_OFFSET = 0.00001f;
	static const float Z_LENGTH = 8000.0f;

	const float maxLengthX = (shadowProjMinMax.y - shadowProjMinMax.x) * 1.5f;
	const float maxLengthY = (shadowProjMinMax.w - shadowProjMinMax.z) * 1.5f;

	#if (SHADOWMATRIX_NONLINEAR == 1)
	const float shadowMapX =              sqrt( fabs(shadowProjMinMax.y) ); // sqrt( |x2| )
	const float shadowMapY =              sqrt( fabs(shadowProjMinMax.w) ); // sqrt( |y2| )
	const float shadowMapW = shadowMapX + sqrt( fabs(shadowProjMinMax.x) ); // sqrt( |x2| ) + sqrt( |x1| )
	const float shadowMapH = shadowMapY + sqrt( fabs(shadowProjMinMax.z) ); // sqrt( |y2| ) + sqrt( |y1| )

	shadowProjCenter.x = 1.0f - (shadowMapX / shadowMapW);
	shadowProjCenter.y = 1.0f - (shadowMapY / shadowMapH);
	#else
	shadowProjCenter.x = 0.5f;
	shadowProjCenter.y = 0.5f;
	#endif

	shadowMatrix[ 0] = sunDirX.x / maxLengthX;
	shadowMatrix[ 1] = sunDirY.x / maxLengthY;
	shadowMatrix[ 2] = sunDirZ.x / Z_DIVIDE;

	shadowMatrix[ 4] = sunDirX.y / maxLengthX;
	shadowMatrix[ 5] = sunDirY.y / maxLengthY;
	shadowMatrix[ 6] = sunDirZ.y / Z_DIVIDE;

	shadowMatrix[ 8] = sunDirX.z / maxLengthX;
	shadowMatrix[ 9] = sunDirY.z / maxLengthY;
	shadowMatrix[10] = sunDirZ.z / Z_DIVIDE;

	// rotate the camera position into sun-space for the translation
	shadowMatrix[12] = -(sunDirX.dot(centerPos)) / maxLengthX;
	shadowMatrix[13] = -(sunDirY.dot(centerPos)) / maxLengthY;
	shadowMatrix[14] = -(sunDirZ.dot(centerPos)  / Z_DIVIDE) + 0.5f;

	glLoadMatrixf(shadowMatrix.m);

	//! set the shadow-parameter registers
	//! NOTE: so long as any part of Spring rendering still uses
	//! ARB programs at run-time, these lines can not be removed
	//! (all ARB programs share the same environment)
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, shadowProjCenter.x, shadowProjCenter.y, 0.0f, 0.0f);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17, shadowProjCenter.z, shadowProjCenter.z, 0.0f, 0.0f);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 18, shadowProjCenter.w, shadowProjCenter.w, 0.0f, 0.0f);

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			shadowGenProgs[i]->Enable();
			shadowGenProgs[i]->SetUniform4fv(0, &shadowProjCenter.x);
			shadowGenProgs[i]->Disable();
		}
	}

	if (L->GetLightIntensity() > 0.0f) {
		//! move view into sun-space
		const float3 oldup = camera->up;

		camera->right = sunDirX;
		camera->up = sunDirY;
		camera->pos2 = camera->pos - sunDirZ * Z_LENGTH;

		DrawShadowPasses();

		camera->up = oldup;
		camera->pos2 = camera->pos;
	}

	shadowMatrix[14] -= Z_OFFSET;

	glShadeModel(GL_SMOOTH);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//we do this later to save render context switches (this is one of the slowest opengl operations!)
	//fb.Unbind();
	//glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
}


void CShadowHandler::CalcMinMaxView()
{
	left.clear();

	// add restraints for camera frustum planes
	GetFrustumSide(cam2->bottom, false);
	GetFrustumSide(cam2->top, true);
	GetFrustumSide(cam2->rightside, false);
	GetFrustumSide(cam2->leftside, false);

	std::vector<fline>::iterator fli,fli2;
	for (fli = left.begin(); fli != left.end(); ++fli) {
		for (fli2 = left.begin(); fli2 != left.end(); ++fli2) {
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

	shadowProjMinMax.x = -100.0f;
	shadowProjMinMax.y =  100.0f;
	shadowProjMinMax.z = -100.0f;
	shadowProjMinMax.w =  100.0f;

	//if someone could figure out how the frustum and nonlinear shadow transform really works (and not use the SJan trial and error method)
	//so that we can skip this sort of fudge factors it would be good
	float borderSize = 270.0f;
	float maxSize = globalRendering->viewRange * 0.75f;

	if (shadowMapSize == 1024) {
		borderSize *= 1.5f;
		maxSize *= 1.2f;
	}

	if (!left.empty()) {
		std::vector<fline>::iterator fli;
		for (fli = left.begin(); fli != left.end(); ++fli) {
			if (fli->minz < fli->maxz) {
				float3 p[5];
				p[0] = float3(fli->base + fli->dir * fli->minz, 0.0, fli->minz);
				p[1] = float3(fli->base + fli->dir * fli->maxz, 0.0f, fli->maxz);
				p[2] = float3(fli->base + fli->dir * fli->minz, readmap->maxheight + 200, fli->minz);
				p[3] = float3(fli->base + fli->dir * fli->maxz, readmap->maxheight + 200, fli->maxz);
				p[4] = float3(camera->pos.x, 0.0f, camera->pos.z);

				for (int a = 0; a < 5; ++a) {
					const float xd = (p[a] - centerPos).dot(sunDirX);
					const float yd = (p[a] - centerPos).dot(sunDirY);

					if (xd + borderSize > shadowProjMinMax.y) { shadowProjMinMax.y = xd + borderSize; }
					if (xd - borderSize < shadowProjMinMax.x) { shadowProjMinMax.x = xd - borderSize; }
					if (yd + borderSize > shadowProjMinMax.w) { shadowProjMinMax.w = yd + borderSize; }
					if (yd - borderSize < shadowProjMinMax.z) { shadowProjMinMax.z = yd - borderSize; }
				}
			}
		}

		if (shadowProjMinMax.x < -maxSize) { shadowProjMinMax.x = -maxSize; }
		if (shadowProjMinMax.y >  maxSize) { shadowProjMinMax.y =  maxSize; }
		if (shadowProjMinMax.z < -maxSize) { shadowProjMinMax.z = -maxSize; }
		if (shadowProjMinMax.w >  maxSize) { shadowProjMinMax.w =  maxSize; }
	} else {
		shadowProjMinMax.x = -maxSize;
		shadowProjMinMax.y =  maxSize;
		shadowProjMinMax.z = -maxSize;
		shadowProjMinMax.w =  maxSize;
	}
}



//maybe standardize all these things in one place sometime (and maybe one day i should try to understand how i made them work)
void CShadowHandler::GetFrustumSide(float3& side, bool upside)
{

	// get vector for collision between frustum and horizontal plane
	float3 b = UpVector.cross(side);

	if (fabs(b.z) < 0.0001f)
		b.z = 0.00011f;
	if (fabs(b.z) > 0.0001f) {
		fline temp;
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
