/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFRenderState.h"
#include "SMFGroundDrawer.h"
#include "SMFReadMap.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/SkyLight.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/MapRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/float4.h"
#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"

#define SMF_TEXSQUARE_SIZE 1024.0f


ISMFRenderState* ISMFRenderState::GetInstance(bool nopState, bool luaShaders) {
	if (nopState)
		return (new SMFRenderStateNOP());

	return (new SMFRenderStateGLSL(luaShaders));
}


bool SMFRenderStateGLSL::Init(const CSMFGroundDrawer* smfGroundDrawer) {
	const GL::LightHandler* lightHandler = smfGroundDrawer->GetLightHandler();

	const std::string names[GLSL_SHADER_COUNT - 1] = {
		"SMFShaderGLSL-Standard",
		"SMFShaderGLSL-Deferred",
	};
	const std::string defs =
		("#define SMF_TEXSQUARE_SIZE "     + FloatToString(                  SMF_TEXSQUARE_SIZE) + "\n") +
		("#define SMF_INTENSITY_MULT "     + FloatToString(CGlobalRendering::SMF_INTENSITY_MULT) + "\n") +
		("#define SMF_CLIP_PLANE_IDX "     + IntToString(            IWater::ClipPlaneIndex()) + "\n") +
		("#define SMF_FRAGDATA_COUNT "     + IntToString(GL::GeometryBuffer::ATTACHMENT_COUNT) + "\n") +
		("#define NUM_DYNAMIC_MAP_LIGHTS " + IntToString(    lightHandler->NumConfigLights()) + "\n") +
		("#define MAX_DYNAMIC_MAP_LIGHTS " + IntToString(GL::LightHandler::MaxConfigLights()) + "\n") +
		("#define MAX_LIGHT_UNIFORM_VECS " + IntToString(GL::LightHandler::MaxUniformVecs()) + "\n");

	if (useLuaShaders) {
		for (unsigned int n = GLSL_SHADER_STANDARD; n <= GLSL_SHADER_DEFERRED; n++) {
			glslShaders[n] = shaderHandler->CreateProgramObject("[SMFGroundDrawer::Lua]", names[n] + "-Lua");
			// release ID created by GLSLProgramObject's ctor; should be 0
			glslShaders[n]->Release();
		}
	} else {
		for (unsigned int n = GLSL_SHADER_STANDARD; n <= GLSL_SHADER_DEFERRED; n++) {
			// load from VFS files
			glslShaders[n] = shaderHandler->CreateProgramObject("[SMFGroundDrawer::VFS]", names[n]);
			glslShaders[n]->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFVertProg4.glsl", defs, GL_VERTEX_SHADER));
			glslShaders[n]->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFFragProg4.glsl", defs, GL_FRAGMENT_SHADER));
		}
	}

	glslShaders[GLSL_SHADER_CURRENT] = glslShaders[GLSL_SHADER_STANDARD];
	return true;
}

void SMFRenderStateGLSL::Kill() {
	if (useLuaShaders) {
		// make sure SH deletes only the wrapper objects; programs are managed by LuaShaders
		for (unsigned int n = GLSL_SHADER_STANDARD; n <= GLSL_SHADER_DEFERRED; n++) {
			if (glslShaders[n] != nullptr) {
				glslShaders[n]->LoadFromID(0);
			}
		}

		shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer::Lua]");
	} else {
		shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer::VFS]");
	}
}

