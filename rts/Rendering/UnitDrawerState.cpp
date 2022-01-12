/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDrawerState.hpp"
#include "UnitDrawer.h"
#include "Game/Camera.h"
// #include "Lua/LuaMaterial.h"
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
#include "System/SpringMath.h"
#include "System/StringUtil.h"

static std::array<const CMatrix44f, 128> dummyPieceMatrices;


const CMatrix44f& IUnitDrawerState::GetDummyPieceMatrixRef(size_t idx) { return  dummyPieceMatrices[idx]; }
const CMatrix44f* IUnitDrawerState::GetDummyPieceMatrixPtr(size_t idx) { return &dummyPieceMatrices[idx]; }

float4 IUnitDrawerState::GetTeamColor(int team, float alpha) {
	assert(teamHandler.IsValidTeam(team));

	const   CTeam* t = teamHandler.Team(team);
	const uint8_t* c = t->color;

	return (float4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, alpha));
}



IUnitDrawerState* IUnitDrawerState::GetInstance(bool nopState, bool luaState) {
	if (nopState)
		return (new UnitDrawerStateNOP());
	if (luaState)
		return (new UnitDrawerStateLUA());

	return (new UnitDrawerStateGLSL());
}


void IUnitDrawerState::EnableCommon(const CUnitDrawer* ud, bool deferredPass) {
	EnableTexturesCommon();

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);
	assert(modelShaders[MODEL_SHADER_ACTIVE] != nullptr);
	modelShaders[MODEL_SHADER_ACTIVE]->Enable();

	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void IUnitDrawerState::DisableCommon(const CUnitDrawer* ud, bool deferredPass) {
	assert(modelShaders[MODEL_SHADER_ACTIVE] != nullptr);

	modelShaders[MODEL_SHADER_ACTIVE]->Disable();
	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);

	DisableTexturesCommon();
}


void IUnitDrawerState::EnableTexturesCommon() const {
	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetEnvReflectionTextureID());

	glActiveTexture(GL_TEXTURE0);
}

