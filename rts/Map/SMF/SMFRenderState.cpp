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
#include "Rendering/Env/SkyLight.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/Util.h"

#define SMF_TEXSQUARE_SIZE 1024.0f



ISMFRenderState* ISMFRenderState::GetInstance(bool haveARB, bool haveGLSL) {
	ISMFRenderState* instance = NULL;

	if (!haveARB && !haveGLSL) {
		instance = new SMFRenderStateFFP();
	} else {
		if (!haveGLSL) {
			instance = new SMFRenderStateARB();
		} else {
			instance = new SMFRenderStateGLSL();
		}
	}

	return instance;
}



bool SMFRenderStateARB::Init(const CSMFGroundDrawer* smfGroundDrawer) {
	memset(&arbShaders[0], 0, sizeof(arbShaders));

	if (!globalRendering->haveARB) {
		// not possible to do (ARB) shader-based map rendering
		return false;
	}
	if (!configHandler->GetBool("AdvMapShading")) {
		// not allowed to do (ARB) shader-based map rendering
		return false;
	}

	#define sh shaderHandler
	arbShaders[ARB_SHADER_DEFAULT] = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderBaseARB", true);
	arbShaders[ARB_SHADER_REFLECT] = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderReflARB", true);
	arbShaders[ARB_SHADER_REFRACT] = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderRefrARB", true);
	arbShaders[ARB_SHADER_CURRENT] = arbShaders[ARB_SHADER_DEFAULT];

	arbShaders[ARB_SHADER_DEFAULT]->AttachShaderObject(sh->CreateShaderObject("ARB/ground.vp", "", GL_VERTEX_PROGRAM_ARB));
	arbShaders[ARB_SHADER_DEFAULT]->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	arbShaders[ARB_SHADER_DEFAULT]->Link();

	arbShaders[ARB_SHADER_REFLECT]->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundreflectinverted.vp", "", GL_VERTEX_PROGRAM_ARB));
	arbShaders[ARB_SHADER_REFLECT]->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	arbShaders[ARB_SHADER_REFLECT]->Link();

	arbShaders[ARB_SHADER_REFRACT]->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundrefract.vp", "", GL_VERTEX_PROGRAM_ARB));
	arbShaders[ARB_SHADER_REFRACT]->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	arbShaders[ARB_SHADER_REFRACT]->Link();
	#undef sh

	return true;
}

void SMFRenderStateARB::Kill() {
	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");
}



