/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"
#include <cfloat>

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/ModelDrawer.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"

#define DEFAULT_SHADOWMAPSIZE 2048
#define SHADOWMATRIX_NONLINEAR 0
#define USE_CAMERA_PROJECTION_CENTER 0

CONFIG(int, Shadows).defaultValue(0);
CONFIG(int, ShadowMapSize).defaultValue(DEFAULT_SHADOWMAPSIZE);

CShadowHandler* shadowHandler = NULL;

bool CShadowHandler::shadowsSupported = false;
bool CShadowHandler::firstInit = true;


void CShadowHandler::Reload(const char* args)
{
	int nextShadowConfig = (shadowConfig + 1) & 0xF;
	int nextShadowMapSize = DEFAULT_SHADOWMAPSIZE;

	if (args != NULL) {
		if (sscanf(args, "%i %i", &nextShadowConfig, &nextShadowMapSize) > 1) {
			configHandler->Set("ShadowMapSize", nextShadowMapSize);
		}
	}

	configHandler->Set("Shadows", nextShadowConfig & 0xF);

	Kill();
	Init();
}

void CShadowHandler::Init()
{
	const bool tmpFirstInit = firstInit;
	firstInit = false;

	shadowConfig = configHandler->GetInt("Shadows");
	shadowMapSize = configHandler->GetInt("ShadowMapSize");
	shadowModeMask = SHADOWMODE_NONE;

	shadowsLoaded = false;
	inShadowPass = false;

	shadowTexture = 0;
	dummyColorTexture = 0;

	if (!tmpFirstInit && !shadowsSupported) {
		return;
	}

	// possible values for the "Shadows" config-parameter:
	// < 0: disable and don't try to initialize
	//   0: disable, but still check if the hardware is able to run them
	// > 0: enabled (by default for all shadow-casting geometry if equal to 1)
	if (shadowConfig < 0) {
		LOG("[%s] shadow rendering is disabled (config-value %d)", __FUNCTION__, shadowConfig);
		return;
	}

	if (shadowConfig > 0)
		shadowModeMask = SHADOWMODE_MODEL | SHADOWMODE_MAP | SHADOWMODE_PROJ | SHADOWMODE_TREE;

	if (shadowConfig > 1) {
		if ((shadowConfig & SHADOWMODE_MODEL) == 0) { shadowModeMask &= (~SHADOWMODE_MODEL); }
		if ((shadowConfig & SHADOWMODE_MAP  ) == 0) { shadowModeMask &= (~SHADOWMODE_MAP  ); }
		if ((shadowConfig & SHADOWMODE_PROJ ) == 0) { shadowModeMask &= (~SHADOWMODE_PROJ ); }
		if ((shadowConfig & SHADOWMODE_TREE ) == 0) { shadowModeMask &= (~SHADOWMODE_TREE ); }
	}


	if (!globalRendering->haveARB && !globalRendering->haveGLSL) {
		LOG_L(L_WARNING, "[%s] GPU does not support either ARB or GLSL shaders for shadow rendering", __FUNCTION__);
		return;
	}

	if (!globalRendering->haveGLSL) {
		if (!GLEW_ARB_shadow || !GLEW_ARB_depth_texture || !GLEW_ARB_texture_env_combine) {
			LOG_L(L_WARNING, "[%s] required OpenGL ARB-extensions missing for shadow rendering", __FUNCTION__);
			// NOTE: these should only be relevant for FFP shadows
			// return;
		}
		if (!GLEW_ARB_shadow_ambient) {
			// can't use arbitrary texvals in case the depth comparison op fails (only 0)
			LOG_L(L_WARNING, "[%s] \"ARB_shadow_ambient\" extension missing (will probably make shadows darker than they should be)", __FUNCTION__);
		}
	}


	if (!InitDepthTarget()) {
		LOG_L(L_ERROR, "[%s] failed to initialize depth-texture FBO", __FUNCTION__);
		return;
	}

	if (tmpFirstInit) {
		shadowsSupported = true;
	}

	if (shadowConfig == 0) {
		// free any resources allocated by InitDepthTarget()
		glDeleteTextures(1, &shadowTexture    ); shadowTexture     = 0;
		glDeleteTextures(1, &dummyColorTexture); dummyColorTexture = 0;
		// shadowsLoaded is still false
		return;
	}

	LoadShadowGenShaderProgs();
}