void IUnitDrawerState::DisableTexturesCommon() const {
	if (shadowHandler.ShadowsLoaded())
		shadowHandler.ResetShadowTexSampler(GL_TEXTURE2);

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
		("#define NUM_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler->NumConfigLights()) + "\n") +
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(GL::LightHandler::MaxConfigLights()) + "\n") +
		("#define MAX_LIGHT_UNIFORM_VECS "   + IntToString(GL::LightHandler::MaxUniformVecs()) + "\n") +
		("#define MDL_CLIP_PLANE_IDX "       + IntToString(            IWater::ClipPlaneIndex()) + "\n") +
		("#define MDL_FRAGDATA_COUNT "       + IntToString(GL::GeometryBuffer::ATTACHMENT_COUNT) + "\n");

	const float3 cameraPos = camera->GetPos();
	const float3 fogParams = {sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist()};

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

		modelShaders[n]->SetUniformLocation("sunDir");            // idx  4

		modelShaders[n]->SetUniformLocation("pieceMatrices[0]");  // idx  5
		modelShaders[n]->SetUniformLocation("modelMatrix");       // idx  6
		modelShaders[n]->SetUniformLocation("viewMatrix");        // idx  7
		modelShaders[n]->SetUniformLocation("projMatrix");        // idx  8
		modelShaders[n]->SetUniformLocation("fogParams");         // idx  9
		modelShaders[n]->SetUniformLocation("waterClipPlane");    // idx 10
		modelShaders[n]->SetUniformLocation("upperClipPlane");    // idx 11
		modelShaders[n]->SetUniformLocation("lowerClipPlane");    // idx 12

		modelShaders[n]->SetUniformLocation("cameraPos");         // idx 13
		modelShaders[n]->SetUniformLocation("teamColor");         // idx 14
		modelShaders[n]->SetUniformLocation("nanoColor");         // idx 15
		modelShaders[n]->SetUniformLocation("fogColor");          // idx 16
		modelShaders[n]->SetUniformLocation("sunAmbient");        // idx 17
		modelShaders[n]->SetUniformLocation("sunDiffuse");        // idx 18
		modelShaders[n]->SetUniformLocation("sunSpecular");       // idx 19
		modelShaders[n]->SetUniformLocation("specularExponent");  // idx 20
		modelShaders[n]->SetUniformLocation("shadowDensity");     // idx 21
		modelShaders[n]->SetUniformLocation("shadowMatrix");      // idx 22
		modelShaders[n]->SetUniformLocation("shadowParams");      // idx 23
		modelShaders[n]->SetUniformLocation("alphaTestCtrl");     // idx 24
		modelShaders[n]->SetUniformLocation("gammaExponent");     // idx 25
		modelShaders[n]->SetUniformLocation("fwdDynLights");      // idx 26

		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
		modelShaders[n]->SetUniform1i(1, 1); // shadingTex  (idx 1, texunit 1)
		modelShaders[n]->SetUniform1i(2, 2); // shadowTex   (idx 2, texunit 2)
		modelShaders[n]->SetUniform1i(3, 3); // reflectTex  (idx 3, texunit 3)

		modelShaders[n]->SetUniform3fv(4, sky->GetLight()->GetLightDir());
		modelShaders[n]->SetUniform3fv(9, &fogParams.x);
		modelShaders[n]->SetUniform4fv(10, IWater::ModelNullClipPlane());
		modelShaders[n]->SetUniform4fv(11, IWater::ModelNullClipPlane());
		modelShaders[n]->SetUniform4fv(12, IWater::ModelNullClipPlane());
		modelShaders[n]->SetUniform3fv(13, &cameraPos.x);
		modelShaders[n]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
		modelShaders[n]->SetUniformMatrix4fv(8, false, camera->GetProjectionMatrix());
		modelShaders[n]->SetUniform4f(14, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4f(15, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4fv(16, sky->fogColor);
		modelShaders[n]->SetUniform3fv(17, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(18, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform3fv(19, &sunLighting->modelSpecularColor[0]);
		modelShaders[n]->SetUniform1f(20, sunLighting->specularExponent);

		modelShaders[n]->SetUniform1f(21, sunLighting->modelShadowDensity);
		modelShaders[n]->SetUniformMatrix4fv(22, false, shadowHandler.GetShadowViewMatrixRaw());
		modelShaders[n]->SetUniform4fv(23, shadowHandler.GetShadowParams());
		modelShaders[n]->SetUniform4fv(24, float4{0.0f, 0.0f, 0.0f, 1.0f}); // alphaTestCtrl
		modelShaders[n]->SetUniform1f(25, globalRendering->gammaExponent);
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader(shadowHandler.ShadowsLoaded(), false);

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
	const float3 fogParams = {sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist()};

	Shader::IProgramObject* shader = modelShaders[MODEL_SHADER_ACTIVE];

	const GL::LightHandler* cLightHandler = ud->GetLightHandler();
	      GL::LightHandler* mLightHandler = const_cast<GL::LightHandler*>(cLightHandler); // XXX

	if (cLightHandler->NumConfigLights() > 0) {
		mLightHandler->Update();
		shader->SetUniform4fv(26, cLightHandler->NumUniformVecs(), cLightHandler->GetRawLightDataPtr());
	}

	shader->SetUniform3fv(9, &fogParams.x);
	shader->SetUniform3fv(13, &cameraPos.x);
	shader->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
	shader->SetUniformMatrix4fv(8, false, camera->GetProjectionMatrix());
	shader->SetUniformMatrix4fv(22, false, shadowHandler.GetShadowViewMatrixRaw());
	shader->SetUniform4fv(23, shadowHandler.GetShadowParams());
	shader->SetUniform1f(25, globalRendering->gammaExponent);
}

void UnitDrawerStateGLSL::Disable(const CUnitDrawer* ud, bool deferredPass) {
	DisableCommon(ud, deferredPass);
}


void UnitDrawerStateGLSL::EnableTextures() const { EnableTexturesCommon(); }
void UnitDrawerStateGLSL::DisableTextures() const { DisableTexturesCommon(); }

void UnitDrawerStateGLSL::EnableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Enable(); }
void UnitDrawerStateGLSL::DisableShaders(const CUnitDrawer*) { modelShaders[MODEL_SHADER_ACTIVE]->Disable(); }


void UnitDrawerStateGLSL::SetSkyLight(const ISkyLight* skyLight) const {
	// note: the NOSHADOW shaders do not care about shadow-density
	for (unsigned int n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform3fv(4, skyLight->GetLightDir());
		modelShaders[n]->SetUniform3fv(17, &sunLighting->modelAmbientColor.x);
		modelShaders[n]->SetUniform3fv(18, &sunLighting->modelDiffuseColor.x);
		modelShaders[n]->SetUniform3fv(19, &sunLighting->modelSpecularColor.x);
		modelShaders[n]->SetUniform1f(20, sunLighting->specularExponent);
		modelShaders[n]->SetUniform1f(21, sunLighting->modelShadowDensity);
		modelShaders[n]->Disable();
	}
}


void UnitDrawerStateGLSL::SetAlphaTest(const float4& params) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(24, params);
}

void UnitDrawerStateGLSL::SetTeamColor(int team, const float2 alpha) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(14, GetTeamColor(team, alpha.x));
}

void UnitDrawerStateGLSL::SetNanoColor(const float4& color) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(15, color);
}


