/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cfloat>

#include "ShadowHandler.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GameVersion.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

#include "lib/fmt/format.h"

CONFIG(int, Shadows).defaultValue(2).headlessValue(-1).minimumValue(-1).safemodeValue(-1).description("Sets whether shadows are rendered.\n-1:=forceoff, 0:=off, 1:=full, 2:=fast (skip terrain)"); //FIXME document bitmask
CONFIG(int, ShadowMapSize).defaultValue(CShadowHandler::DEF_SHADOWMAP_SIZE).minimumValue(32).description("Sets the resolution of shadows. Higher numbers increase quality at the cost of performance.");
CONFIG(int, ShadowProjectionMode).defaultValue(CShadowHandler::SHADOWPROMODE_CAM_CENTER);

CShadowHandler shadowHandler;

void CShadowHandler::Reload(const char* argv)
{
	int nextShadowConfig = (shadowConfig + 1) & 0xF;
	int nextShadowMapSize = shadowMapSize;
	int nextShadowProMode = shadowProMode;

	if (argv != nullptr)
		(void) sscanf(argv, "%i %i %i", &nextShadowConfig, &nextShadowMapSize, &nextShadowProMode);

	// do nothing without a parameter change
	if (nextShadowConfig == shadowConfig && nextShadowMapSize == shadowMapSize && nextShadowProMode == shadowProMode)
		return;

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
	// disabled; other option usually produces worse resolution
	// shadowProMode = configHandler->GetInt("ShadowProjectionMode");
	shadowProMode = SHADOWPROMODE_CAM_CENTER;
	shadowGenBits = SHADOWGEN_BIT_NONE;

	shadowsLoaded = false;
	inShadowPass = false;

	shadowDepthTexture = 0;
	shadowColorTexture = 0;

	if (!tmpFirstInit && !shadowsSupported)
		return;

	// possible values for the "Shadows" config-parameter:
	// < 0: disable and don't try to initialize
	//   0: disable, but still check if the hardware is able to run them
	// > 0: enabled (by default for all shadow-casting geometry if equal to 1)
	if (shadowConfig < 0) {
		LOG("[%s] shadow rendering is disabled (config-value %d)", __func__, shadowConfig);
		return;
	}

	if (shadowConfig > 0)
		shadowGenBits = SHADOWGEN_BIT_MODEL | SHADOWGEN_BIT_MAP | SHADOWGEN_BIT_PROJ | SHADOWGEN_BIT_TREE;

	if (shadowConfig > 1)
		shadowGenBits &= (~shadowConfig);

	// no warnings when running headless
	if (SpringVersion::IsHeadless())
		return;

	if (!globalRendering->haveGLSL) {
		LOG_L(L_WARNING, "[%s] GPU does not support either GLSL shaders for shadow rendering", __func__);
		return;
	}

	if (!globalRendering->haveGLSL) {
		if (!GLEW_ARB_shadow || !GLEW_ARB_depth_texture || !GLEW_ARB_texture_env_combine) {
			LOG_L(L_WARNING, "[%s] required OpenGL ARB-extensions missing for shadow rendering", __func__);
			// NOTE: these should only be relevant for FFP shadows
			// return;
		}
		if (!GLEW_ARB_shadow_ambient) {
			// can't use arbitrary texvals in case the depth comparison op fails (only 0)
			LOG_L(L_WARNING, "[%s] \"ARB_shadow_ambient\" extension missing (will probably make shadows darker than they should be)", __func__);
		}
	}


	if (!InitFBOAndTextures()) {
		// free any resources allocated by InitFBOAndTextures()
		FreeFBOAndTextures();

		LOG_L(L_ERROR, "[%s] failed to initialize depth-texture FBO", __func__);
		return;
	}

	if (tmpFirstInit)
		shadowsSupported = true;

	if (shadowConfig == 0) {
		// free any resources allocated by InitFBOAndTextures()
		FreeFBOAndTextures();

		// shadowsLoaded is still false
		return;
	}

	LoadProjectionMatrix(CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW));
	LoadShadowGenShaders();
}

