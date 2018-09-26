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
#include "Rendering/FeatureDrawer.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/GrassDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

#define SHADOWMATRIX_NONLINEAR 0
#define SHADOWGEN_PER_FRAGMENT 1 // only for maps

CONFIG(int, Shadows).defaultValue(2).headlessValue(-1).minimumValue(-1).safemodeValue(-1).description("Sets whether shadows are rendered.\n-1:=forceoff, 0:=off, 1:=full, 2:=fast (skip terrain)"); //FIXME document bitmask
CONFIG(int, ShadowMapSize).defaultValue(CShadowHandler::DEF_SHADOWMAP_SIZE).minimumValue(32).description("Sets the resolution of shadows. Higher numbers increase quality at the cost of performance.");
CONFIG(int, ShadowProjectionMode).defaultValue(CShadowHandler::SHADOWPROMODE_CAM_CENTER);

CShadowHandler shadowHandler;

bool CShadowHandler::shadowsSupported = false;
bool CShadowHandler::firstInit = true;


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

	shadowTexture = 0;
	dummyColorTexture = 0;

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

	if (!globalRendering->haveARB && !globalRendering->haveGLSL) {
		LOG_L(L_WARNING, "[%s] GPU does not support either ARB or GLSL shaders for shadow rendering", __func__);
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


	if (!InitDepthTarget()) {
		// free any resources allocated by InitDepthTarget()
		FreeTextures();

		LOG_L(L_ERROR, "[%s] failed to initialize depth-texture FBO", __func__);
		return;
	}

	if (tmpFirstInit)
		shadowsSupported = true;

	if (shadowConfig == 0) {
		// free any resources allocated by InitDepthTarget()
		FreeTextures();

		// shadowsLoaded is still false
		return;
	}

	LoadProjectionMatrix(CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW));
	LoadShadowGenShaders();
}

void CShadowHandler::Kill()
{
	FreeTextures();
	shaderHandler->ReleaseProgramObjects("[ShadowHandler]");
	shadowGenProgs.fill(nullptr);
}