void UnitDrawerStateGLSL::SetMatrices(const CMatrix44f& modelMat, const CMatrix44f* pieceMats, size_t numPieceMats) const {
	Shader::IProgramObject* po = nullptr;

	if (!shadowHandler.InShadowPass()) {
		po = modelShaders[MODEL_SHADER_ACTIVE];

		assert(shadowHandler.GetCurrentPass() == CShadowHandler::SHADOWGEN_PROGRAM_LAST);
		assert(po->IsBound());

		if (numPieceMats > 0)
			po->SetUniformMatrix4fv(5, -int(std::min(numPieceMats, dummyPieceMatrices.size())), false, &pieceMats[0].m[0]);

		po->SetUniformMatrix4fv(6, false, modelMat);
	} else {
		po = shadowHandler.GetCurrentShadowGenProg();

		assert(shadowHandler.GetCurrentPass() == CShadowHandler::SHADOWGEN_PROGRAM_MODEL || shadowHandler.GetCurrentPass() == CShadowHandler::SHADOWGEN_PROGRAM_PROJECTILE);
		assert(po->IsBound());

		if (numPieceMats > 0)
			po->SetUniformMatrix4fv(4, -int(std::min(numPieceMats, dummyPieceMatrices.size())), false, &pieceMats[0].m[0]);

		// {Unit,Projectile}Drawer::DrawShadowPass sets view and proj
		po->SetUniformMatrix4fv(3, false, modelMat);
	}
}


void UnitDrawerStateGLSL::SetWaterClipPlane(const DrawPass::e& drawPass) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());

	switch (drawPass) {
		case DrawPass::WaterReflection: { modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(10, IWater::ModelReflClipPlane()); } break;
		case DrawPass::WaterRefraction: { modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(10, IWater::ModelRefrClipPlane()); } break;
		default                       : {                                                                                     } break;
	}
}

void UnitDrawerStateGLSL::SetBuildClipPlanes(const float4& upper, const float4& lower) const {
	assert(modelShaders[MODEL_SHADER_ACTIVE]->IsBound());
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(11, upper);
	modelShaders[MODEL_SHADER_ACTIVE]->SetUniform4fv(12, lower);
}

