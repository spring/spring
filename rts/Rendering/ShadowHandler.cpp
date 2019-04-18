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
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


CONFIG(int, Shadows).defaultValue(2).headlessValue(-1).minimumValue(-1).safemodeValue(-1).description("Sets whether shadows are rendered.\n-1:=forceoff, 0:=off, 1:=full, 2:=fast (skip terrain)"); //FIXME document bitmask
CONFIG(int, ShadowMapSize).defaultValue(CShadowHandler::DEF_SHADOWMAP_SIZE).minimumValue(32).description("Sets the resolution of shadows. Higher numbers increase quality at the cost of performance.");

CShadowHandler shadowHandler;

bool CShadowHandler::shadowsSupported = false;
bool CShadowHandler::firstInit = true;


void CShadowHandler::Reload(const char* argv)
{
	int nextShadowConfig = (shadowConfig + 1) & 0xF;
	int nextShadowMapSize = shadowMapSize;

	if (argv != nullptr)
		(void) sscanf(argv, "%i %i", &nextShadowConfig, &nextShadowMapSize);

	// do nothing without a parameter change
	if (nextShadowConfig == shadowConfig && nextShadowMapSize == shadowMapSize)
		return;

	configHandler->Set("Shadows", nextShadowConfig & 0xF);
	configHandler->Set("ShadowMapSize", Clamp(nextShadowMapSize, int(MIN_SHADOWMAP_SIZE), int(MAX_SHADOWMAP_SIZE)));

	Kill();
	Init();
}