bool SMFRenderStateGLSL::Init(const CSMFGroundDrawer* smfGroundDrawer) {
	memset(&glslShaders[0], 0, sizeof(glslShaders));

	if (!globalRendering->haveGLSL) {
		// not possible to do (GLSL) shader-based map rendering
		return false;
	}
	if (!configHandler->GetBool("AdvMapShading")) {
		// not allowed to do (GLSL) shader-based map rendering
		return false;
	}

	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
	const GL::LightHandler* lightHandler = smfGroundDrawer->GetLightHandler();

	const std::string names[GLSL_SHADER_COUNT] = {
		"SMFShaderGLSL-Standard",
		"SMFShaderGLSL-Deferred",
		""
	};
	const std::string defs =
		("#define SMF_TEXSQUARE_SIZE " + FloatToString(                  SMF_TEXSQUARE_SIZE) + "\n") +
		("#define SMF_INTENSITY_MULT " + FloatToString(CGlobalRendering::SMF_INTENSITY_MULT) + "\n");

	#define sh shaderHandler
	for (unsigned int n = GLSL_SHADER_STANDARD; n <= GLSL_SHADER_DEFERRED; n++) {
		glslShaders[n] = sh->CreateProgramObject("[SMFGroundDrawer]", names[n], false);
		glslShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFVertProg.glsl", defs, GL_VERTEX_SHADER));
		glslShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFFragProg.glsl", defs, GL_FRAGMENT_SHADER));

		glslShaders[n]->SetFlag("SMF_VOID_WATER",               int(mapInfo->map.voidWater));
		glslShaders[n]->SetFlag("SMF_VOID_GROUND",              int(mapInfo->map.voidGround));
		glslShaders[n]->SetFlag("SMF_ARB_LIGHTING",             int(!smfMap->HaveSpecularTexture()));
		glslShaders[n]->SetFlag("SMF_DETAIL_TEXTURE_SPLATTING", int(smfMap->HaveSplatTexture()));
		glslShaders[n]->SetFlag("SMF_WATER_ABSORPTION",         int(smfMap->HasVisibleWater()));
		glslShaders[n]->SetFlag("SMF_SKY_REFLECTIONS",          int(smfMap->GetSkyReflectModTexture() != 0));
		glslShaders[n]->SetFlag("SMF_DETAIL_NORMALS",           int(smfMap->GetDetailNormalTexture() != 0));
		glslShaders[n]->SetFlag("SMF_LIGHT_EMISSION",           int(smfMap->GetLightEmissionTexture() != 0));
		glslShaders[n]->SetFlag("SMF_PARALLAX_MAPPING",         int(smfMap->GetParallaxHeightTexture() != 0));

		glslShaders[n]->SetFlag("BASE_DYNAMIC_MAP_LIGHT",       lightHandler->GetBaseLight());
		glslShaders[n]->SetFlag("MAX_DYNAMIC_MAP_LIGHTS",       lightHandler->GetMaxLights());

		// both are runtime set, but ATI drivers need them set from the beginning
		glslShaders[n]->SetFlag("HAVE_SHADOWS", 0);
		glslShaders[n]->SetFlag("HAVE_INFOTEX", 0);
		// used to strip down the shader for the deferred pass
		glslShaders[n]->SetFlag("DEFERRED_MODE", int(n != GLSL_SHADER_STANDARD));

		glslShaders[n]->Link();
		glslShaders[n]->SetUniformLocation("diffuseTex");          // idx  0
		glslShaders[n]->SetUniformLocation("normalsTex");          // idx  1
		glslShaders[n]->SetUniformLocation("shadowTex");           // idx  2
		glslShaders[n]->SetUniformLocation("detailTex");           // idx  3
		glslShaders[n]->SetUniformLocation("specularTex");         // idx  4
		glslShaders[n]->SetUniformLocation("infoTex");             // idx  5
		glslShaders[n]->SetUniformLocation("mapSizePO2");          // idx  6
		glslShaders[n]->SetUniformLocation("mapSize");             // idx  7
		glslShaders[n]->SetUniformLocation("texSquare");           // idx  8
		glslShaders[n]->SetUniformLocation("mapHeights");          // idx  9
		glslShaders[n]->SetUniformLocation("lightDir");            // idx 10
		glslShaders[n]->SetUniformLocation("cameraPos");           // idx 11
		glslShaders[n]->SetUniformLocation("$UNUSED$");            // idx 12
		glslShaders[n]->SetUniformLocation("shadowMat");           // idx 13
		glslShaders[n]->SetUniformLocation("shadowParams");        // idx 14
		glslShaders[n]->SetUniformLocation("groundAmbientColor");  // idx 15
		glslShaders[n]->SetUniformLocation("groundDiffuseColor");  // idx 16
		glslShaders[n]->SetUniformLocation("groundSpecularColor"); // idx 17
		glslShaders[n]->SetUniformLocation("groundShadowDensity"); // idx 18
		glslShaders[n]->SetUniformLocation("waterMinColor");       // idx 19
		glslShaders[n]->SetUniformLocation("waterBaseColor");      // idx 20
		glslShaders[n]->SetUniformLocation("waterAbsorbColor");    // idx 21
		glslShaders[n]->SetUniformLocation("splatDetailTex");      // idx 22
		glslShaders[n]->SetUniformLocation("splatDistrTex");       // idx 23
		glslShaders[n]->SetUniformLocation("splatTexScales");      // idx 24
		glslShaders[n]->SetUniformLocation("splatTexMults");       // idx 25
		glslShaders[n]->SetUniformLocation("skyReflectTex");       // idx 26
		glslShaders[n]->SetUniformLocation("skyReflectModTex");    // idx 27
		glslShaders[n]->SetUniformLocation("detailNormalTex");     // idx 28
		glslShaders[n]->SetUniformLocation("lightEmissionTex");    // idx 29
		glslShaders[n]->SetUniformLocation("parallaxHeightTex");   // idx 30
		glslShaders[n]->SetUniformLocation("infoTexIntensityMul"); // idx 31
		glslShaders[n]->SetUniformLocation("normalTexGen");        // idx 32
		glslShaders[n]->SetUniformLocation("specularTexGen");      // idx 33
		glslShaders[n]->SetUniformLocation("infoTexGen");          // idx 34

		glslShaders[n]->Enable();
		glslShaders[n]->SetUniform1i(0,  0); // diffuseTex  (idx 0, texunit  0)
		glslShaders[n]->SetUniform1i(1,  5); // normalsTex  (idx 1, texunit  5)
		glslShaders[n]->SetUniform1i(2,  4); // shadowTex   (idx 2, texunit  4)
		glslShaders[n]->SetUniform1i(3,  2); // detailTex   (idx 3, texunit  2)
		glslShaders[n]->SetUniform1i(4,  6); // specularTex (idx 4, texunit  6)
		glslShaders[n]->SetUniform1i(5, 14); // infoTex     (idx 5, texunit 14)
		glslShaders[n]->SetUniform2f(6, (gs->pwr2mapx * SQUARE_SIZE), (gs->pwr2mapy * SQUARE_SIZE));
		glslShaders[n]->SetUniform2f(7, (gs->mapx * SQUARE_SIZE), (gs->mapy * SQUARE_SIZE));
		glslShaders[n]->SetUniform4fv(10, &((sky->GetLight())->GetLightDir()).x);
		glslShaders[n]->SetUniform3fv(15, &mapInfo->light.groundAmbientColor[0]);
		glslShaders[n]->SetUniform3fv(16, &mapInfo->light.groundSunColor[0]);
		glslShaders[n]->SetUniform3fv(17, &mapInfo->light.groundSpecularColor[0]);
		glslShaders[n]->SetUniform1f(18, sky->GetLight()->GetGroundShadowDensity());
		glslShaders[n]->SetUniform3fv(19, &mapInfo->water.minColor[0]);
		glslShaders[n]->SetUniform3fv(20, &mapInfo->water.baseColor[0]);
		glslShaders[n]->SetUniform3fv(21, &mapInfo->water.absorb[0]);
		glslShaders[n]->SetUniform1i(22, 7); // splatDetailTex (idx 22, texunit 7)
		glslShaders[n]->SetUniform1i(23, 8); // splatDistrTex (idx 23, texunit 8)
		glslShaders[n]->SetUniform4fv(24, &mapInfo->splats.texScales[0]);
		glslShaders[n]->SetUniform4fv(25, &mapInfo->splats.texMults[0]);
		glslShaders[n]->SetUniform1i(26,  9); // skyReflectTex (idx 26, texunit 9)
		glslShaders[n]->SetUniform1i(27, 10); // skyReflectModTex (idx 27, texunit 10)
		glslShaders[n]->SetUniform1i(28, 11); // detailNormalTex (idx 28, texunit 11)
		glslShaders[n]->SetUniform1i(29, 12); // lightEmisionTex (idx 29, texunit 12)
		glslShaders[n]->SetUniform1i(30, 13); // parallaxHeightTex (idx 30, texunit 13)
		glslShaders[n]->SetUniform1f(31, 1.0f); // infoTexIntensityMul
		glslShaders[n]->SetUniform2f(32, 1.0f / ((smfMap->normalTexSize.x - 1) * SQUARE_SIZE), 1.0f / ((smfMap->normalTexSize.y - 1) * SQUARE_SIZE));
		glslShaders[n]->SetUniform2f(33, 1.0f / (gs->mapx * SQUARE_SIZE), 1.0f / (gs->mapy * SQUARE_SIZE));
		glslShaders[n]->SetUniform2f(34, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE));
		glslShaders[n]->Disable();
		glslShaders[n]->Validate();
	}

	#undef sh

	glslShaders[GLSL_SHADER_CURRENT] = glslShaders[GLSL_SHADER_STANDARD];
	return true;
}

