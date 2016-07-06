/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDrawerState.hpp"
#include "UnitDrawer.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/SkyLight.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/myMath.h"



void IUnitDrawerState::PushTransform(const CCamera* cam) {
	// set model-drawing transform; view is combined with projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMultMatrixf(cam->GetViewMatrix());
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void IUnitDrawerState::PopTransform() {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}



float4 IUnitDrawerState::GetTeamColor(int team, float alpha) {
	assert(teamHandler->IsValidTeam(team));

	const   CTeam* t = teamHandler->Team(team);
	const uint8_t* c = t->color;

	return (float4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, alpha));
}



IUnitDrawerState* IUnitDrawerState::GetInstance(bool haveARB, bool haveGLSL) {
	IUnitDrawerState* instance = nullptr;

	if (!haveARB && !haveGLSL) {
		instance = new UnitDrawerStateFFP();
	} else {
		if (!haveGLSL) {
			instance = new UnitDrawerStateARB();
		} else {
			instance = new UnitDrawerStateGLSL();
		}
	}

	return instance;
}


void IUnitDrawerState::EnableCommon(const CUnitDrawer* ud, bool deferredPass) {
	PushTransform(camera);
	EnableTexturesCommon();

	SetActiveShader(shadowHandler->ShadowsLoaded(), deferredPass);
	assert(modelShaders[MODEL_SHADER_ACTIVE] != nullptr);
	modelShaders[MODEL_SHADER_ACTIVE]->Enable();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void IUnitDrawerState::DisableCommon(const CUnitDrawer* ud, bool deferredPass) {
	assert(modelShaders[MODEL_SHADER_ACTIVE] != nullptr);

	modelShaders[MODEL_SHADER_ACTIVE]->Disable();
	SetActiveShader(shadowHandler->ShadowsLoaded(), deferredPass);

	DisableTexturesCommon();
	PopTransform();
}


void IUnitDrawerState::EnableTexturesCommon() const {
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	if (shadowHandler->ShadowsLoaded())
		shadowHandler->SetupShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetEnvReflectionTextureID());

	glActiveTexture(GL_TEXTURE4);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSpecularTextureID());

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
}

void IUnitDrawerState::DisableTexturesCommon() const {
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	if (shadowHandler->ShadowsLoaded())
		shadowHandler->ResetShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glActiveTexture(GL_TEXTURE4);
	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}



// note: never actually called, SSP-state is tested
bool UnitDrawerStateFFP::CanEnable(const CUnitDrawer* ud) const {
	return (!ud->UseAdvShading());
}

void UnitDrawerStateFFP::Enable(const CUnitDrawer* ud, bool deferredPass, bool alphaPass) {
	glEnable(GL_LIGHTING);
	// only for the advshading=0 case
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
	glLightfv(GL_LIGHT1, GL_AMBIENT, sunLighting->unitAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, sunLighting->unitDiffuseColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, sunLighting->unitAmbientColor);
	glEnable(GL_LIGHT1);

	CUnitDrawer::SetupBasicS3OTexture1();
	CUnitDrawer::SetupBasicS3OTexture0();

	const float4 color = {1.0f, 1.0f, 1.0, mix(1.0f, ud->alphaValues.x, (1.0f * alphaPass))};

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &color.x);
	glColor4fv(&color.x);

	PushTransform(camera);
}

void UnitDrawerStateFFP::Disable(const CUnitDrawer* ud, bool) {
	PopTransform();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT1);

	CUnitDrawer::CleanupBasicS3OTexture1();
	CUnitDrawer::CleanupBasicS3OTexture0();
}


void UnitDrawerStateFFP::EnableTextures() const {
	glEnable(GL_LIGHTING);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}

void UnitDrawerStateFFP::DisableTextures() const {
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}

void UnitDrawerStateFFP::SetTeamColor(int team, const float2 alpha) const {
	// non-shader case via texture combiners
	const float4 m = {1.0f, 1.0f, 1.0f, alpha.x};

	glActiveTexture(GL_TEXTURE0);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, std::move(GetTeamColor(team, alpha.x)));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &m.x);
}