void SMFRenderStateGLSL::Update(
	const CSMFGroundDrawer* smfGroundDrawer,
	const LuaMapShaderData* luaMapShaderData
) {
	if (useLuaShaders) {
		assert(luaMapShaderData != nullptr);

		// load from LuaShader ID; should be a linked and valid program (or 0)
		// NOTE: only non-custom shaders get to have engine flags and uniforms!
		for (unsigned int n = GLSL_SHADER_STANDARD; n <= GLSL_SHADER_DEFERRED; n++) {
			glslShaders[n]->LoadFromID(luaMapShaderData->shaderIDs[n]);
		}
	} else {
		assert(luaMapShaderData == nullptr);

		const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
		// const GL::LightHandler* lightHandler = smfGroundDrawer->GetLightHandler();

		const int2 normTexSize = smfMap->GetTextureSize(MAP_BASE_NORMALS_TEX);
		// const int2 specTexSize = smfMap->GetTextureSize(MAP_SSMF_SPECULAR_TEX);

		const float3 cameraPos = camera->GetPos();
		const float3 fogParams = {sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist()};

		for (unsigned int n = GLSL_SHADER_STANDARD; n <= GLSL_SHADER_DEFERRED; n++) {
			glslShaders[n]->SetFlag("SMF_VOID_WATER",                       mapRendering->voidWater);
			glslShaders[n]->SetFlag("SMF_VOID_GROUND",                      mapRendering->voidGround);
			glslShaders[n]->SetFlag("SMF_SPECULAR_LIGHTING",               (smfMap->GetSpecularTexture() != 0));
			glslShaders[n]->SetFlag("SMF_DETAIL_TEXTURE_SPLATTING",        (smfMap->GetSplatDistrTexture() != 0 && smfMap->GetSplatDetailTexture() != 0));
			glslShaders[n]->SetFlag("SMF_DETAIL_NORMAL_TEXTURE_SPLATTING", (smfMap->GetSplatDistrTexture() != 0 && smfMap->HaveSplatNormalTexture()));
			glslShaders[n]->SetFlag("SMF_DETAIL_NORMAL_DIFFUSE_ALPHA",      mapRendering->splatDetailNormalDiffuseAlpha);
			glslShaders[n]->SetFlag("SMF_WATER_ABSORPTION",                 smfMap->HasVisibleWater());
			glslShaders[n]->SetFlag("SMF_SKY_REFLECTIONS",                 (smfMap->GetSkyReflectModTexture() != 0));
			glslShaders[n]->SetFlag("SMF_BLEND_NORMALS",                   (smfMap->GetBlendNormalsTexture() != 0));
			glslShaders[n]->SetFlag("SMF_LIGHT_EMISSION",                  (smfMap->GetLightEmissionTexture() != 0));
			glslShaders[n]->SetFlag("SMF_PARALLAX_MAPPING",                (smfMap->GetParallaxHeightTexture() != 0));

			// both are runtime set in ::Enable, but ATI drivers need values from the beginning
			glslShaders[n]->SetFlag("HAVE_SHADOWS", false);
			glslShaders[n]->SetFlag("HAVE_INFOTEX", false);

			// used to strip down the shader for the deferred pass
			glslShaders[n]->SetFlag("DEFERRED_MODE", (n != GLSL_SHADER_STANDARD));
			glslShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
			glslShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
			glslShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
			glslShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
			glslShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
			glslShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

			glslShaders[n]->Link();
			glslShaders[n]->Enable();

			// tex1 (shadingTex) is not used by SMFFragProg
			glslShaders[n]->SetUniform("diffuseTex",             0);
			glslShaders[n]->SetUniform("detailTex",              2);
			glslShaders[n]->SetUniform("shadowTex",              4);
			glslShaders[n]->SetUniform("normalsTex",             5);
			glslShaders[n]->SetUniform("specularTex",            6);
			glslShaders[n]->SetUniform("splatDetailTex",         7);
			glslShaders[n]->SetUniform("splatDistrTex",          8);
			glslShaders[n]->SetUniform("skyReflectTex",          9);
			glslShaders[n]->SetUniform("skyReflectModTex",      10);
			glslShaders[n]->SetUniform("blendNormalsTex",       11);
			glslShaders[n]->SetUniform("lightEmissionTex",      12);
			glslShaders[n]->SetUniform("parallaxHeightTex",     13);
			glslShaders[n]->SetUniform("infoTex",               14);
			glslShaders[n]->SetUniform("splatDetailNormalTex1", 15);
			glslShaders[n]->SetUniform("splatDetailNormalTex2", 16);
			glslShaders[n]->SetUniform("splatDetailNormalTex3", 17);
			glslShaders[n]->SetUniform("splatDetailNormalTex4", 18);

			glslShaders[n]->SetUniform("mapSizePO2", mapDims.pwr2mapx * SQUARE_SIZE * 1.0f, mapDims.pwr2mapy * SQUARE_SIZE * 1.0f);
			glslShaders[n]->SetUniform("mapSize",    mapDims.mapx     * SQUARE_SIZE * 1.0f, mapDims.mapy     * SQUARE_SIZE * 1.0f);

			glslShaders[n]->SetUniform4v<float>("lightDir",  sky->GetLight()->GetLightDir());
			glslShaders[n]->SetUniform3v<float>("cameraPos", &cameraPos.x);

			glslShaders[n]->SetUniform3v<float>("groundAmbientColor",  &sunLighting->groundAmbientColor.x);
			glslShaders[n]->SetUniform3v<float>("groundDiffuseColor",  &sunLighting->groundDiffuseColor.x);
			glslShaders[n]->SetUniform3v<float>("groundSpecularColor", &sunLighting->groundSpecularColor.x);

			glslShaders[n]->SetUniform("groundSpecularExponent", sunLighting->specularExponent);
			glslShaders[n]->SetUniform("groundShadowDensity", sunLighting->groundShadowDensity);

			glslShaders[n]->SetUniform("gammaExponent", globalRendering->gammaExponent);

			glslShaders[n]->SetUniformMatrix4x4<float>("viewMat", false, camera->GetViewMatrix());
			glslShaders[n]->SetUniformMatrix4x4<float>("viewMatInv", false, camera->GetViewMatrixInverse());
			glslShaders[n]->SetUniformMatrix4x4<float>("viewProjMat", false, camera->GetViewProjectionMatrix());
			glslShaders[n]->SetUniformMatrix4x4<float>("shadowMat", false, shadowHandler.GetShadowViewMatrix());
			glslShaders[n]->SetUniform4v<float>("shadowParams", shadowHandler.GetShadowParams());

			glslShaders[n]->SetUniform4v<float>("alphaTestCtrl", float4{0.0f, 0.0f, 0.0f, 1.0f});

			// Enable always sets these
			// glslShaders[n]->SetUniform4v<float>("fwdDynLights", lightHandler->NumLightUniformVecs(), lightHandler->GetRawLightDataPtr());

			glslShaders[n]->SetUniform3v<float>("fogParams", &fogParams.x);
			glslShaders[n]->SetUniform4v<float>("fogColor", sky->fogColor);

			glslShaders[n]->SetUniform4v<float>("clipPlane", IWater::MapNullClipPlane());

			glslShaders[n]->SetUniform3v<float>("waterMinColor",    &waterRendering->minColor.x);
			glslShaders[n]->SetUniform3v<float>("waterBaseColor",   &waterRendering->baseColor.x);
			glslShaders[n]->SetUniform3v<float>("waterAbsorbColor", &waterRendering->absorb.x);

			glslShaders[n]->SetUniform4v<float>("splatTexScales", mapRendering->splatTexScales);
			glslShaders[n]->SetUniform4v<float>("splatTexMults", mapRendering->splatTexMults);

			glslShaders[n]->SetUniform("infoTexIntensityMul", 1.0f);

			glslShaders[n]->SetUniform(  "normalTexGen", 1.0f / ((normTexSize.x - 1) * SQUARE_SIZE), 1.0f / ((normTexSize.y - 1) * SQUARE_SIZE));
			glslShaders[n]->SetUniform("specularTexGen", 1.0f / (   mapDims.mapx     * SQUARE_SIZE), 1.0f / (   mapDims.mapy     * SQUARE_SIZE));
			glslShaders[n]->SetUniform(    "infoTexGen", 1.0f / (   mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (   mapDims.pwr2mapy * SQUARE_SIZE));

			glslShaders[n]->Disable();
			glslShaders[n]->Validate();
		}
	}
}

bool SMFRenderStateGLSL::HasValidShader(const DrawPass::e& drawPass) const {
	Shader::IProgramObject* shader = nullptr;

	switch (drawPass) {
		case DrawPass::TerrainDeferred: { shader = glslShaders[GLSL_SHADER_DEFERRED]; } break;
		default:                        { shader = glslShaders[GLSL_SHADER_CURRENT ]; } break;
	}

	return (shader != nullptr && shader->IsValid());
}



void SMFRenderStateGLSL::Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) {
	Shader::IProgramObject* shader = glslShaders[GLSL_SHADER_CURRENT];

	if (useLuaShaders) {
		// use raw, GLSLProgramObject::Enable also calls RecompileIfNeeded
		shader->EnableRaw();
		// Set the clip plane even for Lua shaders, is unknowable otherwise
		switch (drawPass) {
			case DrawPass::WaterReflection: { shader->SetUniform4v<float>("clipPlane", IWater::MapReflClipPlane()); } break;
			case DrawPass::WaterRefraction: { shader->SetUniform4v<float>("clipPlane", IWater::MapRefrClipPlane()); } break;
			default: {} break;
		}
		// diffuse textures are always bound (SMFGroundDrawer::SetupBigSquare)
		glActiveTexture(GL_TEXTURE0);
		return;
	}

	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();

	const GL::LightHandler* cLightHandler = smfGroundDrawer->GetLightHandler();
	      GL::LightHandler* mLightHandler = const_cast<GL::LightHandler*>(cLightHandler); // XXX

	const float3 cameraPos = camera->GetPos();
	const float3 fogParams = {sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist()};
	const float2 mapParams = {readMap->GetCurrMinHeight(), readMap->GetCurrMaxHeight()};

	shader->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	shader->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());

	shader->Enable();
	shader->SetUniform2v<float>("mapHeights", &mapParams.x);
	shader->SetUniform3v<float>("cameraPos", &cameraPos.x);
	shader->SetUniformMatrix4x4<float>("viewMat", false, camera->GetViewMatrix());
	shader->SetUniformMatrix4x4<float>("viewMatInv", false, camera->GetViewMatrixInverse());
	shader->SetUniformMatrix4x4<float>("viewProjMat", false, camera->GetViewProjectionMatrix());
	shader->SetUniformMatrix4x4<float>("shadowMat", false, shadowHandler.GetShadowViewMatrix());
	shader->SetUniform4v<float>("shadowParams", shadowHandler.GetShadowParams());
	shader->SetUniform3v<float>("fogParams", &fogParams.x);
	shader->SetUniform<float>("infoTexIntensityMul", float(infoTextureHandler->InMetalMode()) + 1.0f);
	shader->SetUniform<float>("gammaExponent", globalRendering->gammaExponent);

	if (cLightHandler->NumConfigLights() > 0) {
		mLightHandler->Update();
		shader->SetUniform4v<float>("fwdDynLights", cLightHandler->NumUniformVecs(), cLightHandler->GetRawLightDataPtr());
	}

	switch (drawPass) {
		case DrawPass::WaterReflection: { shader->SetUniform4v<float>("clipPlane", IWater::MapReflClipPlane()); } break;
		case DrawPass::WaterRefraction: { shader->SetUniform4v<float>("clipPlane", IWater::MapRefrClipPlane()); } break;
		default: {} break;
	}

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE4);

	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, smfMap->GetSpecularTexture());
	glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDetailTexture());
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDistrTexture());
	glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetSkyReflectionTextureID());
	glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, smfMap->GetSkyReflectModTexture());
	glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, smfMap->GetBlendNormalsTexture());
	glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, smfMap->GetLightEmissionTexture());
	glActiveTexture(GL_TEXTURE13); glBindTexture(GL_TEXTURE_2D, smfMap->GetParallaxHeightTexture());
	glActiveTexture(GL_TEXTURE14); glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());

	for (int i = 0; i < CSMFReadMap::NUM_SPLAT_DETAIL_NORMALS; i++) {
		if (smfMap->GetSplatNormalTexture(i) != 0) {
			glActiveTexture(GL_TEXTURE15 + i); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatNormalTexture(i));
		}
	}

	glActiveTexture(GL_TEXTURE0);
}