void CShadowHandler::Kill()
{
	FreeFBOAndTextures();
	shaderHandler->ReleaseProgramObjects("[ShadowHandler]");
	shadowGenProgs.fill(nullptr);
}


void CShadowHandler::Update()
{
	CCamera* playCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	CCamera* shadCam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);

	SetShadowMatrix(playCam, shadCam);
	SetShadowCamera(shadCam);
}

void CShadowHandler::SaveShadowMapTextures() const
{
	glSaveTexture(shadowDepthTexture, fmt::format("smDepth_{}.png", globalRendering->drawFrame).c_str());
	glSaveTexture(shadowColorTexture, fmt::format("smColor_{}.png", globalRendering->drawFrame).c_str());
}

void CShadowHandler::FreeFBOAndTextures() {
	if (smOpaqFBO.IsValid()) {
		smOpaqFBO.Bind();
		smOpaqFBO.DetachAll();
		smOpaqFBO.Unbind();
	}

	smOpaqFBO.Kill();

	glDeleteTextures(1, &shadowDepthTexture); shadowDepthTexture = 0;
	glDeleteTextures(1, &shadowColorTexture); shadowColorTexture = 0;
}



void CShadowHandler::LoadProjectionMatrix(const CCamera* shadowCam)
{
	const CMatrix44f& ccm = shadowCam->GetClipControlMatrix();
	      CMatrix44f& spm = projMatrix[SHADOWMAT_TYPE_DRAWING];

	// same as glOrtho(-1, 1,  -1, 1,  -1, 1); just inverts Z
	// spm.LoadIdentity();
	// spm.SetZ(-FwdVector);

	// same as glOrtho(0, 1,  0, 1,  0, -1); maps [0,1] to [-1,1]
	spm.LoadIdentity();
	spm.Translate(-OnesVector);
	spm.Scale(OnesVector * 2.0f);

	// if using ZTO clip-space, cancel out the above remap for Z
	spm = ccm * spm;
}

void CShadowHandler::LoadShadowGenShaders()
{
	if (!globalRendering->haveGLSL)
		return;

	#define sh shaderHandler
	static const std::string shadowGenProgHandles[SHADOWGEN_PROGRAM_COUNT] = {
		"ShadowGenShaderProgModel",
		"ShadowGenShaderProgModelGL4",
		"ShadowGenshaderProgMap",
		"ShadowGenshaderProgProjectileOpaque",
	};
	static const std::string shadowGenProgDefines[SHADOWGEN_PROGRAM_COUNT] = {
		"#define SHADOWGEN_PROGRAM_MODEL\n",
		"#define SHADOWGEN_PROGRAM_MODEL_GL4\n",
		"#define SHADOWGEN_PROGRAM_MAP\n",
		"#define SHADOWGEN_PROGRAM_PROJ_OPAQ\n",
	};

	// #version has to be added here because it is conditional
	static const std::string versionDefs[2] = {
		"#version 130\n",
		"#version " + IntToString(globalRendering->supportFragDepthLayout? 420: 130) + "\n",
	};

	static const std::string extraDefs =
		("#define SUPPORT_CLIP_CONTROL " + IntToString(globalRendering->supportClipSpaceControl) + "\n") +
		("#define SUPPORT_DEPTH_LAYOUT " + IntToString(globalRendering->supportFragDepthLayout) + "\n");

	for (int i = 0; i < SHADOWGEN_PROGRAM_COUNT; i++) {
		if (i == SHADOWGEN_PROGRAM_MODEL_GL4)
			continue; //special path

		Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "GLSL");

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertProg.glsl", versionDefs[0] + shadowGenProgDefines[i] + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProg.glsl", versionDefs[1] + shadowGenProgDefines[i] + extraDefs, GL_FRAGMENT_SHADER));

		po->Link();
		po->Enable();
		po->SetUniform("alphaMaskTex", 0);
		po->SetUniform("alphaParams", mapInfo->map.voidAlphaMin, 0.0f);
		po->Disable();
		po->Validate();

		shadowGenProgs[i] = po;
	}
	{
		Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[SHADOWGEN_PROGRAM_MODEL_GL4] + "GLSL");

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertProgGL4.glsl", shadowGenProgDefines[SHADOWGEN_PROGRAM_MODEL_GL4] + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProgGL4.glsl", shadowGenProgDefines[SHADOWGEN_PROGRAM_MODEL_GL4] + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();
		po->Enable();
		po->SetUniform("cameraMode", 1);
		po->SetUniform("alphaCtrl", 0.5f, 1.0f, 0.0f, 0.0f); // test > 0.5
		po->Disable();
		po->Validate();

		shadowGenProgs[SHADOWGEN_PROGRAM_MODEL_GL4] = po;
	}

	shadowsLoaded = true;
	#undef sh
}