void UnitDrawerStateFFP::SetNanoColor(const float4& color) const {
	if (color.a > 0.0f) {
		UnitDrawerStateFFP::DisableTextures();
		glColorf4(color);
	} else {
		UnitDrawerStateFFP::EnableTextures();
		glColorf3(OnesVector);
	}
}




bool UnitDrawerStateARB::Init(const CUnitDrawer* ud) {
	if (!globalRendering->haveARB) {
		// not possible to do (ARB) shader-based model rendering
		return false;
	}
	if (!configHandler->GetBool("AdvUnitShading")) {
		// not allowed to do (ARB) shader-based model rendering
		return false;
	}

	// if GLEW_NV_vertex_program2 is supported, transparent objects are clipped against GL_CLIP_PLANE3
	const char* vertProgNamesARB[2] = {"ARB/units3o.vp", "ARB/units3o2.vp"};
	const char* fragProgNamesARB[2] = {"ARB/units3o.fp", "ARB/units3o_shadow.fp"};

	#define sh shaderHandler
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderDefARB", true);
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]->AttachShaderObject(sh->CreateShaderObject(vertProgNamesARB[GLEW_NV_vertex_program2], "", GL_VERTEX_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]->AttachShaderObject(sh->CreateShaderObject(fragProgNamesARB[0], "", GL_FRAGMENT_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]->Link();

	modelShaders[MODEL_SHADER_SHADOWED_STANDARD] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderAdvARB", true);
	modelShaders[MODEL_SHADER_SHADOWED_STANDARD]->AttachShaderObject(sh->CreateShaderObject(vertProgNamesARB[GLEW_NV_vertex_program2], "", GL_VERTEX_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_SHADOWED_STANDARD]->AttachShaderObject(sh->CreateShaderObject(fragProgNamesARB[1], "", GL_FRAGMENT_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_SHADOWED_STANDARD]->Link();

	// make the active shader non-NULL
	SetActiveShader(shadowHandler->ShadowsLoaded(), false);

	#undef sh
	return true;
}

void UnitDrawerStateARB::Kill() {
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer]");
}

bool UnitDrawerStateARB::CanEnable(const CUnitDrawer* ud) const {
	// ARB shaders should support vertex program + clipplanes
	// (used for water ref**ction passes) but only with option
	// ARB_position_invariant; this is present so skip the RHS
	return (ud->UseAdvShading() /*&& (!water->DrawReflectionPass() && !water->DrawRefractionPass())*/);
}

void UnitDrawerStateARB::Enable(const CUnitDrawer* ud, bool deferredPass, bool alphaPass) {
	EnableCommon(ud, deferredPass && false);

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(10, &sky->GetLight()->GetLightDir().x);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4f(11, sunLighting->unitDiffuseColor.x, sunLighting->unitDiffuseColor.y, sunLighting->unitDiffuseColor.z, 0.0f);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4f(12, sunLighting->unitAmbientColor.x, sunLighting->unitAmbientColor.y, sunLighting->unitAmbientColor.z, 1.0f); //!
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4f(13, camera->GetPos().x, camera->GetPos().y, camera->GetPos().z, 0.0f);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4f(10, 0.0f, 0.0f, 0.0f, sky->GetLight()->GetUnitShadowDensity());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4f(11, sunLighting->unitAmbientColor.x, sunLighting->unitAmbientColor.y, sunLighting->unitAmbientColor.z, 1.0f);

	glMatrixMode(GL_MATRIX0_ARB);
	glLoadMatrixf(shadowHandler->GetShadowMatrixRaw());
	glMatrixMode(GL_MODELVIEW);
}

void UnitDrawerStateARB::Disable(const CUnitDrawer* ud, bool) {
	DisableCommon(ud, false);
}


void UnitDrawerStateARB::EnableTextures() const { EnableTexturesCommon(); }
void UnitDrawerStateARB::DisableTextures() const { DisableTexturesCommon(); }

void UnitDrawerStateARB::EnableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Enable(); }
void UnitDrawerStateARB::DisableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Disable(); }