void SMFRenderStateGLSL::Kill() {
	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");
}



bool SMFRenderStateFFP::CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const {
	return (!smfGroundDrawer->advShading || smfGroundDrawer->DrawExtraTex());
}

bool SMFRenderStateARB::CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const {
	// NOTE:
	//     the ARB shaders assume shadows are always on, so
	//     SMFRenderStateARB can be used only when they are
	//     in fact enabled (see Init)
	return (smfGroundDrawer->advShading && !smfGroundDrawer->DrawExtraTex() && shadowHandler->shadowsLoaded);
}

bool SMFRenderStateGLSL::CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const {
	return (smfGroundDrawer->advShading);
}



void SMFRenderStateFFP::Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e&) {
	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
	static const GLfloat planeX[] = {0.02f, 0.0f, 0.00f, 0.0f};
	static const GLfloat planeZ[] = {0.00f, 0.0f, 0.02f, 0.0f};

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	if (smfGroundDrawer->DrawExtraTex()) {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, smfGroundDrawer->infoTex);
		glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f, 1.0f, 1.0f, 1.0f); // fix nvidia bug with gltexgen
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);


		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); // fix nvidia bug with gltexgen

		if (smfGroundDrawer->GetDrawMode() == CBaseGroundDrawer::drawMetal) {
			// increase brightness for metal spots
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		}

		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);


		glActiveTexture(GL_TEXTURE3);
		if (smfMap->GetDetailTexture() != 0) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
			glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f, 1.0f, 1.0f, 1.0f); // fix nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glDisable(GL_TEXTURE_2D);
		}
	} else {
		if (smfMap->GetDetailTexture()) {
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
			glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f, 1.0f, 1.0f, 1.0f); // fix nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_2D);
		}


		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); // fix nvidia bug with gltexgen
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), -0.5f / gs->pwr2mapx, -0.5f / gs->pwr2mapy);


		// bind the detail texture a 2nd time to increase the details (-> GL_ADD_SIGNED_ARB is limited -0.5 to +0.5)
		// (also do this after the shading texture cause of color clamping issues)
		if (smfMap->GetDetailTexture()) {
			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
			glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f, 1.0f, 1.0f, 1.0f); // fix nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glActiveTexture(GL_TEXTURE3);
			glDisable(GL_TEXTURE_2D);
		}
	}

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
}