void CShadowHandler::FreeTextures() {
	if (shadowMapFBO.IsValid()) {
		shadowMapFBO.Bind();
		shadowMapFBO.DetachAll();
		shadowMapFBO.Unbind();
	}

	shadowMapFBO.Kill();

	glDeleteTextures(1, &shadowTexture    ); shadowTexture     = 0;
	glDeleteTextures(1, &dummyColorTexture); dummyColorTexture = 0;
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
	#define sh shaderHandler
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

	// #version has to be added here because it is conditional
	static const std::string versionDefs[2] = {
		"#version 130\n",
		"#version " + IntToString(globalRendering->supportFragDepthLayout? 420: 130) + "\n",
	};
	static const std::string extraDefs =
		("#define SHADOWMATRIX_NONLINEAR " + IntToString(SHADOWMATRIX_NONLINEAR) + "\n") +
		("#define SHADOWGEN_PER_FRAGMENT " + IntToString(SHADOWGEN_PER_FRAGMENT) + "\n") +
		("#define SUPPORT_CLIP_CONTROL " + IntToString(globalRendering->supportClipSpaceControl) + "\n") +
		("#define SUPPORT_DEPTH_LAYOUT " + IntToString(globalRendering->supportFragDepthLayout) + "\n");

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			Shader::IProgramObject* po = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "GLSL", false);

			if (i == SHADOWGEN_PROGRAM_MAP) {
				po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertProg.glsl", versionDefs[0] + shadowGenProgDefines[i] + extraDefs, GL_VERTEX_SHADER));
				po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenFragProg.glsl", versionDefs[1] + shadowGenProgDefines[i] + extraDefs, GL_FRAGMENT_SHADER));
			} else {
				po->AttachShaderObject(sh->CreateShaderObject("GLSL/ShadowGenVertProg.glsl", versionDefs[0] + shadowGenProgDefines[i] + extraDefs, GL_VERTEX_SHADER));
			}

			po->Link();
			po->SetUniformLocation("shadowParams");  // idx 0
			po->SetUniformLocation("cameraDirX");    // idx 1, used by SHADOWGEN_PROGRAM_TREE_NEAR
			po->SetUniformLocation("cameraDirY");    // idx 2, used by SHADOWGEN_PROGRAM_TREE_NEAR
			po->SetUniformLocation("treeOffset");    // idx 3, used by SHADOWGEN_PROGRAM_TREE_NEAR
			po->SetUniformLocation("alphaMaskTex");  // idx 4
			po->SetUniformLocation("alphaParams");   // idx 5, used by SHADOWGEN_PROGRAM_MAP
			po->Enable();
			po->SetUniform1i(4, 0); // alphaMaskTex
			po->SetUniform2f(5, mapInfo->map.voidAlphaMin, 0.0f); // alphaParams
			po->Disable();
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
	constexpr bool useColorTexture = false;

	// shadowMapFBO is no-op constructed, has to be initialized manually
	shadowMapFBO.Init(false);

	if (!shadowMapFBO.IsValid()) {
		LOG_L(L_ERROR, "[%s] framebuffer not valid", __func__);
		return false;
	}

	glGenTextures(1, &shadowTexture);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);
	constexpr float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, one);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // shadowtex linear sampling is for-free on NVidias
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (useColorTexture) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadowMapSize, shadowMapSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		shadowMapFBO.Bind();
		shadowMapFBO.AttachTexture(shadowTexture);

		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		// Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
		shadowMapFBO.Bind();
		shadowMapFBO.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);

		glDrawBuffer(GL_NONE);
		// glReadBuffer() only works with color buffers
	}

	// test the FBO
	bool status = shadowMapFBO.CheckStatus("SHADOW");

	if (!status && !useColorTexture)
		status = WorkaroundUnsupportedFboRenderTargets();

	shadowMapFBO.Unbind();
	return status;
}


bool CShadowHandler::WorkaroundUnsupportedFboRenderTargets()
{
	bool status = false;

	// some drivers/GPUs fail to render to GL_CLAMP_TO_BORDER (and GL_LINEAR may cause a drop in performance for them, too)
	{
		shadowMapFBO.Detach(GL_DEPTH_ATTACHMENT_EXT);
		glDeleteTextures(1, &shadowTexture);

		glGenTextures(1, &shadowTexture);
		glBindTexture(GL_TEXTURE_2D, shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		const GLint texFormat = globalRendering->support24bitDepthBuffer? GL_DEPTH_COMPONENT24: GL_DEPTH_COMPONENT16;
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexImage2D(GL_TEXTURE_2D, 0, texFormat, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		shadowMapFBO.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT_EXT);

		if (shadowMapFBO.CheckStatus("SHADOW-GL_CLAMP_TO_EDGE"))
			return true;
	}


	// ATI sometimes fails without an attached color texture, so check a few formats (not all supported texture formats are renderable)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

		// 1st: try the smallest unsupported format (4bit per pixel)
		glGenTextures(1, &dummyColorTexture);
		glBindTexture(GL_TEXTURE_2D, dummyColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA4, shadowMapSize, shadowMapSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);

		shadowMapFBO.AttachTexture(dummyColorTexture);

		if (shadowMapFBO.CheckStatus("SHADOW-GL_ALPHA4"))
			return true;

		// failed revert changes of 1st attempt
		shadowMapFBO.Detach(GL_COLOR_ATTACHMENT0_EXT);
		glDeleteTextures(1, &dummyColorTexture);

		// 2nd: try smallest standard format that must be renderable for OGL3
		shadowMapFBO.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RED, shadowMapSize, shadowMapSize);

		if (shadowMapFBO.CheckStatus("SHADOW-GL_RED"))
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
				grassDrawer->DrawShadow();
			}

			if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0)
				projectileDrawer->DrawShadowPass();

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
			if ((shadowGenBits & SHADOWGEN_BIT_MAP) != 0)
				readMap->GetGroundDrawer()->DrawShadowPass();

	glPopAttrib();

	inShadowPass = false;
}