void UnitDrawerStateARB::SetTeamColor(int team, const float2 alpha) const {
	// NOTE:
	//   both UnitDrawer::DrawAlphaPass and FeatureDrawer::DrawAlphaPass
	//   disable advShading in case of ARB, so in that case we should end
	//   up in StateFFP::SetTeamColor
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(14, std::move(GetTeamColor(team, alpha.x)));
}

void UnitDrawerStateARB::SetNanoColor(const float4& color) const {
	if (color.a > 0.0f) {
		glColorf4(color);
	} else {
		glColorf3(OnesVector);
	}
}



bool UnitDrawerStateGLSL::Init(const CUnitDrawer* ud) {
	if (!globalRendering->haveGLSL) {
		// not possible to do (GLSL) shader-based model rendering
		return false;
	}
	if (!configHandler->GetBool("AdvUnitShading")) {
		// not allowed to do (GLSL) shader-based model rendering
		return false;
	}

	#define sh shaderHandler

	const GL::LightHandler* lightHandler = ud->GetLightHandler();
	const std::string shaderNames[MODEL_SHADER_COUNT - 1] = {
		"ModelShaderGLSL-NoShadowStandard",
		"ModelShaderGLSL-ShadowedStandard",
		"ModelShaderGLSL-NoShadowDeferred",
		"ModelShaderGLSL-ShadowedDeferred",
	};
	const std::string extraDefs =
		("#define BASE_DYNAMIC_MODEL_LIGHT " + IntToString(lightHandler->GetBaseLight()) + "\n") +
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler->GetMaxLights()) + "\n");

	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject("[UnitDrawer]", shaderNames[n], false);
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
		modelShaders[n]->SetUniform3fv(11, &sunLighting->unitAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(12, &sunLighting->unitDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(13, sky->GetLight()->GetUnitShadowDensity());
		// modelShaders[n]->SetUniform1f(16, 0.0f); // alphaPass
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader(shadowHandler->ShadowsLoaded(), false);

	#undef sh
	return true;
}

void UnitDrawerStateGLSL::Kill() {
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer]");
}

bool UnitDrawerStateGLSL::CanEnable(const CUnitDrawer* ud) const {
	return (ud->UseAdvShading());
}

void UnitDrawerStateGLSL::Enable(const CUnitDrawer* ud, bool deferredPass, bool alphaPass) {
	EnableCommon(ud, deferredPass);

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform3fv(6, &camera->GetPos()[0]);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(14, false, shadowHandler->GetShadowMatrixRaw());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(15, &(shadowHandler->GetShadowParams().x));

	const_cast<GL::LightHandler*>(ud->GetLightHandler())->Update(modelShaders[MODEL_SHADER_ACTIVE]);
}

void UnitDrawerStateGLSL::Disable(const CUnitDrawer* ud, bool deferredPass) {
	DisableCommon(ud, deferredPass);
}


void UnitDrawerStateGLSL::EnableTextures() const { EnableTexturesCommon(); }
void UnitDrawerStateGLSL::DisableTextures() const { DisableTexturesCommon(); }

void UnitDrawerStateGLSL::EnableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Enable(); }
void UnitDrawerStateGLSL::DisableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Disable(); }


void UnitDrawerStateGLSL::UpdateCurrentShaderSky(const CUnitDrawer* ud, const ISkyLight* skyLight) const {
	// note: the NOSHADOW shaders do not care about shadow-density
	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform3fv(5, &skyLight->GetLightDir().x);
		modelShaders[n]->SetUniform3fv(11, &sunLighting->unitAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(12, &sunLighting->unitDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(13, skyLight->GetUnitShadowDensity());
		modelShaders[n]->Disable();
	}
}


void UnitDrawerStateGLSL::SetTeamColor(int team, const float2 alpha) const {
	// NOTE: see UnitDrawerStateARB::SetTeamColor
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(9, std::move(GetTeamColor(team, alpha.x)));
	// modelShaders[MODEL_SHADER_ACTIVE]->SetUniform1f(16, alpha.y);
}

void UnitDrawerStateGLSL::SetNanoColor(const float4& color) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(10, color);
}

