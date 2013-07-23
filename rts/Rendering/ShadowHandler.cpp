/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cfloat>

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Game/GameVersion.h"
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

#define SHADOWMATRIX_NONLINEAR      0

CONFIG(int, Shadows).defaultValue(2).minimumValue(0).description("Sets whether shadows are rendered.\n0:=off, 1:=full, 2:=fast (skip terrain)"); //FIXME document bitmask
CONFIG(int, ShadowMapSize).defaultValue(CShadowHandler::DEF_SHADOWMAP_SIZE).minimumValue(32).description("Sets the resolution of shadows. Higher numbers increase quality at the cost of performance.");
CONFIG(int, ShadowProjectionMode).defaultValue(CShadowHandler::SHADOWPROMODE_CAM_CENTER);

CShadowHandler* shadowHandler = NULL;

bool CShadowHandler::shadowsSupported = false;
bool CShadowHandler::firstInit = true;


void CShadowHandler::Reload(const char* argv)
{
	int nextShadowConfig = (shadowConfig + 1) & 0xF;
	int nextShadowMapSize = shadowMapSize;
	int nextShadowProMode = shadowProMode;

	if (argv != NULL) {
		(void) sscanf(argv, "%i %i %i", &nextShadowConfig, &nextShadowMapSize, &nextShadowProMode);
	}

	configHandler->Set("Shadows", nextShadowConfig & 0xF);
	configHandler->Set("ShadowMapSize", Clamp(nextShadowMapSize, int(MIN_SHADOWMAP_SIZE), int(MAX_SHADOWMAP_SIZE)));
	configHandler->Set("ShadowProjectionMode", Clamp(nextShadowProMode, int(SHADOWPROMODE_MAP_CENTER), int(SHADOWPROMODE_MIX_CAMMAP)));

	Kill();
	Init();
}

void CShadowHandler::Init()
{
	const bool tmpFirstInit = firstInit;
	firstInit = false;

	shadowConfig  = configHandler->GetInt("Shadows");
	shadowMapSize = configHandler->GetInt("ShadowMapSize");
	shadowProMode = configHandler->GetInt("ShadowProjectionMode");
	shadowGenBits = SHADOWGEN_BIT_NONE;

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
		shadowGenBits = SHADOWGEN_BIT_MODEL | SHADOWGEN_BIT_MAP | SHADOWGEN_BIT_PROJ | SHADOWGEN_BIT_TREE;

	if (shadowConfig > 1) {
		shadowGenBits &= (~shadowConfig);
	}

	// no warnings when running headless
	if (SpringVersion::IsHeadless())
		return;

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
		// free any resources allocated by InitDepthTarget()
		FreeTextures();

		LOG_L(L_ERROR, "[%s] failed to initialize depth-texture FBO", __FUNCTION__);
		return;
	}

	if (tmpFirstInit) {
		shadowsSupported = true;
	}

	if (shadowConfig == 0) {
		// free any resources allocated by InitDepthTarget()
		FreeTextures();

		// shadowsLoaded is still false
		return;
	}

	LoadShadowGenShaderProgs();
}

void CShadowHandler::Kill()
{
	FreeTextures();
	shaderHandler->ReleaseProgramObjects("[ShadowHandler]");
	shadowGenProgs.clear();
}

void CShadowHandler::FreeTextures() {
	if (fb.IsValid())
		fb.DetachAll();

	glDeleteTextures(1, &shadowTexture    ); shadowTexture     = 0;
	glDeleteTextures(1, &dummyColorTexture); dummyColorTexture = 0;
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

	fb.Bind();

	glGenTextures(1, &shadowTexture);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	const float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // shadowtex linear sampling is for-free on NVidias
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (useColorTexture) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		fb.AttachTexture(shadowTexture);
	} else {
		const GLint texFormat = GL_DEPTH_COMPONENT32;
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, texFormat, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		fb.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);
	}

	// test the FBO
	glDrawBuffer(useColorTexture ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE);
	glDrawBuffer(useColorTexture ? GL_COLOR_ATTACHMENT0_EXT : GL_NONE);
	bool status = fb.CheckStatus("SHADOW");
	if (!status && !useColorTexture) {
		status = WorkaroundUnsupportedFboRenderTargets();
	}
	fb.Unbind();
	return status;
}


