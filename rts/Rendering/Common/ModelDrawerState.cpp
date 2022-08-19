/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ModelDrawerState.hpp"
#include "ModelDrawer.h"
#include "Game/Camera.h"
#include "Game/Game.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/SkyLight.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Matrix44f.h"
#include "System/Config/ConfigHandler.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"



bool IModelDrawerState::SetTeamColor(int team, float alpha) const
{
	// need this because we can be called by no-team projectiles
	if (!teamHandler.IsValidTeam(team))
		return false;

	// should be an assert, but projectiles (+FlyingPiece) would trigger it
	if (shadowHandler.InShadowPass())
		return false;

	return true;
}

void IModelDrawerState::SetupOpaqueDrawing(bool deferredPass) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * CModelDrawerConcept::WireFrameModeRef() + GL_FILL * (1 - CModelDrawerConcept::WireFrameModeRef()));

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	if (IsLegacy()) {
		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);
	}

	Enable(deferredPass, false);
}

void IModelDrawerState::ResetOpaqueDrawing(bool deferredPass) const
{
	Disable(deferredPass);

	if (IsLegacy())
		glDisable(GL_ALPHA_TEST);

	glPopAttrib();
}

void IModelDrawerState::SetupAlphaDrawing(bool deferredPass) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | (GL_COLOR_BUFFER_BIT * IsLegacy()));
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * CModelDrawerConcept::WireFrameModeRef() + GL_FILL * (1 - CModelDrawerConcept::WireFrameModeRef()));

	Enable(/*deferredPass always false*/ false, true);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (IsLegacy()) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.1f);
	}

	glDepthMask(GL_FALSE);
}

void IModelDrawerState::ResetAlphaDrawing(bool deferredPass) const
{
	Disable(/*deferredPass*/ false);
	glPopAttrib();
}


////////////// FFP ////////////////

/**
 * Set up the texture environment in texture unit 0
 * to give an S3O texture its team-colour.
 *
 * Also:
 * - call SetBasicTeamColour to set the team colour to transform to.
 * - Replace the output alpha channel. If not, only the team-coloured bits will show, if that. Or something.
 */
void CModelDrawerStateFFP::SetupBasicS3OTexture0()
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// RGB = Texture * (1 - Alpha) + Teamcolor * Alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

	// ALPHA = Ignore
}

/**
 * This sets the first texture unit to GL_MODULATE the colours from the
 * first texture unit with the current glColor.
 *
 * Normal S3O drawing sets the color to full white; translucencies
 * use this setup to 'tint' the drawn model.
 *
 * - Leaves glActivateTextureARB at the first unit.
 * - This doesn't tinker with the output alpha, either.
 */
void CModelDrawerStateFFP::SetupBasicS3OTexture1()
{
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// RGB = Primary Color * Previous
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

	// ALPHA = Current alpha * Alpha mask
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
}

void CModelDrawerStateFFP::CleanupBasicS3OTexture1()
{
	// reset texture1 state
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void CModelDrawerStateFFP::CleanupBasicS3OTexture0()
{
	// reset texture0 state
	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

CModelDrawerStateFFP::CModelDrawerStateFFP() {}
CModelDrawerStateFFP::~CModelDrawerStateFFP() {}

bool CModelDrawerStateFFP::SetTeamColor(int team, float alpha) const
{
	if (!IModelDrawerState::SetTeamColor(team, alpha))
		return false;

	// non-shader case via texture combiners
	const float4 m = { 1.0f, 1.0f, 1.0f, alpha };

	glActiveTexture(GL_TEXTURE0);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, std::move(CModelDrawerHelper::GetTeamColor(team, alpha)));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &m.x);

	return true;
}

void CModelDrawerStateFFP::Enable(bool deferredPass, bool alphaPass) const
{
	glEnable(GL_LIGHTING);
	// only for the advshading=0 case
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
	glLightfv(GL_LIGHT1, GL_AMBIENT, sunLighting->modelAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, sunLighting->modelDiffuseColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, sunLighting->modelSpecularColor);
	glEnable(GL_LIGHT1);

	CModelDrawerStateFFP::SetupBasicS3OTexture1();
	CModelDrawerStateFFP::SetupBasicS3OTexture0();

	const float4 color = { 1.0f, 1.0f, 1.0, mix(1.0f, alphaValues.x, (1.0f * alphaPass)) };

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &color.x);
	glColor4fv(&color.x);

	CModelDrawerHelper::PushTransform(camera);
}

void CModelDrawerStateFFP::Disable(bool deferredPass) const
{
	CModelDrawerHelper::PopTransform();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT1);

	CModelDrawerStateFFP::CleanupBasicS3OTexture1();
	CModelDrawerStateFFP::CleanupBasicS3OTexture0();
}