bool CShadowHandler::InitFBOAndTextures()
{
	// smOpaqFBO is no-op constructed, has to be initialized manually
	smOpaqFBO.Init(false);

	if (!smOpaqFBO.IsValid()) {
		LOG_L(L_ERROR, "[%s] framebuffer not valid", __func__);
		return false;
	}

	// TODO: add bit depth?
	static constexpr struct {
		GLint clampMode;
		GLint filterMode;
		const char* name;
	} presets[] = {
		{GL_CLAMP_TO_BORDER, GL_LINEAR , "SHADOW-BEST"  },
		{GL_CLAMP_TO_EDGE  , GL_NEAREST, "SHADOW-COMPAT"},
	};

	bool status = false;
	for (const auto& preset : presets)
	{
		if (FBO::GetCurrentBoundFBO() == smOpaqFBO.GetId())
			smOpaqFBO.DetachAll();

		//depth
		glDeleteTextures(1, &shadowDepthTexture);
		glGenTextures(1, &shadowDepthTexture);
		glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
		constexpr float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, preset.clampMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, preset.clampMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, preset.filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, preset.filterMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //no mips

		const int depthBits = std::min(globalRendering->supportDepthBufferBitDepth, 24);
		const GLint depthFormat = CGlobalRendering::DepthBitsToFormat(depthBits);

		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, depthFormat, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		/// color
		glDeleteTextures(1, &shadowColorTexture);
		glGenTextures(1, &shadowColorTexture);
		glBindTexture(GL_TEXTURE_2D, shadowColorTexture);
		// TODO: Figure out if mips make sense here.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); //no mips

		// Store full color here, so special effects can cast nice color shadow.
		if (GLEW_EXT_packed_float)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_FLOAT, nullptr);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glBindTexture(GL_TEXTURE_2D, 0);

		// Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
		smOpaqFBO.Bind();
		smOpaqFBO.AttachTexture(shadowDepthTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT);
		smOpaqFBO.AttachTexture(shadowColorTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		// test the FBO
		status = smOpaqFBO.CheckStatus(preset.name);

		if (status) //exit on the first occasion
			break;
	}

	smOpaqFBO.Unbind();
	return status;
}

void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	eventHandler.DrawWorldShadow();

	EnableColorOutput(true);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	EnableColorOutput(false);

	if ((shadowGenBits & SHADOWGEN_BIT_TREE) != 0) {
		grassDrawer->DrawShadow();
	}

	if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0)
		projectileDrawer->DrawShadowPassOpaque();

	if ((shadowGenBits & SHADOWGEN_BIT_MODEL) != 0) {
		unitDrawer->DrawShadowPass();
		featureDrawer->DrawShadowPass();
	}

	// cull front-faces during the terrain shadow pass: sun direction
	// can be set so oblique that geometry back-faces are visible (eg.
	// from hills near map edges) from its POV
	//
	// not the best idea, causes acne when projecting the shadow-map
	// (rasterizing back-faces writes different depth values) and is
	// no longer required since border geometry will fully hide them
	// (could just disable culling of terrain faces entirely, but we
	// also want to prevent overdraw in low-angle passes)
	// glCullFace(GL_FRONT);

	// Restore GL_BACK culling, because Lua shadow materials might
	// have changed culling at their own discretion
	glCullFace(GL_BACK);
	if ((shadowGenBits & SHADOWGEN_BIT_MAP) != 0)
		readMap->GetGroundDrawer()->DrawShadowPass();

	//transparent pass, comes last
	if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0)
		projectileDrawer->DrawShadowPassTransparent();

	glPopAttrib();

	inShadowPass = false;
}