void CShadowHandler::Init()
{
	const bool tmpFirstInit = firstInit;
	firstInit = false;

	shadowConfig  = configHandler->GetInt("Shadows");
	shadowMapSize = configHandler->GetInt("ShadowMapSize");
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
	static const std::string shadowGenProgHandles[SHADOWGEN_PROGRAM_LAST] = {
		"ShadowGenShaderProgModel",
		"ShadowGenShaderProgMap",
		"ShadowGenShaderProgTree",
		"ShadowGenShaderProgProjectile",
		"ShadowGenShaderProgParticle",
	};

	// #version has to be added here because it is conditional
	static const std::string versionDef = "#version " + IntToString(globalRendering->supportFragDepthLayout? 420: 410) + "\n";
	static const std::string extraDefs =
		("#define SHADOWMATRIX_NONLINEAR " + IntToString(0) + "\n") +
		("#define SHADOWGEN_PER_FRAGMENT " + IntToString(0) + "\n") + // maps only
		("#define SUPPORT_CLIP_CONTROL " + IntToString(globalRendering->supportClipSpaceControl) + "\n") +
		("#define SUPPORT_DEPTH_LAYOUT " + IntToString(globalRendering->supportFragDepthLayout) + "\n");

	for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
		shadowGenProgs[i] = sh->CreateProgramObject("[ShadowHandler]", shadowGenProgHandles[i] + "GLSL");
	}

	{
		Shader::IProgramObject* po = shadowGenProgs[SHADOWGEN_PROGRAM_MODEL];

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelShadowGenVertProg.glsl", versionDef + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelShadowGenFragProg.glsl", versionDef + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();

		po->SetUniformLocation("shadowParams"  ); // idx 0
		po->SetUniformLocation("shadowViewMat" ); // idx 1
		po->SetUniformLocation("shadowProjMat" ); // idx 2
		po->SetUniformLocation("modelMat"      ); // idx 3
		po->SetUniformLocation("pieceMats"     ); // idx 4
		po->SetUniformLocation("alphaMaskTex"  ); // idx 5
		po->SetUniformLocation("alphaTestCtrl" ); // idx 6
		po->SetUniformLocation("upperClipPlane"); // idx 7
		po->SetUniformLocation("lowerClipPlane"); // idx 8

		po->Enable();
		po->SetUniform1i(5, 0);
		po->SetUniform2f(6, 0.5f, 0.5f);
		po->Disable();
		po->Validate();
	}
	{
		Shader::IProgramObject* po = shadowGenProgs[SHADOWGEN_PROGRAM_MAP];

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/MapShadowGenVertProg.glsl", versionDef + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/MapShadowGenFragProg.glsl", versionDef + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();

		po->SetUniformLocation("shadowParams" ); // idx 0
		po->SetUniformLocation("shadowViewMat"); // idx 1
		po->SetUniformLocation("shadowProjMat"); // idx 2

		#if 0
		// TODO
		po->Enable();
		po->SetUniform1i(3, 0); // alphaMaskTex
		po->SetUniform2f(4, mapInfo->map.voidAlphaMin, mapInfo->map.voidAlphaMax); // alphaTestCtrl
		po->Disable();
		#endif

		po->Validate();
	}
	{
		Shader::IProgramObject* po = shadowGenProgs[SHADOWGEN_PROGRAM_TREE];

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/TreeShadowGenVertProg.glsl", versionDef + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/TreeShadowGenFragProg.glsl", versionDef + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();

		po->SetUniformLocation("shadowParams" ); // idx 0
		po->SetUniformLocation("shadowViewMat"); // idx 1
		po->SetUniformLocation("shadowProjMat"); // idx 2
		po->SetUniformLocation("treeMat"      ); // idx 3
		po->SetUniformLocation("cameraDirX"   ); // idx 4
		po->SetUniformLocation("cameraDirY"   ); // idx 5
		po->SetUniformLocation("$dummy$"      ); // idx 6, unused
		po->SetUniformLocation("alphaMaskTex" ); // idx 7
		po->SetUniformLocation("alphaTestCtrl"); // idx 8

		po->Enable();
		po->SetUniform1i(7, 0);
		po->SetUniform2f(8, 0.5f, 0.5f);
		po->Disable();
		po->Validate();
	}
	{
		Shader::IProgramObject* po = shadowGenProgs[SHADOWGEN_PROGRAM_PROJECTILE];

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ProjectileShadowGenVertProg.glsl", versionDef + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ProjectileShadowGenFragProg.glsl", versionDef + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();

		po->SetUniformLocation("shadowParams" ); // idx 0
		po->SetUniformLocation("shadowViewMat"); // idx 1
		po->SetUniformLocation("shadowProjMat"); // idx 2
		po->SetUniformLocation("modelMat"     ); // idx 3
		po->SetUniformLocation("pieceMats"    ); // idx 4
		po->Validate();
	}
	{
		Shader::IProgramObject* po = shadowGenProgs[SHADOWGEN_PROGRAM_PARTICLE];

		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ParticleShadowGenVertProg.glsl", versionDef + extraDefs, GL_VERTEX_SHADER));
		po->AttachShaderObject(sh->CreateShaderObject("GLSL/ParticleShadowGenFragProg.glsl", versionDef + extraDefs, GL_FRAGMENT_SHADER));
		po->Link();

		po->SetUniformLocation("shadowParams" ); // idx 0
		po->SetUniformLocation("shadowViewMat"); // idx 1
		po->SetUniformLocation("shadowProjMat"); // idx 2
		po->SetUniformLocation("alphaMaskTex" ); // idx 3
		po->SetUniformLocation("alphaTestCtrl"); // idx 4

		po->Enable();
		po->SetUniform1i(3, 0);
		po->SetUniform2f(4, 0.3f, 0.3f);
		po->Disable();
		po->Validate();
	}

	for (int i = 0; i < SHADOWGEN_PROGRAM_LAST; i++) {
		shadowGenProgs[i]->Enable();
		shadowGenProgs[i]->SetUniform4fv(0, &shadowProjParams.x);
		shadowGenProgs[i]->Disable();
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

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		// Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
		shadowMapFBO.Bind();
		shadowMapFBO.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT);

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
	// some drivers/GPUs fail to render to GL_CLAMP_TO_BORDER (and GL_LINEAR may cause a drop in performance for them, too)
	{
		shadowMapFBO.Detach(GL_DEPTH_ATTACHMENT);
		glDeleteTextures(1, &shadowTexture);

		glGenTextures(1, &shadowTexture);
		glBindTexture(GL_TEXTURE_2D, shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		const GLint texFormat = globalRendering->support24bitDepthBuffer? GL_DEPTH_COMPONENT24: GL_DEPTH_COMPONENT16;
		glTexImage2D(GL_TEXTURE_2D, 0, texFormat, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		shadowMapFBO.AttachTexture(shadowTexture, GL_TEXTURE_2D, GL_DEPTH_ATTACHMENT);

		if (shadowMapFBO.CheckStatus("SHADOW-GL_CLAMP_TO_EDGE"))
			return true;
	}


	// ATI sometimes fails without an attached color texture, so check a few formats (not all supported texture formats are renderable)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		// 1st: try the smallest unsupported format (4bit per pixel)
		glGenTextures(1, &dummyColorTexture);
		glBindTexture(GL_TEXTURE_2D, dummyColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA4, shadowMapSize, shadowMapSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
		shadowMapFBO.AttachTexture(dummyColorTexture);

		if (shadowMapFBO.CheckStatus("SHADOW-GL_ALPHA4"))
			return true;

		// failed revert changes of 1st attempt
		shadowMapFBO.Detach(GL_COLOR_ATTACHMENT0);
		glDeleteTextures(1, &dummyColorTexture);

		// 2nd: try smallest standard format that must be renderable for OGL3
		shadowMapFBO.CreateRenderBuffer(GL_COLOR_ATTACHMENT0, GL_RED, shadowMapSize, shadowMapSize);

		if (shadowMapFBO.CheckStatus("SHADOW-GL_RED"))
			return true;
	}

	return false;
}


void CShadowHandler::DrawShadowPasses()
{
	inShadowPass = true;

	glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_POLYGON_BIT);
	glAttribStatePtr->EnableCullFace();
	glAttribStatePtr->CullFace(GL_BACK);

		eventHandler.DrawWorldShadow();

		if ((shadowGenBits & SHADOWGEN_BIT_TREE) != 0) {
			currentShadowPass = SHADOWGEN_PROGRAM_TREE;
			treeDrawer->DrawShadow();
			grassDrawer->DrawShadow();
		}

		if ((shadowGenBits & SHADOWGEN_BIT_PROJ) != 0) {
			currentShadowPass = SHADOWGEN_PROGRAM_PROJECTILE;
			projectileDrawer->DrawShadowPass();
		}

		if ((shadowGenBits & SHADOWGEN_BIT_MODEL) != 0) {
			currentShadowPass = SHADOWGEN_PROGRAM_MODEL;
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
		// glAttribStatePtr->CullFace(GL_FRONT);
		if ((shadowGenBits & SHADOWGEN_BIT_MAP) != 0) {
			currentShadowPass = SHADOWGEN_PROGRAM_MAP;
			readMap->GetGroundDrawer()->DrawShadowPass();
		}

		currentShadowPass = SHADOWGEN_PROGRAM_LAST;

	glAttribStatePtr->PopBits();

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
	return (CMatrix44f::OrthoProj(-scales.x * 0.5f, scales.x * 0.5f, -scales.y * 0.5f, scales.y * 0.5f, scales.w * 0.5f, -scales.w * 0.5f));
	// return (CMatrix44f(ZeroVector,  RgtVector * 2.0f / scales.x, UpVector * 2.0f / scales.y, FwdVector * 2.0f / (scales.w - scales.z)));
}


void CShadowHandler::SetShadowMatrix(CCamera* playerCam)
{
	const CMatrix44f lightMatrix = ComposeLightMatrix(sky->GetLight());
	const CMatrix44f scaleMatrix = ComposeScaleMatrix(shadowProjScales = GetShadowProjectionScales(playerCam, lightMatrix));

	#if 0
	// reshape frustum (to maximize SM resolution); for culling we want
	// the scales-matrix applied to projMatrix instead of to viewMatrix
	// ((V*S) * P = V * (S*P); note that S * P is a pre-multiplication
	// so we express it as P * S in code)
	projMatrix[SHADOWMAT_TYPE_CULLING] = projMatrix[SHADOWMAT_TYPE_DRAWING];
	projMatrix[SHADOWMAT_TYPE_CULLING] *= scaleMatrix;

	// frustum-culling needs this form
	viewMatrix[SHADOWMAT_TYPE_CULLING].LoadIdentity();
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetX(lightMatrix.GetX()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetY(lightMatrix.GetY()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetZ(lightMatrix.GetZ()));
	viewMatrix[SHADOWMAT_TYPE_CULLING].Transpose(); // invert rotation (R^T == R^{-1})
	viewMatrix[SHADOWMAT_TYPE_CULLING].Translate(-projMidPos); // same as SetPos(mat * -pos)
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
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetX(lightMatrix.GetX());
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetY(lightMatrix.GetY());
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetZ(lightMatrix.GetZ());
	viewMatrix[SHADOWMAT_TYPE_CULLING].SetPos(projMidPos);
	#endif

	// shaders need this form, projection into SM-space is done by shadow2DProj()
	viewMatrix[SHADOWMAT_TYPE_DRAWING].LoadIdentity();
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetX(lightMatrix.GetX());
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetY(lightMatrix.GetY());
	viewMatrix[SHADOWMAT_TYPE_DRAWING].SetZ(lightMatrix.GetZ());
	viewMatrix[SHADOWMAT_TYPE_DRAWING].Transpose();
	viewMatrix[SHADOWMAT_TYPE_DRAWING].Translate(-projMidPos);

	viewMatrix[SHADOWMAT_TYPE_DRAWING] = biasMatrix * scaleMatrix * viewMatrix[SHADOWMAT_TYPE_DRAWING];
}

void CShadowHandler::SetShadowCamera(CCamera* shadowCam)
{
	// first set matrices needed by shaders
	shadowCam->SetProjMatrix(projMatrix[SHADOWMAT_TYPE_DRAWING]);
	shadowCam->SetViewMatrix(viewMatrix[SHADOWMAT_TYPE_DRAWING]);

	shadowCam->SetAspectRatio(shadowProjScales.x / shadowProjScales.y);
	// convert xy-diameter to radius
	shadowCam->SetFrustumScales(shadowProjScales * float4(0.5f, 0.5f, 1.0f, 1.0f));
	shadowCam->UpdateFrustum();
	shadowCam->UpdateLoadViewPort(0, 0, shadowMapSize, shadowMapSize);

	// next set matrices needed for SP visibility culling
	shadowCam->SetProjMatrix(projMatrix[SHADOWMAT_TYPE_CULLING]);
	shadowCam->SetViewMatrix(viewMatrix[SHADOWMAT_TYPE_CULLING]);
	shadowCam->UpdateFrustum();
}


void CShadowHandler::SetupShadowTexSampler(unsigned int texUnit) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, shadowTexture);

	SetupShadowTexSamplerRaw();
}