void CShadowHandler::Kill()
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
			po->Validate();

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
		LOG_L(L_ERROR, "[%s] framebuffer not valid", __FUNCTION__);
		return false;
	}
	glGenTextures(1,&shadowTexture);

	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
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
	} else {
		if (globalRendering->atiHacks)
			fb.AttachTexture(dummyColorTexture);
		fb.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);
	}

	const int buffer = (useColorTexture || globalRendering->atiHacks) ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE;
	glDrawBuffer(buffer);
	glReadBuffer(buffer);
	const bool status = fb.CheckStatus("SHADOW");
	fb.Unbind();
	return status;
}

void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		// cull front-faces during the terrain shadow pass: sun direction
		// can be set so oblique that geometry back-faces are visible (eg.
		// from hills near map edges) from its POV
		// (could just disable culling of terrain faces, but we also want
		// to prevent overdraw in such low-angle passes)
		if ((shadowModeMask & SHADOWMODE_MAP) != 0)
			readmap->GetGroundDrawer()->DrawShadowPass();

		glCullFace(GL_BACK);

		if ((shadowModeMask & SHADOWMODE_MODEL) != 0) {
			unitDrawer->DrawShadowPass();
			modelDrawer->Draw();
			featureDrawer->DrawShadowPass();
		}

		if ((shadowModeMask & SHADOWMODE_TREE) != 0)
			treeDrawer->DrawShadowPass();

		if ((shadowModeMask & SHADOWMODE_PROJ) != 0)
			projectileDrawer->DrawShadowPass();

		eventHandler.DrawWorldShadow();
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
	sunDirY = (sunDirX.cross(sunDirZ)).ANormalize();

	#if (USE_CAMERA_PROJECTION_CENTER == 1)
	centerPos = camera->pos;
	#else
	// use map-center as projection target rather than camera position,
	// because camera can be in a map corner looking at an area of the
	// map that falls outside the sun frustum when variable scaling is
	// used
	centerPos.x = (gs->mapx * SQUARE_SIZE) * 0.5f;
	centerPos.z = (gs->mapy * SQUARE_SIZE) * 0.5f;
	#endif
	centerPos.y = 0.0f;

	// derive the size of the shadow-map from the
	// intersection points of the camera frustum
	// with the xz-plane
	// CalcMinMaxView();
	SetShadowMapSizeFactors();

	// FIXME:
	//     these scaling factors do not change linearly or smoothly with
	//     camera movements, creating visible artefacts (resolution jumps)
	//     therefore, use fixed values such that the entire map barely fits
	//     into the sun's frustum: pretend the map is embedded in a sphere
	//     and take its radius as the scale factor
	//     this means larger maps will have more blurred/aliased shadows if
	//     the depth buffer is kept at the same size
	//     in the ideal case, the zoom-factor should be such that everything
	//     that can be seen by the camera maximally fills the sun's frustum
	//     (and nothing is left out), but CalcMinMaxView fails to achieve this
	// NOTE:
	//     when DynamicSun is enabled, the orbit is always circular in the xz
	//     plane, instead of elliptical when the map has an aspect-ratio != 1
	//
	// const float xScale = (shadowProjMinMax.y - shadowProjMinMax.x) * 1.5f;
	// const float yScale = (shadowProjMinMax.w - shadowProjMinMax.z) * 1.5f;
	const float zScale = GetOrthoProjectedMapRadius(-sunDirZ);
	const float xScale = zScale;
	const float yScale = zScale;

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

	shadowMatrix[ 0] = sunDirX.x / xScale;
	shadowMatrix[ 1] = sunDirY.x / yScale;
	shadowMatrix[ 2] = sunDirZ.x / zScale;

	shadowMatrix[ 4] = sunDirX.y / xScale;
	shadowMatrix[ 5] = sunDirY.y / yScale;
	shadowMatrix[ 6] = sunDirZ.y / zScale;

	shadowMatrix[ 8] = sunDirX.z / xScale;
	shadowMatrix[ 9] = sunDirY.z / yScale;
	shadowMatrix[10] = sunDirZ.z / zScale;

	// rotate the target position into sun-space for the translation
	shadowMatrix[12] = (-sunDirX.dot(centerPos) / xScale);
	shadowMatrix[13] = (-sunDirY.dot(centerPos) / yScale);
	shadowMatrix[14] = (-sunDirZ.dot(centerPos) / zScale) + 0.5f;

	glLoadMatrixf(shadowMatrix.m);

	// set the shadow-parameter registers
	// NOTE: so long as any part of Spring rendering still uses
	// ARB programs at run-time, these lines can not be removed
	// (all ARB programs share the same environment)
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
		// move view into sun-space
		const float3 oldup = camera->up;

		camera->right = sunDirX;
		camera->up = sunDirY;

		DrawShadowPasses();

		camera->up = oldup;
	}

	glShadeModel(GL_SMOOTH);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//we do this later to save render context switches (this is one of the slowest opengl operations!)
	//fb.Unbind();
	//glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
}