void CModelDrawerStateFFP::SetNanoColor(const float4& color) const
{
	if (color.a > 0.0f) {
		DisableTextures();
		glColorf4(color);
	}
	else {
		EnableTextures();
		glColorf3(OnesVector);
	}
}

void CModelDrawerStateFFP::EnableTextures() const
{
	glEnable(GL_LIGHTING);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}

void CModelDrawerStateFFP::DisableTextures() const
{
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}

////////////// GSSL ////////////////

CModelDrawerStateGLSL::CModelDrawerStateGLSL()
{
	if (!CanEnable())
		return;

	#define sh shaderHandler

	const GL::LightHandler* lightHandler = CModelDrawerConcept::GetLightHandler();
	static const std::string shaderNames[MODEL_SHADER_COUNT] = {
		"ModelShaderGLSL-NoShadowStandard",
		"ModelShaderGLSL-ShadowedStandard",
		"ModelShaderGLSL-NoShadowDeferred",
		"ModelShaderGLSL-ShadowedDeferred",
	};
	const std::string extraDefs =
		("#define BASE_DYNAMIC_MODEL_LIGHT " + IntToString(lightHandler->GetBaseLight()) + "\n") +
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler->GetMaxLights()) + "\n");

	for (uint32_t n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject(PO_CLASS, shaderNames[n]);
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProg.glsl", extraDefs, GL_VERTEX_SHADER));
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProg.glsl", extraDefs, GL_FRAGMENT_SHADER));

		modelShaders[n]->SetFlag("USE_SHADOWS", int((n & 1) == 1));
		modelShaders[n]->SetFlag("DEFERRED_MODE", int(n >= MODEL_SHADER_NOSHADOW_DEFERRED));
		modelShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
		modelShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
		modelShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
		modelShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
		modelShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
		modelShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

		modelShaders[n]->Link();
		modelShaders[n]->SetUniformLocation("diffuseTex");        // idx  0 (t1: diffuse + team-color)
		modelShaders[n]->SetUniformLocation("shadingTex");        // idx  1 (t2: spec/refl + self-illum)
		modelShaders[n]->SetUniformLocation("shadowTex");         // idx  2
		modelShaders[n]->SetUniformLocation("reflectTex");        // idx  3 (cube)
		modelShaders[n]->SetUniformLocation("specularTex");       // idx  4 (cube)
		modelShaders[n]->SetUniformLocation("sunDir");            // idx  5
		modelShaders[n]->SetUniformLocation("cameraPos");         // idx  6
		modelShaders[n]->SetUniformLocation("cameraMat");         // idx  7
		modelShaders[n]->SetUniformLocation("cameraMatInv");      // idx  8
		modelShaders[n]->SetUniformLocation("teamColor");         // idx  9
		modelShaders[n]->SetUniformLocation("nanoColor");         // idx 10
		modelShaders[n]->SetUniformLocation("sunAmbient");        // idx 11
		modelShaders[n]->SetUniformLocation("sunDiffuse");        // idx 12
		modelShaders[n]->SetUniformLocation("shadowDensity");     // idx 13
		modelShaders[n]->SetUniformLocation("shadowMatrix");      // idx 14
		modelShaders[n]->SetUniformLocation("shadowParams");      // idx 15
		// modelShaders[n]->SetUniformLocation("alphaPass");         // idx 16

		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
		modelShaders[n]->SetUniform1i(1, 1); // shadingTex  (idx 1, texunit 1)
		modelShaders[n]->SetUniform1i(2, 2); // shadowTex   (idx 2, texunit 2)
		modelShaders[n]->SetUniform1i(3, 3); // reflectTex  (idx 3, texunit 3)
		modelShaders[n]->SetUniform1i(4, 4); // specularTex (idx 4, texunit 4)
		modelShaders[n]->SetUniform3fv(5, &sky->GetLight()->GetLightDir().x);
		modelShaders[n]->SetUniform3fv(6, &camera->GetPos()[0]);
		modelShaders[n]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
		modelShaders[n]->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
		modelShaders[n]->SetUniform4f(9, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4f(10, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform3fv(11, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(12, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(13, sunLighting->modelShadowDensity);
		modelShaders[n]->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
		modelShaders[n]->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));
		// modelShaders[n]->SetUniform1f(16, 0.0f); // alphaPass
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader(shadowHandler.ShadowsLoaded(), false);

	#undef sh
}

CModelDrawerStateGLSL::~CModelDrawerStateGLSL()
{
	modelShaders.fill(nullptr);
	modelShader = nullptr;
	shaderHandler->ReleaseProgramObjects(PO_CLASS);
}

bool CModelDrawerStateGLSL::CanEnable() const { return globalRendering->haveGLSL && CModelDrawerConcept::UseAdvShading(); }
bool CModelDrawerStateGLSL::CanDrawDeferred() const { return CModelDrawerConcept::DeferredAllowed(); }

bool CModelDrawerStateGLSL::SetTeamColor(int team, float alpha) const
{
	if (!IModelDrawerState::SetTeamColor(team, alpha))
		return false;

	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	modelShader->SetUniform4fv(9, std::move(CModelDrawerHelper::GetTeamColor(team, alpha)));

	return true;
}

void CModelDrawerStateGLSL::Enable(bool deferredPass, bool alphaPass) const
{
	// body of former EnableCommon();
	CModelDrawerHelper::PushTransform(camera);
	CModelDrawerHelper::EnableTexturesCommon();

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);
	assert(modelShader != nullptr);
	modelShader->Enable();
	// end of EnableCommon();

	modelShader->SetUniform3fv(6, &camera->GetPos()[0]);
	modelShader->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
	modelShader->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
	modelShader->SetUniform3fv(5,  &sky->GetLight()->GetLightDir().x);
	modelShader->SetUniform3fv(11, &sunLighting->modelAmbientColor[0]);
	modelShader->SetUniform3fv(12, &sunLighting->modelDiffuseColor[0]);
	modelShader->SetUniform1f(13, sunLighting->modelShadowDensity);
	modelShader->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
	modelShader->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));

	CModelDrawerConcept::GetLightHandler()->Update(modelShader);
}