void CShadowHandler::SetShadowMapSizeFactors()
{
	#if (SHADOWMATRIX_NONLINEAR == 1)
	// note: depends on CalcMinMaxView(), which is no longer called
	const float shadowMapX =              std::sqrt( std::fabs(shadowProjMinMax.y) ); // math::sqrt( |x2| )
	const float shadowMapY =              std::sqrt( std::fabs(shadowProjMinMax.w) ); // math::sqrt( |y2| )
	const float shadowMapW = shadowMapX + std::sqrt( std::fabs(shadowProjMinMax.x) ); // math::sqrt( |x2| ) + math::sqrt( |x1| )
	const float shadowMapH = shadowMapY + std::sqrt( std::fabs(shadowProjMinMax.z) ); // math::sqrt( |y2| ) + math::sqrt( |y1| )

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
	// .xy are used to bias the SM-space projection; the values
	// of .z and .w are such that (invsqrt(xy + zz) + ww) ~= 1
	shadowTexProjCenter.x = 0.5f;
	shadowTexProjCenter.y = 0.5f;
	shadowTexProjCenter.z = FLT_MAX;
	shadowTexProjCenter.w = 1.0f;
	#endif
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
	const CMatrix44f lightMatrix = std::move(ComposeLightMatrix(sky->GetLight()));
	const CMatrix44f scaleMatrix = std::move(ComposeScaleMatrix(GetShadowProjectionScales(playerCam, lightMatrix)));

	// convert xy-diameter to radius
	shadowCam->SetFrustumScales(shadowProjScales * float4(0.5f, 0.5f, 1.0f, 1.0f));

	#if 0
	// reshape frustum (to maximize SM resolution); for culling we want
	// the scales-matrix applied to projMatrix instead of to viewMatrix
	// ((V*S) * P = V * (S*P); note that S * P is a pre-multiplication
	// so we express it as P * S in code)
	projMatrix[SHADOWMAT_TYPE_CULLING] = projMatrix[SHADOWMAT_TYPE_DRAWING];
	projMatrix[SHADOWMAT_TYPE_CULLING] *= scaleMatrix;

	// frustum-culling needs this form
	viewMatrix[SHADOWMAT_TYPE_CULLING].LoadIdentity();
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetX(std::move(lightMatrix.GetX()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetY(std::move(lightMatrix.GetY()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetZ(std::move(lightMatrix.GetZ()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].Transpose(); // invert rotation (R^T == R^{-1})
	viewMatrix[SHADOWMAT_TYPE_CULLING].Translate(-projMidPos[2]); // same as SetPos(mat * -pos)
	#else
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
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetX(std::move(lightMatrix.GetX()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetY(std::move(lightMatrix.GetY()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetZ(std::move(lightMatrix.GetZ()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetPos(projMidPos[2]);
	#endif

	// shaders need this form, projection into SM-space is done by shadow2DProj()
	// note: ShadowGenVertProg is a special case because it does not use uniforms
	viewMatrix[SHADOWMAT_TYPE_DRAWING].LoadIdentity();
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetX(std::move(lightMatrix.GetX()));
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetY(std::move(lightMatrix.GetY()));
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetZ(std::move(lightMatrix.GetZ()));
	viewMatrix[SHADOWMAT_TYPE_DRAWING].Scale(float3(scaleMatrix[0], scaleMatrix[5], scaleMatrix[10])); // extract (X.x, Y.y, Z.z)
	viewMatrix[SHADOWMAT_TYPE_DRAWING].Transpose();
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetPos(viewMatrix[SHADOWMAT_TYPE_DRAWING] * -projMidPos[2]);
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetPos(viewMatrix[SHADOWMAT_TYPE_DRAWING].GetPos() + scaleMatrix.GetPos()); // add z-bias

	#if 0
	// holds true in the non-KISS case, but needs an epsilon-tolerance equality test
	assert((viewMatrix[0] * projMatrix[0]) == (viewMatrix[1] * projMatrix[1]));
	#endif
}

void CShadowHandler::SetShadowCamera(CCamera* shadowCam)
{
	// first set matrices needed by shaders (including ShadowGenVertProg)
	shadowCam->SetProjMatrix(projMatrix[SHADOWMAT_TYPE_DRAWING]);
	shadowCam->SetViewMatrix(viewMatrix[SHADOWMAT_TYPE_DRAWING]);
	// update frustum, load matrices into gl_{ModelView,Projection}Matrix
	shadowCam->Update(false, false, false);
	shadowCam->UpdateLoadViewPort(0, 0, shadowMapSize, shadowMapSize);
	// next set matrices needed for SP visibility culling (these
	// are *NEVER* loaded into gl_{ModelView,Projection}Matrix!)
	shadowCam->SetProjMatrix(projMatrix[SHADOWMAT_TYPE_CULLING]);
	shadowCam->SetViewMatrix(viewMatrix[SHADOWMAT_TYPE_CULLING]);
	shadowCam->UpdateFrustum();
}


void CShadowHandler::SetupShadowTexSampler(unsigned int texUnit, bool enable) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);

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
	shadowMapFBO.Bind();

	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glShadeModel(GL_FLAT);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);


	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_SHADOW);
	CCamera* curCam = CCameraHandler::GetActiveCamera();


	SetShadowMapSizeFactors();
	SetShadowMatrix(prvCam, curCam);
	SetShadowCamera(curCam);

	if (globalRendering->haveARB) {
		// set the shadow-parameter registers
		// NOTE: so long as any part of Spring rendering still uses
		// ARB programs at run-time, these lines can not be removed
		// (all ARB programs share the same environment)
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 16, shadowTexProjCenter.x, shadowTexProjCenter.y, 0.0f, 0.0f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 17, shadowTexProjCenter.z, shadowTexProjCenter.z, 0.0f, 0.0f);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 18, shadowTexProjCenter.w, shadowTexProjCenter.w, 0.0f, 0.0f);
	}

	if (globalRendering->haveGLSL) {
		for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
			shadowGenProgs[i]->Enable();
			shadowGenProgs[i]->SetUniform4fv(0, &shadowTexProjCenter.x);
			shadowGenProgs[i]->Disable();
		}
	}

	if ((sky->GetLight())->GetLightIntensity() > 0.0f)
		DrawShadowPasses();


	CCameraHandler::SetActiveCamera(prvCam->GetCamType());
	prvCam->Update();


	glShadeModel(GL_SMOOTH);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
	projScales.z = cam->GetNearPlaneDist();
	projScales.w = cam->GetFarPlaneDist();
	return (shadowProjScales = projScales);
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
	// two sides, two points per side (plus one extra for MBS radius)
	float3 frustumPoints[2 * 2 + 1];

	#if 0
	{
		projPos = CalcShadowProjectionPos(cam, &frustumPoints[0]);

		// calculate radius of the minimally-bounding sphere around projected frustum
		for (unsigned int n = 0; n < 4; n++) {
			frustumPoints[4].x = std::max(frustumPoints[4].x, (frustumPoints[n] - projPos).SqLength());
		}

		const float maxMapDiameter = readMap->GetBoundingRadius() * 2.0f;
		const float frustumDiameter = std::sqrt(frustumPoints[4].x) * 2.0f;

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

		for (unsigned int n = 0; n < 4; n++) {
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
	float3 projPos;
	float3 frustumCenter;

	cam->CalcFrustumLines(0.0f, 0.0f, 1.0f, true);
	cam->ClipFrustumLines(-100.0f, mapDims.mapy * SQUARE_SIZE + 100.0f, true);

	const CCamera::FrustumLine* lines = cam->GetNegFrustumLines();

	// only need points on these lines
	const unsigned int planes[] = {CCamera::FRUSTUM_PLANE_LFT, CCamera::FRUSTUM_PLANE_RGT};

	for (unsigned int n = 0; n < 2; n++) {
		const CCamera::FrustumLine& fl = lines[ planes[n] ];

		// calculate xz-coordinates
		const float z0 = fl.minz, x0 = fl.base + (fl.dir * z0);
		const float z1 = fl.maxz, x1 = fl.base + (fl.dir * z1);

		// clamp points to map edges
		const float cx0 = Clamp(x0, 0.0f, float3::maxxpos);
		const float cz0 = Clamp(z0, 0.0f, float3::maxzpos);
		const float cx1 = Clamp(x1, 0.0f, float3::maxxpos);
		const float cz1 = Clamp(z1, 0.0f, float3::maxzpos);

		frustumPoints[n * 2 + 0] = float3(cx0, CGround::GetHeightReal(cx0, cz0, false), cz0); // far-point
		frustumPoints[n * 2 + 1] = float3(cx1, CGround::GetHeightReal(cx1, cz1, false), cz1); // near-point

		frustumCenter += frustumPoints[n * 2 + 0];
		frustumCenter += frustumPoints[n * 2 + 1];
	}

	projPos.x = frustumCenter.x * 0.25f;
	projPos.z = frustumCenter.z * 0.25f;
	projPos.y = CGround::GetHeightReal(projPos.x, projPos.z, false);
	return projPos;
}



#if 0
void CShadowHandler::CalcMinMaxView()
{
	// derive the size of the shadow-map from the
	// intersection points of the camera frustum
	// with the xz-plane
	CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_VISCUL);

	cam->CalcFrustumLines(0.0f, 0.0f, 1.0f, true);
	cam->ClipFrustumLines(-20000.0f, mapDims.mapy * SQUARE_SIZE + 20000.0f, true);

	shadowProjMinMax.x = -100.0f;
	shadowProjMinMax.y =  100.0f;
	shadowProjMinMax.z = -100.0f;
	shadowProjMinMax.w =  100.0f;

	//if someone could figure out how the frustum and nonlinear shadow transform really works (and not use the SJan trial and error method)
	//so that we can skip this sort of fudge factors it would be good
	float borderSize = 270.0f;
	float maxSize = camera->GetFarPlaneDist() * 0.75f;

	if (shadowMapSize == 1024) {
		borderSize *= 1.5f;
		maxSize *= 1.2f;
	}

	const CCamera::FrustumLine* lines = cam->GetNegFrustumLines();

	{
		for (int idx = 0; idx < /*negLines[*/4/*].sign*/; idx++) {
			const CCamera::FrustumLine& fl = lines[idx];

			if (fl.minz < fl.maxz) {
				float3 p[5];
				p[0] = float3(fl.base + fl.dir * fl.minz, 0.0f, fl.minz);
				p[1] = float3(fl.base + fl.dir * fl.maxz, 0.0f, fl.maxz);
				p[2] = float3(fl.base + fl.dir * fl.minz, readMap->initMaxHeight + 200, fl.minz);
				p[3] = float3(fl.base + fl.dir * fl.maxz, readMap->initMaxHeight + 200, fl.maxz);
				p[4] = projMidPos[2];

				for (int a = 0; a < 5; ++a) {
					const float xd = (p[a] - projMidPos[2]).dot(sunDirX);
					const float yd = (p[a] - projMidPos[2]).dot(sunDirY);

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
	}

	// xScale = (shadowProjMinMax.y - shadowProjMinMax.x) * 1.5f;
	// yScale = (shadowProjMinMax.w - shadowProjMinMax.z) * 1.5f;
}
#endif