static CMatrix44f ComposeLightMatrix(const ISkyLight* light)
{
	CMatrix44f lightMatrix;

	// sun direction is in world-space, invert it
	lightMatrix.SetZ(-float3(light->GetLightDir()));
	lightMatrix.SetX(((lightMatrix.GetZ()).cross(   UpVector       )).ANormalize());
	lightMatrix.SetY(((lightMatrix.GetX()).cross(lightMatrix.GetZ())).ANormalize());

	return lightMatrix;
}

static CMatrix44f ComposeScaleMatrix(const float4 scales)
{
	// note: T is z-bias, scales.z is z-near
	return (CMatrix44f(FwdVector * 0.5f, RgtVector / scales.x, UpVector / scales.y, FwdVector / scales.w));
}

void CShadowHandler::SetShadowMatrix(CCamera* playerCam, CCamera* shadowCam)
{
	const CMatrix44f lightMatrix = ComposeLightMatrix(sky->GetLight());
	const CMatrix44f scaleMatrix = ComposeScaleMatrix(shadowProjScales = GetShadowProjectionScales(playerCam, lightMatrix));

	// KISS; define only the world-to-light transform (P[CULLING] is unused anyway)
	//
	// we have two options: either place the camera such that it *looks at* projMidPos
	// (along lightMatrix.GetZ()) or such that it is *at or behind* projMidPos looking
	// in the inverse direction (the latter is chosen here since this matrix determines
	// the shadow-camera's position and thereby terrain tessellation shadow-LOD)
	// NOTE:
	//   should be -X-Z, but particle-quads are sensitive to right being flipped
	//   we can omit inverting X (does not impact VC) or disable PD face-culling
	//   or just let objects end up behind znear since InView only tests against
	//   zfar
	viewMatrix[SHADOWMAT_TYPE_CULLING].LoadIdentity();
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetX(lightMatrix.GetX());
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetY(lightMatrix.GetY());
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetZ(lightMatrix.GetZ());
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetPos(projMidPos[2]);

	// shaders need this form, projection into SM-space is done by shadow2DProj()
	// note: ShadowGenVertProg is a special case because it does not use uniforms
	viewMatrix[SHADOWMAT_TYPE_DRAWING].LoadIdentity();
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetX(lightMatrix.GetX());
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetY(lightMatrix.GetY());
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetZ(lightMatrix.GetZ());
	viewMatrix[SHADOWMAT_TYPE_DRAWING].Scale(float3(scaleMatrix[0], scaleMatrix[5], scaleMatrix[10])); // extract (X.x, Y.y, Z.z)
	viewMatrix[SHADOWMAT_TYPE_DRAWING].Transpose();
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetPos(viewMatrix[SHADOWMAT_TYPE_DRAWING] * -projMidPos[2]);
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetPos(viewMatrix[SHADOWMAT_TYPE_DRAWING].GetPos() + scaleMatrix.GetPos()); // add z-bias
}

void CShadowHandler::SetShadowCamera(CCamera* shadowCam)
{
	// first set matrices needed by shaders (including ShadowGenVertProg)
	shadowCam->SetProjMatrix(projMatrix[SHADOWMAT_TYPE_DRAWING]);
	shadowCam->SetViewMatrix(viewMatrix[SHADOWMAT_TYPE_DRAWING]);

	shadowCam->SetAspectRatio(shadowProjScales.x / shadowProjScales.y);
	// convert xy-diameter to radius
	shadowCam->SetFrustumScales(shadowProjScales * float4(0.5f, 0.5f, 1.0f, 1.0f));
	shadowCam->UpdateFrustum();
	shadowCam->UpdateLoadViewPort(0, 0, shadowMapSize, shadowMapSize);
	// load matrices into gl_{ModelView,Projection}Matrix
	shadowCam->Update({false, false, false, false, false});

	// next set matrices needed for SP visibility culling (these
	// are *NEVER* loaded into gl_{ModelView,Projection}Matrix!)
	shadowCam->SetProjMatrix(projMatrix[SHADOWMAT_TYPE_CULLING]);
	shadowCam->SetViewMatrix(viewMatrix[SHADOWMAT_TYPE_CULLING]);
	shadowCam->UpdateFrustum();
}