void CShadowHandler::SetupShadowTexSamplerRaw() const
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

void CShadowHandler::ResetShadowTexSampler(unsigned int texUnit) const
{
	glActiveTexture(texUnit);
	glBindTexture(GL_TEXTURE_2D, 0);

	ResetShadowTexSamplerRaw();
}

void CShadowHandler::ResetShadowTexSamplerRaw() const
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}


void CShadowHandler::CreateShadows()
{
	// NOTE:
	//   we unbind later in WorldDrawer::GenerateIBLTextures() to save render
	//   context switches (which are one of the slowest OpenGL operations!)
	//   together with VP restoration
	shadowMapFBO.Bind();

	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->EnableDepthMask();
	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glAttribStatePtr->Clear(GL_DEPTH_BUFFER_BIT);


	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_SHADOW);
	CCamera* curCam = CCameraHandler::GetActiveCamera();


	SetShadowMatrix(prvCam);
	SetShadowCamera(curCam);

	if ((sky->GetLight())->GetLightIntensity() > 0.0f)
		DrawShadowPasses();


	CCameraHandler::SetActiveCamera(prvCam->GetCamType());
	prvCam->Update();

	glAttribStatePtr->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}



float4 CShadowHandler::GetShadowProjectionScales(CCamera* cam, const CMatrix44f& projMat) {
	float4 projScales;
	projScales.x = GetOrthoProjectedFrustumRadius(cam, projMat, projMidPos);
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

float CShadowHandler::GetOrthoProjectedFrustumRadius(CCamera* cam, const CMatrix44f& projMat, float3& projPos) const {
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

float3 CShadowHandler::CalcShadowProjectionPos(CCamera* cam, float3* frustumPoints) const
{
	float3 projPos;
	float2 mapPlanes = {readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f};

	cam->CalcFrustumLines(mapPlanes.x, mapPlanes.y, 1.0f, true);
	cam->ClipFrustumLines(-100.0f, mapDims.mapy * SQUARE_SIZE + 100.0f, true);

	const CCamera::FrustumLine* lines = cam->GetNegFrustumLines();

	// only need points on these lines
	constexpr unsigned int planes[] = {CCamera::FRUSTUM_PLANE_LFT, CCamera::FRUSTUM_PLANE_RGT};

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

		frustumPoints[n * 2 + 0] = float3(cx0, mapPlanes.x, cz0); // far-point
		frustumPoints[n * 2 + 1] = float3(cx1, mapPlanes.x, cz1); // near-point

		projPos += frustumPoints[n * 2 + 0];
		projPos += frustumPoints[n * 2 + 1];
	}

	return (projPos * 0.25f);
}