bool CShadowHandler::WorkaroundUnsupportedFboRenderTargets()
{
	bool status = false;

	// some drivers/GPUs fail to render to GL_CLAMP_TO_BORDER (and GL_LINEAR may cause a drop in performance for them, too)
	{
		fb.Detach(GL_DEPTH_ATTACHMENT_EXT);
		glDeleteTextures(1, &shadowTexture);

		glGenTextures(1, &shadowTexture);
		glBindTexture(GL_TEXTURE_2D, shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		const GLint texFormat = globalRendering->support24bitDepthBuffers ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16;
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, texFormat, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		fb.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);
		status = fb.CheckStatus("SHADOW-GL_CLAMP_TO_EDGE");
		if (status)
			return true;
	}


	// ATI sometimes fails without an attached color texture, so check a few formats (not all supported texture formats are renderable)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		// 1st: try the smallest unsupported format (4bit per pixel)
		glGenTextures(1, &dummyColorTexture);
		glBindTexture(GL_TEXTURE_2D, dummyColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA4, shadowMapSize, shadowMapSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);
		fb.AttachTexture(dummyColorTexture);
		status = fb.CheckStatus("SHADOW-GL_ALPHA4");
		if (status)
			return true;

		// failed revert changes of 1st attempt
		fb.Detach(GL_COLOR_ATTACHMENT0_EXT);
		glDeleteTextures(1, &dummyColorTexture);

		// 2nd: try smallest standard format that must be renderable for OGL3
		fb.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RED, shadowMapSize, shadowMapSize);
		status = fb.CheckStatus("SHADOW-GL_RED");
		if (status)
			return true;
	}

	return status;
}


void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
		glEnable(GL_CULL_FACE);

		glCullFace(GL_BACK);
			eventHandler.DrawWorldShadow();

			if ((shadowGenBits & SHADOWGEN_BIT_TREE) != 0) {
				treeDrawer->DrawShadowPass();
				treeDrawer->DrawShadowGrass();
			}

			if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0)
				projectileDrawer->DrawShadowPass();

			if ((shadowGenBits & SHADOWGEN_BIT_MODEL) != 0) {
				unitDrawer->DrawShadowPass();
				modelDrawer->Draw();
				featureDrawer->DrawShadowPass();
			}

		glCullFace(GL_FRONT);
			// cull front-faces during the terrain shadow pass: sun direction
			// can be set so oblique that geometry back-faces are visible (eg.
			// from hills near map edges) from its POV
			// (could just disable culling of terrain faces, but we also want
			// to prevent overdraw in such low-angle passes)
			if ((shadowGenBits & SHADOWGEN_BIT_MAP) != 0)
				readmap->GetGroundDrawer()->DrawShadowPass();
	glPopAttrib();

	inShadowPass = false;
}