void SMFRenderStateFFP::Disable(const CSMFGroundDrawer*, const DrawPass::e&) {
	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_2D);

}



void SMFRenderStateARB::Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) {
	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
	const float3 ambientColor = mapInfo->light.groundAmbientColor * CGlobalRendering::SMF_INTENSITY_MULT;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
	// CDynamicWater overrides smfShaderBaseARB during the reflection / refraction
	// pass to distort underwater geometry, but because it's hard to maintain only
	// a vertex shader when working with texture combiners we don't enable this
	// note: we would also want to disable culling for these passes
	if (arbShaders[ARB_SHADER_CURRENT] != arbShaders[ARB_SHADER_DEFAULT]) {
		if (drawPass == DrawPass::WaterReflection) {
			glAlphaFunc(GL_GREATER, 0.9f);
			glEnable(GL_ALPHA_TEST);
		}
	}
	#endif

	arbShaders[ARB_SHADER_CURRENT]->Enable();
	arbShaders[ARB_SHADER_CURRENT]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(12, 1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, 0, 1);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(13, -math::floor(camera->GetPos().x * 0.02f), -math::floor(camera->GetPos().z * 0.02f), 0, 0);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(14, 0.02f, 0.02f, 0, 1);
	arbShaders[ARB_SHADER_CURRENT]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(11, 0, 0, 0, sky->GetLight()->GetGroundShadowDensity());

	glMatrixMode(GL_MATRIX0_ARB);
	glLoadMatrixf(shadowHandler->shadowMatrix);
	glMatrixMode(GL_MODELVIEW);

	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

	glActiveTexture(GL_TEXTURE0);
}

