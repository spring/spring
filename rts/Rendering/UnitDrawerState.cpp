/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// TODO: LuaUnitDrawerState for objects using custom LuaMaterial shaders
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
#include "System/StringUtil.h"



float4 IUnitDrawerState::GetTeamColor(int team, float alpha) {
	assert(teamHandler->IsValidTeam(team));

	const   CTeam* t = teamHandler->Team(team);
	const uint8_t* c = t->color;

	return (float4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, alpha));
}



IUnitDrawerState* IUnitDrawerState::GetInstance(bool nopState) {
	if (nopState)
		return (new UnitDrawerStateNOP());

	return (new UnitDrawerStateGLSL());
}


void IUnitDrawerState::EnableCommon(const CUnitDrawer* ud, bool deferredPass) {
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
}


void IUnitDrawerState::EnableTexturesCommon() const {
	if (shadowHandler->ShadowsLoaded())
		shadowHandler->SetupShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler->GetEnvReflectionTextureID());

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler->GetSpecularTextureID());

	glActiveTexture(GL_TEXTURE0);
}

void IUnitDrawerState::DisableTexturesCommon() const {
	if (shadowHandler->ShadowsLoaded())
		shadowHandler->ResetShadowTexSampler(GL_TEXTURE2, true);

	glActiveTexture(GL_TEXTURE0);
}