void CShadowHandler::SetShadowMapSizeFactors()
{
	#if (SHADOWMATRIX_NONLINEAR == 1)
	// note: depends on CalcMinMaxView(), which is no longer called
	const float shadowMapX =              math::sqrt( math::fabs(shadowProjMinMax.y) ); // math::sqrt( |x2| )
	const float shadowMapY =              math::sqrt( math::fabs(shadowProjMinMax.w) ); // math::sqrt( |y2| )
	const float shadowMapW = shadowMapX + math::sqrt( math::fabs(shadowProjMinMax.x) ); // math::sqrt( |x2| ) + math::sqrt( |x1| )
	const float shadowMapH = shadowMapY + math::sqrt( math::fabs(shadowProjMinMax.z) ); // math::sqrt( |y2| ) + math::sqrt( |y1| )

	shadowTexProjCenter.x = 1.0f - (shadowMapX / shadowMapW);
	shadowTexProjCenter.y = 1.0f - (shadowMapY / shadowMapH);

	if (shadowMapSize >= 2048) {
		shadowTexProjCenter.z =  0.01f;
		shadowTexProjCenter.w = -0.1f;
	} else {
		shadowTexProjCenter.z =  0.0025f;
		shadowTexProjCenter.w = -0.05f;
	}
	#else
	shadowTexProjCenter.x = 0.5f;
	shadowTexProjCenter.y = 0.5f;
	shadowTexProjCenter.z = FLT_MAX;
	shadowTexProjCenter.w = 1.0f;
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
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
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

	SetShadowMapSizeFactors();

	// NOTE:
	//     the xy-scaling factors from CalcMinMaxView do not change linearly
	//     or smoothly with camera movements, creating visible artefacts (eg.
	//     large jumps in shadow resolution)
	//
	//     therefore, EITHER use "fixed" scaling values such that the entire
	//     map barely fits into the sun's frustum (by pretending it is embedded
	//     in a sphere and taking its diameter), OR variable scaling such that
	//     everything that can be seen by the camera maximally fills the sun's
	//     frustum (choice of projection-style is left to the user and can be
	//     changed at run-time)
	//
	//     the first option means larger maps will have more blurred/aliased
	//     shadows if the depth buffer is kept at the same size, but no (map)
	//     geometry is ever omitted
	//
	//     the second option means shadows have higher average resolution, but
	//     become less sharp as the viewing volume increases (through eg.camera
	//     rotations) and geometry can be omitted in some cases
	//
	// NOTE:
	//     when DynamicSun is enabled, the orbit is always circular in the xz
	//     plane, instead of elliptical when the map has an aspect-ratio != 1
	//
	const float xyScale = GetShadowProjectionRadius(camera, centerPos, -sunDirZ);
	const float xScale = xyScale;
	const float yScale = xyScale;
	const float zScale = globalRendering->viewRange;

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
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, shadowTexProjCenter.x, shadowTexProjCenter.y, 0.0f, 0.0f);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17, shadowTexProjCenter.z, shadowTexProjCenter.z, 0.0f, 0.0f);
	glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 18, shadowTexProjCenter.w, shadowTexProjCenter.w, 0.0f, 0.0f);

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			shadowGenProgs[i]->Enable();
			shadowGenProgs[i]->SetUniform4fv(0, &shadowTexProjCenter.x);
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

	// we do this later to save render context switches (this is one of the slowest opengl operations!)
	// fb.Unbind();
	// glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
}



float CShadowHandler::GetShadowProjectionRadius(CCamera* cam, float3& proPos, const float3& proDir) const {
	float radius = 1.0f;

	switch (shadowProMode) {
		case SHADOWPROMODE_CAM_CENTER: {
			radius = GetOrthoProjectedFrustumRadius(cam, proPos);
		} break;
		case SHADOWPROMODE_MAP_CENTER: {
			radius = GetOrthoProjectedMapRadius(proDir, proPos);
		} break;
		case SHADOWPROMODE_MIX_CAMMAP: {
			static float3 opfPos;
			static float3 opmPos;

			const float opfRad = GetOrthoProjectedFrustumRadius(cam, opfPos);
			const float opmRad = GetOrthoProjectedMapRadius(proDir, opmPos);

			if (opfRad <= opmRad) { radius = opfRad; proPos = opfPos; }
			if (opmRad <= opfRad) { radius = opmRad; proPos = opmPos; }
		} break;
	}

	return radius;
}