void SMFRenderStateARB::Disable(const CSMFGroundDrawer*, const DrawPass::e& drawPass) {
	arbShaders[ARB_SHADER_CURRENT]->Disable();

	#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
	if (arbShaders[ARB_SHADER_CURRENT] != arbShaders[ARB_SHADER_DEFAULT]) {
		if (drawPass == DrawPass::WaterReflection) {
			glDisable(GL_ALPHA_TEST);
		}
	}
	#endif

	glActiveTexture(GL_TEXTURE4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
}



void SMFRenderStateGLSL::Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e&) {
	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
	const GL::LightHandler* lightHandler = smfGroundDrawer->GetLightHandler();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glslShaders[GLSL_SHADER_CURRENT]->SetFlag("HAVE_SHADOWS", int(shadowHandler->shadowsLoaded));
	glslShaders[GLSL_SHADER_CURRENT]->SetFlag("HAVE_INFOTEX", int(smfGroundDrawer->DrawExtraTex()));

	glslShaders[GLSL_SHADER_CURRENT]->Enable();
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform2f(9, readMap->GetCurrMinHeight(), readMap->GetCurrMaxHeight());
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform3fv(11, &camera->GetPos()[0]);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniformMatrix4fv(13, false, shadowHandler->shadowMatrix);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform4fv(14, &(shadowHandler->GetShadowParams().x));
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform1f(31, float(smfGroundDrawer->GetDrawMode() == CBaseGroundDrawer::drawMetal) + 1.0f);

	// already on the MV stack at this point
	glLoadIdentity();
	const_cast<GL::LightHandler*>(lightHandler)->Update(glslShaders[GLSL_SHADER_CURRENT]);
	glMultMatrixf(camera->GetViewMatrix());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());

	// setup for shadow2DProj
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, smfMap->GetSpecularTexture());
	glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDetailTexture());
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDistrTexture());
	glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSkyReflectionTextureID());
	glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, smfMap->GetSkyReflectModTexture());
	glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailNormalTexture());
	glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, smfMap->GetLightEmissionTexture());
	glActiveTexture(GL_TEXTURE13); glBindTexture(GL_TEXTURE_2D, smfMap->GetParallaxHeightTexture());
	glActiveTexture(GL_TEXTURE14); glBindTexture(GL_TEXTURE_2D, smfGroundDrawer->infoTex);

	glActiveTexture(GL_TEXTURE0);
}

void SMFRenderStateGLSL::Disable(const CSMFGroundDrawer*, const DrawPass::e&) {
	glslShaders[GLSL_SHADER_CURRENT]->Disable();

	glActiveTexture(GL_TEXTURE14); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE13); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE4);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
}






void SMFRenderStateFFP::SetSquareTexGen(const int sqx, const int sqy) const {
	// const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
	// const float smfTexSquareSizeInv = 1.0f / smfMap->bigTexSize;

	SetTexGen(1.0f / SMF_TEXSQUARE_SIZE, 1.0f / SMF_TEXSQUARE_SIZE, -sqx, -sqy);
}

void SMFRenderStateARB::SetSquareTexGen(const int sqx, const int sqy) const {
	arbShaders[ARB_SHADER_CURRENT]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(11, -sqx, -sqy, 0, 0);
}

void SMFRenderStateGLSL::SetSquareTexGen(const int sqx, const int sqy) const {
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform2i(8, sqx, sqy);
}



void SMFRenderStateARB::SetCurrentShader(const DrawPass::e& drawPass) {
	switch (drawPass) {
		case DrawPass::Normal:          { arbShaders[ARB_SHADER_CURRENT] = arbShaders[ARB_SHADER_DEFAULT]; } break;
		case DrawPass::WaterReflection: { arbShaders[ARB_SHADER_CURRENT] = arbShaders[ARB_SHADER_REFLECT]; } break;
		case DrawPass::WaterRefraction: { arbShaders[ARB_SHADER_CURRENT] = arbShaders[ARB_SHADER_REFRACT]; } break;
		default:                        {                                                                  } break;
	}
}

void SMFRenderStateGLSL::SetCurrentShader(const DrawPass::e& drawPass) {
	switch (drawPass) {
		case DrawPass::TerrainDeferred: { glslShaders[GLSL_SHADER_CURRENT] = glslShaders[GLSL_SHADER_DEFERRED]; } break;
		default:                        { glslShaders[GLSL_SHADER_CURRENT] = glslShaders[GLSL_SHADER_STANDARD]; } break;
	}
}



void SMFRenderStateARB::UpdateCurrentShader(const ISkyLight* skyLight) const {
	arbShaders[ARB_SHADER_CURRENT]->Enable();
	arbShaders[ARB_SHADER_CURRENT]->SetUniform4f(11, 0, 0, 0, skyLight->GetGroundShadowDensity());
	arbShaders[ARB_SHADER_CURRENT]->Disable();
}

void SMFRenderStateGLSL::UpdateCurrentShader(const ISkyLight* skyLight) const {
	glslShaders[GLSL_SHADER_CURRENT]->Enable();
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform4fv(10, &skyLight->GetLightDir().x);
	glslShaders[GLSL_SHADER_CURRENT]->SetUniform1f(18, skyLight->GetGroundShadowDensity());
	glslShaders[GLSL_SHADER_CURRENT]->Disable();
}