bool UnitDrawerStateGLSL::Init(const CUnitDrawer* ud) {
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
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler->GetMaxLights()) + "\n") +
		("#define MDL_CLIP_PLANE_IDX "       + IntToString(            IWater::ClipPlaneIndex()) + "\n") +
		("#define MDL_FRAGDATA_COUNT "       + IntToString(GL::GeometryBuffer::ATTACHMENT_COUNT) + "\n");

	const float3 cameraPos = camera->GetPos();
	const float3 fogParams = {sky->fogStart, sky->fogEnd, globalRendering->viewRange};

	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject("[UnitDrawer]", shaderNames[n]);
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

		modelShaders[n]->SetUniformLocation("pieceMatrices");     // idx  6
		modelShaders[n]->SetUniformLocation("modelMatrix");       // idx  7
		modelShaders[n]->SetUniformLocation("viewMatrix");        // idx  8
		modelShaders[n]->SetUniformLocation("projMatrix");        // idx  9
		modelShaders[n]->SetUniformLocation("fogParams");         // idx 10
		modelShaders[n]->SetUniformLocation("waterClipPlane");    // idx 11
		modelShaders[n]->SetUniformLocation("upperClipPlane");    // idx 12
		modelShaders[n]->SetUniformLocation("lowerClipPlane");    // idx 13

		modelShaders[n]->SetUniformLocation("cameraPos");         // idx 14
		modelShaders[n]->SetUniformLocation("teamColor");         // idx 15
		modelShaders[n]->SetUniformLocation("nanoColor");         // idx 16
		modelShaders[n]->SetUniformLocation("fogColor");          // idx 17
		modelShaders[n]->SetUniformLocation("sunAmbient");        // idx 18
		modelShaders[n]->SetUniformLocation("sunDiffuse");        // idx 19
		modelShaders[n]->SetUniformLocation("shadowDensity");     // idx 20
		modelShaders[n]->SetUniformLocation("shadowMatrix");      // idx 21
		modelShaders[n]->SetUniformLocation("shadowParams");      // idx 22
		// modelShaders[n]->SetUniformLocation("alphaPass");         // idx 23

		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
		modelShaders[n]->SetUniform1i(1, 1); // shadingTex  (idx 1, texunit 1)
		modelShaders[n]->SetUniform1i(2, 2); // shadowTex   (idx 2, texunit 2)
		modelShaders[n]->SetUniform1i(3, 3); // reflectTex  (idx 3, texunit 3)
		modelShaders[n]->SetUniform1i(4, 4); // specularTex (idx 4, texunit 4)
		modelShaders[n]->SetUniform3fv(5, sky->GetLight()->GetLightDir());
		modelShaders[n]->SetUniform3fv(10, &fogParams.x);
		modelShaders[n]->SetUniform4fv(11, IWater::ModelNullClipPlane());
		modelShaders[n]->SetUniform4fv(12, IWater::ModelNullClipPlane());
		modelShaders[n]->SetUniform4fv(13, IWater::ModelNullClipPlane());
		modelShaders[n]->SetUniform3fv(14, &cameraPos.x);
		modelShaders[n]->SetUniformMatrix4fv(8, false, camera->GetViewMatrix());
		modelShaders[n]->SetUniformMatrix4fv(9, false, camera->GetProjectionMatrix());
		modelShaders[n]->SetUniform4f(15, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4f(16, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4fv(17, sky->fogColor);
		modelShaders[n]->SetUniform3fv(18, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(19, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(20, sunLighting->modelShadowDensity);
		modelShaders[n]->SetUniformMatrix4fv(21, false, shadowHandler->GetShadowViewMatrixRaw());
		modelShaders[n]->SetUniform4fv(22, shadowHandler->GetShadowParams());
		// modelShaders[n]->SetUniform1f(23, 0.0f); // alphaPass
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

void UnitDrawerStateGLSL::Enable(const CUnitDrawer* ud, bool deferredPass, bool alphaPass) {
	EnableCommon(ud, deferredPass);

	const float3 cameraPos = camera->GetPos();
	const float3 fogParams = {sky->fogStart, sky->fogEnd, globalRendering->viewRange};

	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform3fv(10, &fogParams.x);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform3fv(14, &cameraPos.x);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(8, false, camera->GetViewMatrix());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(9, false, camera->GetProjectionMatrix());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniformMatrix4fv(21, false, shadowHandler->GetShadowViewMatrixRaw());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(22, shadowHandler->GetShadowParams());

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
		modelShaders[n]->SetUniform3fv(5, skyLight->GetLightDir());
		modelShaders[n]->SetUniform3fv(18, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(19, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(20, sunLighting->modelShadowDensity);
		modelShaders[n]->Disable();
	}
}


void UnitDrawerStateGLSL::SetTeamColor(int team, const float2 alpha) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(15, std::move(GetTeamColor(team, alpha.x)));
}

void UnitDrawerStateGLSL::SetNanoColor(const float4& color) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(16, color);
}

void UnitDrawerStateGLSL::SetMatrices(const CMatrix44f& modelMat, const std::vector<CMatrix44f>& pieceMats) const {
	Shader::IProgramObject* po = nullptr;

	if (!shadowHandler->InShadowPass()) {
		po = modelShaders[MODEL_SHADER_ACTIVE];

		assert(shadowHandler->GetCurrentPass() == CShadowHandler::SHADOWGEN_PROGRAM_LAST);
		assert(po->IsBound());

		if (!pieceMats.empty())
			po->SetUniformMatrix4fv(6, std::min(pieceMats.size(), size_t(128)), false, &pieceMats[0].m[0]);

		po->SetUniformMatrix4fv(7, false, modelMat);
	} else {
		po = shadowHandler->GetCurrentShadowGenProg();

		assert(shadowHandler->GetCurrentPass() == CShadowHandler::SHADOWGEN_PROGRAM_MODEL || shadowHandler->GetCurrentPass() == CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE);
		assert(po->IsBound());

		if (!pieceMats.empty())
			po->SetUniformMatrix4fv(4, std::min(pieceMats.size(), size_t(128)), false, &pieceMats[0].m[0]);

		// {Unit,Projectile}Drawer::DrawShadowPass sets view and proj
		po->SetUniformMatrix4fv(3, false, modelMat);
	}
}

void UnitDrawerStateGLSL::SetWaterClipPlane(const DrawPass::e& drawPass) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	switch (drawPass) {
		case DrawPass::WaterReflection: { modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(11, IWater::ModelReflClipPlane()); } break;
		case DrawPass::WaterRefraction: { modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(11, IWater::ModelRefrClipPlane()); } break;
		default: {} break;
	}
}

void UnitDrawerStateGLSL::SetBuildClipPlanes(const float4& upper, const float4& lower) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(12, upper);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(13, lower);
}