float CShadowHandler::GetOrthoProjectedMapRadius(const float3& sunDir, float3& projectionMidPos) const {
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
	static const float maxMapDiameter = math::sqrtf(Square(gs->mapx * SQUARE_SIZE) + Square(gs->mapy * SQUARE_SIZE));
	static       float curMapDiameter = 0.0f;

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

		curMapDiameter = maxMapDiameter * v2.dot(v1);

		projectionMidPos.x = (gs->mapx * SQUARE_SIZE) * 0.5f;
		projectionMidPos.z = (gs->mapy * SQUARE_SIZE) * 0.5f;
		projectionMidPos.y = ground->GetHeightReal(projectionMidPos.x, projectionMidPos.z, false);
	}

	return curMapDiameter;
}

float CShadowHandler::GetOrthoProjectedFrustumRadius(CCamera* cam, float3& projectionMidPos) const {
	cam->GetFrustumSides(0.0f, 0.0f, 1.0f, true);
	cam->ClipFrustumLines(true, -10000.0f, 400096.0f);

	const std::vector<CCamera::FrustumLine> sides = cam->GetNegFrustumSides();

	if (sides.empty())
		return 0.0f;

	// two points per side; last point is used for the geometric average
	// there are never more than 5 side-lines (10 points), so reserve 16
	static std::vector<float3> frustumPoints(16, ZeroVector);

	float3 frustumCenter = ZeroVector;
	float  frustumRadius = 0.0f;

	for (unsigned int i = 0, j = 0; i < sides.size(); i++) {
		const CCamera::FrustumLine* line = &sides[i];

		if (line->minz < line->maxz) {
			const float x0 = line->base + (line->dir * line->minz), z0 = line->minz;
			const float x1 = line->base + (line->dir * line->maxz), z1 = line->maxz;

			// TODO: smarter clamping
			const float
				cx0 = Clamp(x0, 0.0f, (float3::maxxpos + 1.0f)),
				cz0 = Clamp(z0, 0.0f, (float3::maxzpos + 1.0f)),
				cx1 = Clamp(x1, 0.0f, (float3::maxxpos + 1.0f)),
				cz1 = Clamp(z1, 0.0f, (float3::maxzpos + 1.0f));

			const float3 p0 = float3(cx0, ground->GetHeightReal(cx0, cz0, false), cz0);
			const float3 p1 = float3(cx1, ground->GetHeightReal(cx1, cz1, false), cz1);

			frustumPoints[j + 0] = p0;
			frustumPoints[j + 1] = p1;
			frustumCenter += p0;
			frustumCenter += p1;

			j += 2;
		}
	}

	projectionMidPos.x = frustumCenter.x / (sides.size() * 2);
	projectionMidPos.z = frustumCenter.z / (sides.size() * 2);
	projectionMidPos.y = ground->GetHeightReal(projectionMidPos.x, projectionMidPos.z, false);

	// calculate the radius of the minimally-bounding sphere around the projected frustum
	for (unsigned int n = 0; n < (sides.size() * 2); n++) {
		const float3& pos = frustumPoints[n];
		const float   rad = (pos - projectionMidPos).SqLength();

		frustumRadius = std::max(frustumRadius, rad);
	}

	static const float maxMapDiameter = math::sqrtf(Square(gs->mapx * SQUARE_SIZE) + Square(gs->mapy * SQUARE_SIZE));
	       const float frustumDiameter = math::sqrtf(frustumRadius) * 2.0f;

	return std::min(maxMapDiameter, frustumDiameter);
}



#if 0
void CShadowHandler::CalcMinMaxView()
{
	// derive the size of the shadow-map from the
	// intersection points of the camera frustum
	// with the xz-plane
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

	const std::vector<CCamera::FrustumLine> negSides = cam2->GetNegFrustumSides();
	const std::vector<CCamera::FrustumLine> posSides = cam2->GetPosFrustumSides();
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

	// xScale = (shadowProjMinMax.y - shadowProjMinMax.x) * 1.5f;
	// yScale = (shadowProjMinMax.w - shadowProjMinMax.z) * 1.5f;
}
#endif