void SMFRenderStateGLSL::Disable(const CSMFGroundDrawer*, const DrawPass::e&) {
	if (useLuaShaders) {
		glActiveTexture(GL_TEXTURE0);
		glslShaders[GLSL_SHADER_CURRENT]->DisableRaw();
		return;
	}

	if (shadowHandler.ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}

	glActiveTexture(GL_TEXTURE0);
	glslShaders[GLSL_SHADER_CURRENT]->Disable();
}






void SMFRenderStateGLSL::SetSquareTexGen(const int sqx, const int sqy, const int nsx, const int mip) const {
	// needs to be set even for Lua shaders, is unknowable otherwise
	// (works because SMFGroundDrawer::SetupBigSquare always calls us)
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform("texSquare", sqx, sqy, sqy * nsx + sqx, mip);
}



void SMFRenderStateGLSL::SetCurrentShader(const DrawPass::e& drawPass) {
	switch (drawPass) {
		case DrawPass::TerrainDeferred: { glslShaders[GLSL_SHADER_CURRENT] = glslShaders[GLSL_SHADER_DEFERRED]; } break;
		default:                        { glslShaders[GLSL_SHADER_CURRENT] = glslShaders[GLSL_SHADER_STANDARD]; } break;
	}
}



void SMFRenderStateGLSL::SetSkyLight(const ISkyLight* skyLight) const {
	// called during the SunChanged event, shader is not bound
	glslShaders[GLSL_SHADER_CURRENT]->Enable();
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform4v("lightDir", &skyLight->GetLightDir().x);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform("groundShadowDensity", sunLighting->groundShadowDensity);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform3v("groundAmbientColor",  &sunLighting->groundAmbientColor[0]);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform3v("groundDiffuseColor",  &sunLighting->groundDiffuseColor[0]);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform3v("groundSpecularColor", &sunLighting->groundSpecularColor[0]);
	glslShaders[GLSL_SHADER_CURRENT]->Disable();
}

void SMFRenderStateGLSL::SetAlphaTest(const float4& params) const {
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform4v<float>("alphaTestCtrl", params);
}