void CShadowHandler::SetupShadowTexSampler(unsigned int texUnit, bool enable) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

	// support FFP context
	if (enable)
		glEnable(GL_TEXTURE_2D);

	SetupShadowTexSamplerRaw();
}

void CShadowHandler::SetupShadowTexSamplerRaw() const
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	// glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	// glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_ALPHA);
}

void CShadowHandler::ResetShadowTexSampler(unsigned int texUnit, bool disable) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (disable)
		glDisable(GL_TEXTURE_2D);

	ResetShadowTexSamplerRaw();
}

void CShadowHandler::ResetShadowTexSamplerRaw() const
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
}


void CShadowHandler::CreateShadows()
{
	// NOTE:
	//   we unbind later in WorldDrawer::GenerateIBLTextures() to save render
	//   context switches (which are one of the slowest OpenGL operations!)
	//   together with VP restoration
	smOpaqFBO.Bind();

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);


	//flickers without it. Why?
	SetShadowCamera(CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW));

	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_SHADOW);

	if ((sky->GetLight())->GetLightIntensity() > 0.0f)
		DrawShadowPasses();

	CCameraHandler::SetActiveCamera(prvCam->GetCamType());
	prvCam->Update();


	glShadeModel(GL_SMOOTH);

	//revert to default, EnableColorOutput(true) is not enough
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void CShadowHandler::EnableColorOutput(bool enable) const
{
	assert(FBO::GetCurrentBoundFBO() == smOpaqFBO.GetId());

	const GLboolean b = static_cast<GLboolean>(enable);
	glColorMask(b, b, b, GL_FALSE);
}



float4 CShadowHandler::GetShadowProjectionScales(CCamera* cam, const CMatrix44f& projMat) {
	float4 projScales;
	float2 projRadius;

	// NOTE:
	//   the xy-scaling factors from CalcMinMaxView do not change linearly
	//   or smoothly with camera movements, creating visible artefacts (eg.
	//   large jumps in shadow resolution)
	//
	//   therefore, EITHER use "fixed" scaling values such that the entire
	//   map barely fits into the sun's frustum (by pretending it is embedded
	//   in a sphere and taking its diameter), OR variable scaling such that
	//   everything that can be seen by the camera maximally fills the sun's
	//   frustum (choice of projection-style is left to the user and can be
	//   changed at run-time)
	//
	//   the first option means larger maps will have more blurred/aliased
	//   shadows if the depth buffer is kept at the same size, but no (map)
	//   geometry is ever omitted
	//
	//   the second option means shadows have higher average resolution, but
	//   become less sharp as the viewing volume increases (through eg.camera
	//   rotations) and geometry can be omitted in some cases
	//
	switch (shadowProMode) {
		case SHADOWPROMODE_CAM_CENTER: {
			projScales.x = GetOrthoProjectedFrustumRadius(cam, projMat, projMidPos[2]);
		} break;
		case SHADOWPROMODE_MAP_CENTER: {
			projScales.x = GetOrthoProjectedMapRadius(-projMat.GetZ(), projMidPos[2]);
		} break;
		case SHADOWPROMODE_MIX_CAMMAP: {
			projRadius.x = GetOrthoProjectedFrustumRadius(cam, projMat, projMidPos[0]);
			projRadius.y = GetOrthoProjectedMapRadius(-projMat.GetZ(), projMidPos[1]);
			projScales.x = std::min(projRadius.x, projRadius.y);

			// pick the center position (0 or 1) for which radius is smallest
			projMidPos[2] = projMidPos[projRadius.x >= projRadius.y];
		} break;
	}

	projScales.y = projScales.x;
	#if 0
	projScales.z = cam->GetNearPlaneDist();
	projScales.w = cam->GetFarPlaneDist();
	#else
	// prefer slightly tighter fixed bounds
	projScales.z = 0.0f;
	projScales.w = readMap->GetBoundingRadius() * 2.0f;
	#endif
	return projScales;
}