void CModelDrawerStateGLSL::Disable(bool deferredPass) const
{
	assert(modelShader != nullptr);

	modelShader->Disable();
	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);

	CModelDrawerHelper::DisableTexturesCommon();
	CModelDrawerHelper::PopTransform();
}

void CModelDrawerStateGLSL::SetNanoColor(const float4& color) const
{
	assert(modelShader->IsBound());
	modelShader->SetUniform4fv(10, color);
}

void CModelDrawerStateGLSL::EnableTextures() const { CModelDrawerHelper::EnableTexturesCommon(); }
void CModelDrawerStateGLSL::DisableTextures() const { CModelDrawerHelper::DisableTexturesCommon(); }

////////////// GL4 ////////////////

CModelDrawerStateGL4::CModelDrawerStateGL4()
{
	if (!CanEnable())
	return;

	#define sh shaderHandler

	const GL::LightHandler* lightHandler = CModelDrawerConcept::GetLightHandler();
	static const std::string shaderNames[MODEL_SHADER_COUNT] = {
		"ModelShaderGL4-NoShadowStandard",
		"ModelShaderGL4-ShadowedStandard",
		"ModelShaderGL4-NoShadowDeferred",
		"ModelShaderGL4-ShadowedDeferred",
	};

	for (uint32_t n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject(PO_CLASS, shaderNames[n]);
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProgGL4.glsl", "", GL_VERTEX_SHADER));
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProgGL4.glsl", "", GL_FRAGMENT_SHADER));

		modelShaders[n]->SetFlag("USE_SHADOWS", int((n & 1) == 1));
		modelShaders[n]->SetFlag("DEFERRED_MODE", int(n >= MODEL_SHADER_NOSHADOW_DEFERRED));
		modelShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
		modelShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
		modelShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
		modelShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
		modelShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
		modelShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

		modelShaders[n]->Link();
		modelShaders[n]->Enable();
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader(shadowHandler.ShadowsLoaded(), false);

	#undef sh
}

CModelDrawerStateGL4::~CModelDrawerStateGL4()
{
	modelShaders.fill(nullptr);
	modelShader = nullptr;
	shaderHandler->ReleaseProgramObjects(PO_CLASS);
}

bool CModelDrawerStateGL4::CanEnable() const { return globalRendering->haveGL4 && CModelDrawerConcept::UseAdvShading(); }
bool CModelDrawerStateGL4::CanDrawDeferred() const { return CModelDrawerConcept::DeferredAllowed(); }

bool CModelDrawerStateGL4::SetTeamColor(int team, float alpha) const
{
	if (!IModelDrawerState::SetTeamColor(team, alpha))
		return false;

	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	modelShader->SetUniform("teamColorAlpha", alpha);

	return true;
}

void CModelDrawerStateGL4::Enable(bool deferredPass, bool alphaPass) const
{
	// body of former EnableCommon();
	CModelDrawerHelper::EnableTexturesCommon();

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);
	assert(modelShader != nullptr);
	modelShader->Enable();

	switch (game->GetDrawMode())
	{
	case CGame::GameDrawMode::gameReflectionDraw: {
		glEnable(GL_CLIP_DISTANCE2);
		SetCameraMode(ShaderCameraModes::REFLCT_CAMERA);
	} break;
	case CGame::GameDrawMode::gameRefractionDraw: {
		glEnable(GL_CLIP_DISTANCE2);
		SetCameraMode(ShaderCameraModes::REFRAC_CAMERA);
	} break;
	default: SetCameraMode(ShaderCameraModes::NORMAL_CAMERA); break;
	}

	float gtThreshold = mix(0.5, 0.1, static_cast<float>(alphaPass));
	modelShader->SetUniform("alphaCtrl", gtThreshold, 1.0f, 0.0f, 0.0f); // test > 0.1 | 0.5

	// end of EnableCommon();
}