void CShadowHandler::CalcMinMaxView()
{
	cam2->GetFrustumSides(0.0f, 0.0f, 1.0f, true);
	cam2->ClipFrustumLines(true, -20000.0f, gs->mapy * SQUARE_SIZE + 20000.0f);

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

	const std::vector<CCamera::FrustumLine>& negSides = cam2->negFrustumSides;
	const std::vector<CCamera::FrustumLine>& posSides = cam2->posFrustumSides;
	std::vector<CCamera::FrustumLine>::const_iterator fli;

	if (!negSides.empty()) {
		for (fli = negSides.begin(); fli != negSides.end(); ++fli) {
			if (fli->minz < fli->maxz) {
				float3 p[5];
				p[0] = float3(fli->base + fli->dir * fli->minz, 0.0f, fli->minz);
				p[1] = float3(fli->base + fli->dir * fli->maxz, 0.0f, fli->maxz);
				p[2] = float3(fli->base + fli->dir * fli->minz, readmap->initMaxHeight + 200, fli->minz);
				p[3] = float3(fli->base + fli->dir * fli->maxz, readmap->initMaxHeight + 200, fli->maxz);
				p[4] = centerPos;

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

float CShadowHandler::GetOrthoProjectedMapRadius(const float3& sunDir) const {
	// to fit the map inside the frustum, we need to know
	// the distance from one corner to its opposing corner
	//
	// this distance is maximal when the sun direction is
	// orthogonal to the diagonal, but in other cases we
	// can gain some precision by projecting the diagonal
	// onto a vector orthogonal to the sun direction and
	// using the length of that projected vector instead
	//
	// note: "radius" is actually the diameter
	static const float maxMapRadius = math::sqrtf(Square(gs->mapx * SQUARE_SIZE) + Square(gs->mapy * SQUARE_SIZE));
	static       float curMapRadius = 0.0f;

	static float3 sunDir3D = ZeroVector;

	if ((sunDir3D != sunDir)) {
		float3 sunDir2D;
		float3 mapVerts[2];

		sunDir3D   = sunDir;
		sunDir2D.x = sunDir3D.x;
		sunDir2D.z = sunDir3D.z;
		sunDir2D.ANormalize();

		if (sunDir2D.x >= 0.0f) {
			if (sunDir2D.z >= 0.0f) {
				// use diagonal vector from top-right to bottom-left
				mapVerts[0] = float3(gs->mapx * SQUARE_SIZE, 0.0f,                   0.0f);
				mapVerts[1] = float3(                  0.0f, 0.0f, gs->mapy * SQUARE_SIZE);
			} else {
				// use diagonal vector from top-left to bottom-right
				mapVerts[0] = float3(                  0.0f, 0.0f,                   0.0f);
				mapVerts[1] = float3(gs->mapx * SQUARE_SIZE, 0.0f, gs->mapy * SQUARE_SIZE);
			}
		} else {
			if (sunDir2D.z >= 0.0f) {
				// use diagonal vector from bottom-right to top-left
				mapVerts[0] = float3(gs->mapx * SQUARE_SIZE, 0.0f, gs->mapy * SQUARE_SIZE);
				mapVerts[1] = float3(                  0.0f, 0.0f,                   0.0f);
			} else {
				// use diagonal vector from bottom-left to top-right
				mapVerts[0] = float3(                  0.0f, 0.0f, gs->mapy * SQUARE_SIZE);
				mapVerts[1] = float3(gs->mapx * SQUARE_SIZE, 0.0f,                   0.0f);
			}
		}

		const float3 v1 = (mapVerts[1] - mapVerts[0]).ANormalize();
		const float3 v2 = float3(-sunDir2D.z, 0.0f, sunDir2D.x);

		curMapRadius = maxMapRadius * v2.dot(v1);
	}

	return curMapRadius;
}