float CShadowHandler::GetOrthoProjectedMapRadius(const float3& sunDir, float3& projPos) {
	// to fit the map inside the frustum, we need to know
	// the distance from one corner to its opposing corner
	//
	// this distance is maximal when the sun direction is
	// orthogonal to the diagonal, but in other cases we
	// can gain some precision by projecting the diagonal
	// onto a vector orthogonal to the sun direction and
	// using the length of that projected vector instead
	//
	const float maxMapDiameter = readMap->GetBoundingRadius() * 2.0f;
	static float curMapDiameter = 0.0f;

	// recalculate pos only if the sun-direction has changed
	if (sunProjDir != sunDir) {
		sunProjDir = sunDir;

		float3 sunDirXZ = (sunDir * XZVector).ANormalize();
		float3 mapVerts[2];

		if (sunDirXZ.x >= 0.0f) {
			if (sunDirXZ.z >= 0.0f) {
				// use diagonal vector from top-right to bottom-left
				mapVerts[0] = float3(mapDims.mapx * SQUARE_SIZE, 0.0f,                       0.0f);
				mapVerts[1] = float3(                      0.0f, 0.0f, mapDims.mapy * SQUARE_SIZE);
			} else {
				// use diagonal vector from top-left to bottom-right
				mapVerts[0] = float3(                      0.0f, 0.0f,                       0.0f);
				mapVerts[1] = float3(mapDims.mapx * SQUARE_SIZE, 0.0f, mapDims.mapy * SQUARE_SIZE);
			}
		} else {
			if (sunDirXZ.z >= 0.0f) {
				// use diagonal vector from bottom-right to top-left
				mapVerts[0] = float3(mapDims.mapx * SQUARE_SIZE, 0.0f, mapDims.mapy * SQUARE_SIZE);
				mapVerts[1] = float3(                      0.0f, 0.0f,                       0.0f);
			} else {
				// use diagonal vector from bottom-left to top-right
				mapVerts[0] = float3(                      0.0f, 0.0f, mapDims.mapy * SQUARE_SIZE);
				mapVerts[1] = float3(mapDims.mapx * SQUARE_SIZE, 0.0f,                       0.0f);
			}
		}

		const float3 v1 = (mapVerts[1] - mapVerts[0]).ANormalize();
		const float3 v2 = float3(-sunDirXZ.z, 0.0f, sunDirXZ.x);

		curMapDiameter = maxMapDiameter * v2.dot(v1);

		projPos.x = (mapDims.mapx * SQUARE_SIZE) * 0.5f;
		projPos.z = (mapDims.mapy * SQUARE_SIZE) * 0.5f;
		projPos.y = CGround::GetHeightReal(projPos.x, projPos.z, false);
	}

	return curMapDiameter;
}