void CModelDrawerStateGL4::Disable(bool deferredPass) const
{
	assert(modelShader != nullptr);

	modelShader->Disable();

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);

	switch (game->GetDrawMode())
	{
	case CGame::GameDrawMode::gameReflectionDraw: {
		glDisable(GL_CLIP_DISTANCE2);
	} break;
	case CGame::GameDrawMode::gameRefractionDraw: {
		glDisable(GL_CLIP_DISTANCE2);
	} break;
	default: {} break;
	}

	CModelDrawerHelper::DisableTexturesCommon();
}

void CModelDrawerStateGL4::SetNanoColor(const float4& color) const
{
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	modelShader->SetUniform("nanoColor", color.r, color.g, color.b, color.a);
}

void CModelDrawerStateGL4::EnableTextures() const { CModelDrawerHelper::EnableTexturesCommon(); }
void CModelDrawerStateGL4::DisableTextures() const { CModelDrawerHelper::DisableTexturesCommon(); }

void CModelDrawerStateGL4::SetColorMultiplier(float r, float g, float b, float a) const
{
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());
	modelShader->SetUniform("colorMult", r, g, b, a);
}

ShaderCameraModes CModelDrawerStateGL4::SetCameraMode(ShaderCameraModes scm_) const
{
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	std::swap(scm, scm_);
	modelShader->SetUniform("cameraMode", static_cast<int>(scm));

	switch (scm)
	{
	case ShaderCameraModes::REFLCT_CAMERA:
		SetClipPlane(2, { 0.0f,  1.0f, 0.0f, 0.0f });
		break;
	case ShaderCameraModes::REFRAC_CAMERA:
		SetClipPlane(2, { 0.0f, -1.0f, 0.0f, 0.0f });
		break;
	default:
		SetClipPlane(2  /* default, no clipping  */);
		break;
	}

	return scm_; //old state
}

ShaderMatrixModes CModelDrawerStateGL4::SetMatrixMode(ShaderMatrixModes smm_) const
{
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	std::swap(smm, smm_);
	modelShader->SetUniform("matrixMode", static_cast<int>(smm));

	return smm_; //old state
}

ShaderShadingModes CModelDrawerStateGL4::SetShadingMode(ShaderShadingModes ssm_) const
{
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	std::swap(ssm, ssm_);
	modelShader->SetUniform("shadingMode", static_cast<int>(ssm));

	return ssm_; //old state
}

void CModelDrawerStateGL4::SetStaticModelMatrix(const CMatrix44f& mat) const
{
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	modelShader->SetUniformMatrix4x4("staticModelMatrix", false, &mat.m[0]);
}

void CModelDrawerStateGL4::SetClipPlane(uint8_t idx, const float4& cp) const
{
	switch (idx)
	{
	case 0: //upper construction clip plane
		modelShader->SetUniform("clipPlane0", cp.x, cp.y, cp.z, cp.w);
		break;
	case 1: //lower construction clip plane
		modelShader->SetUniform("clipPlane1", cp.x, cp.y, cp.z, cp.w);
		break;
	case 2: //water clip plane
		modelShader->SetUniform("clipPlane2", cp.x, cp.y, cp.z, cp.w);
		break;
	default:
		assert(false);
		break;
	}
}

IModelDrawerState::IModelDrawerState()
{
	modelShaders.fill(nullptr);

	//dup with every instance, but ok
	alphaValues.x = std::max(0.11f, std::min(1.0f, 1.0f - configHandler->GetFloat("UnitTransparency")));
	alphaValues.y = std::min(1.0f, alphaValues.x + 0.1f);
	alphaValues.z = std::min(1.0f, alphaValues.x + 0.2f);
	alphaValues.w = std::min(1.0f, alphaValues.x + 0.4f);
}

bool IModelDrawerState::IsValid() const
{
	bool valid = true;
	for (auto ms : modelShaders) {
		if (!ms)
			continue;

		valid &= ms->IsValid();
	}

	return valid;
}