float CShadowHandler::GetOrthoProjectedFrustumRadius(CCamera* cam, const CMatrix44f& projMat, float3& projPos) {
	float3 frustumPoints[8];

	#if 0
	{
		float sqRadius = 0.0f;
		projPos = CalcShadowProjectionPos(cam, &frustumPoints[0]);

		// calculate radius of the minimally-bounding sphere around projected frustum
		for (unsigned int n = 0; n < 8; n++) {
			sqRadius = std::max(sqRadius, (frustumPoints[n] - projPos).SqLength());
		}

		const float maxMapDiameter = readMap->GetBoundingRadius() * 2.0f;
		const float frustumDiameter = std::sqrt(sqRadius) * 2.0f;

		return (std::min(maxMapDiameter, frustumDiameter));
	}
	#else
	{
		CMatrix44f frustumProjMat;
		frustumProjMat.SetX(projMat.GetX());
		frustumProjMat.SetY(projMat.GetY());
		frustumProjMat.SetZ(projMat.GetZ());
		frustumProjMat.SetPos(projPos = CalcShadowProjectionPos(cam, &frustumPoints[0]));

		// find projected width along {x,z}-axes (.x := min, .y := max)
		float2 xbounds = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
		float2 zbounds = {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};

		for (unsigned int n = 0; n < 8; n++) {
			frustumPoints[n] = frustumProjMat * frustumPoints[n];

			xbounds.x = std::min(xbounds.x, frustumPoints[n].x);
			xbounds.y = std::max(xbounds.y, frustumPoints[n].x);
			zbounds.x = std::min(zbounds.x, frustumPoints[n].z);
			zbounds.y = std::max(zbounds.y, frustumPoints[n].z);
		}

		// factor in z-bounds to prevent clipping
		return (std::min(readMap->GetBoundingRadius() * 2.0f, std::max(xbounds.y - xbounds.x, zbounds.y - zbounds.x)));
	}
	#endif
}

float3 CShadowHandler::CalcShadowProjectionPos(CCamera* cam, float3* frustumPoints)
{
	const auto ClipByPlanes = [](const float3& p0, float3& p, const std::initializer_list<float4>& clipPlanes) {
		float tMin = 1.0f;
		float3 rayMin;

		for (const auto& clipPlane : clipPlanes) {
			const float3 ray = (p - p0);

			float denom = clipPlane.dot(ray);
			if (std::fabs(denom) < 1e-4)
				continue;

			float t = (clipPlane.w - clipPlane.dot(p0)) / denom;
			if (t < 0.0f || t > 1.0f)
				continue;

			if (t < tMin) {
				tMin = t;
				rayMin = ray;
			}
		}

		if (tMin < 1.0f)
			p = p0 + rayMin * tMin;
	};

	static constexpr float T1 = 100.0f;
	static constexpr float T2 = 200.0f;

	float3 projPos;
	for (int i = 0; i < 8; ++i)
		frustumPoints[i] = cam->GetFrustumVert(i);

	const std::initializer_list<float4> nearClipPlanes = {
		float4{ UpVector, readMap->GetCurrMaxHeight() + T1 },
		//float4{ RgtVector, -T2 },
		//float4{ RgtVector, mapDims.mapx * SQUARE_SIZE + T2 },
		//float4{ FwdVector, -T2 },
		//float4{ FwdVector, mapDims.mapy * SQUARE_SIZE + T2 }
	};
	const std::initializer_list<float4> farClipPlanes = {
		float4{ UpVector, readMap->GetCurrMinHeight() - T1 },
		//float4{ RgtVector, -T2 },
		//float4{ RgtVector, mapDims.mapx * SQUARE_SIZE + T2 },
		//float4{ FwdVector, -T2 },
		//float4{ FwdVector, mapDims.mapy * SQUARE_SIZE + T2 }
	};

	for (int i = 0; i < 4; ++i) {
		//near quadrilateral
		ClipByPlanes(frustumPoints[4 + i], frustumPoints[i], nearClipPlanes);
		//far quadrilateral
		ClipByPlanes(frustumPoints[i], frustumPoints[4 + i],  farClipPlanes);

		//hard clamp xz
		frustumPoints[    i].x = std::clamp(frustumPoints[    i].x, -T2, mapDims.mapx * SQUARE_SIZE + T2);
		frustumPoints[    i].z = std::clamp(frustumPoints[    i].z, -T2, mapDims.mapy * SQUARE_SIZE + T2);
		frustumPoints[4 + i].x = std::clamp(frustumPoints[4 + i].x, -T2, mapDims.mapx * SQUARE_SIZE + T2);
		frustumPoints[4 + i].z = std::clamp(frustumPoints[4 + i].z, -T2, mapDims.mapy * SQUARE_SIZE + T2);

		projPos += frustumPoints[i] + frustumPoints[4 + i];
	}

	projPos *= 0.125f;

	return projPos;
}