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
#include "Rendering/GL/LightHandler.h"
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
	smfShaderBaseARB = NULL;
	smfShaderReflARB = NULL;
	smfShaderRefrARB = NULL;
	smfShaderCurrARB = NULL;

	if (!configHandler->GetBool("AdvMapShading")) {
		// not allowed to do shader-based map rendering
		return false;
	}

	#define sh shaderHandler
	smfShaderBaseARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderBaseARB", true);
	smfShaderReflARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderReflARB", true);
	smfShaderRefrARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderRefrARB", true);
	smfShaderCurrARB = smfShaderBaseARB;

	// NOTE:
	//     the ARB shaders assume shadows are always on, so
	//     SMFRenderStateARB can be used only when they are
	//     in fact enabled
	smfShaderBaseARB->AttachShaderObject(sh->CreateShaderObject("ARB/ground.vp", "", GL_VERTEX_PROGRAM_ARB));
	smfShaderBaseARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	smfShaderBaseARB->Link();

	smfShaderReflARB->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundreflectinverted.vp", "", GL_VERTEX_PROGRAM_ARB));
	smfShaderReflARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	smfShaderReflARB->Link();

	smfShaderRefrARB->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundrefract.vp", "", GL_VERTEX_PROGRAM_ARB));
	smfShaderRefrARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
	smfShaderRefrARB->Link();
	#undef sh

	return true;
}

void SMFRenderStateARB::Kill() {
	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");
}



bool SMFRenderStateGLSL::Init(const CSMFGroundDrawer* smfGroundDrawer) {
	smfShaderGLSL = NULL;

	if (!configHandler->GetBool("AdvMapShading")) {
		// not allowed to do shader-based map rendering
		return false;
	}

	const CSMFReadMap* smfMap = smfGroundDrawer->GetReadMap();
	const GL::LightHandler* lightHandler = smfGroundDrawer->GetLightHandler();

	#define sh shaderHandler
	std::ostringstream defBuf;
	defBuf
		<< "#define SMF_TEXSQUARE_SIZE " << float(SMF_TEXSQUARE_SIZE) << std::endl
		<< "#define SMF_INTENSITY_MULT " << float(CGlobalRendering::SMF_INTENSITY_MULT);
	smfShaderGLSL = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderGLSL", false);
	smfShaderGLSL->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFVertProg.glsl", defBuf.str(), GL_VERTEX_SHADER));
	smfShaderGLSL->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFFragProg.glsl", defBuf.str(), GL_FRAGMENT_SHADER));

	smfShaderGLSL->SetFlag("SMF_WATER_ABSORPTION",         (smfMap->initMinHeight <= 0.0f) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_VOID_WATER",               (mapInfo->map.voidWater) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_VOID_GROUND",              (mapInfo->map.voidGround) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_ARB_LIGHTING",             (smfMap->HaveSpecularTexture()) ? 0 : 1);
	smfShaderGLSL->SetFlag("SMF_DETAIL_TEXTURE_SPLATTING", (smfMap->HaveSplatTexture()) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_WATER_ABSORPTION",         (!(smfMap->initMinHeight > 0.0f || mapInfo->map.voidWater)) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_SKY_REFLECTIONS",          (smfMap->GetSkyReflectModTexture() != 0) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_DETAIL_NORMALS",           (smfMap->GetDetailNormalTexture() != 0) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_LIGHT_EMISSION",           (smfMap->GetLightEmissionTexture() != 0) ? 1 : 0);
	smfShaderGLSL->SetFlag("SMF_PARALLAX_MAPPING",         (smfMap->GetParallaxHeightTexture() != 0) ? 1 : 0);

	smfShaderGLSL->SetFlag("BASE_DYNAMIC_MAP_LIGHT",       lightHandler->GetBaseLight());
	smfShaderGLSL->SetFlag("MAX_DYNAMIC_MAP_LIGHTS",       lightHandler->GetMaxLights());

	// both are runtime set, but ATI drivers need them set from the beginning
	smfShaderGLSL->SetFlag("HAVE_SHADOWS", 0);
	smfShaderGLSL->SetFlag("HAVE_INFOTEX", 0);

	smfShaderGLSL->Link();
	smfShaderGLSL->SetUniformLocation("diffuseTex");          // idx  0
	smfShaderGLSL->SetUniformLocation("normalsTex");          // idx  1
	smfShaderGLSL->SetUniformLocation("shadowTex");           // idx  2
	smfShaderGLSL->SetUniformLocation("detailTex");           // idx  3
	smfShaderGLSL->SetUniformLocation("specularTex");         // idx  4
	smfShaderGLSL->SetUniformLocation("infoTex");             // idx  5
	smfShaderGLSL->SetUniformLocation("mapSizePO2");          // idx  6
	smfShaderGLSL->SetUniformLocation("mapSize");             // idx  7
	smfShaderGLSL->SetUniformLocation("texSquare");           // idx  8
	smfShaderGLSL->SetUniformLocation("mapHeights");          // idx  9
	smfShaderGLSL->SetUniformLocation("lightDir");            // idx 10
	smfShaderGLSL->SetUniformLocation("cameraPos");           // idx 11
	smfShaderGLSL->SetUniformLocation("$UNUSED$");            // idx 12
	smfShaderGLSL->SetUniformLocation("shadowMat");           // idx 13
	smfShaderGLSL->SetUniformLocation("shadowParams");        // idx 14
	smfShaderGLSL->SetUniformLocation("groundAmbientColor");  // idx 15
	smfShaderGLSL->SetUniformLocation("groundDiffuseColor");  // idx 16
	smfShaderGLSL->SetUniformLocation("groundSpecularColor"); // idx 17
	smfShaderGLSL->SetUniformLocation("groundShadowDensity"); // idx 18
	smfShaderGLSL->SetUniformLocation("waterMinColor");       // idx 19
	smfShaderGLSL->SetUniformLocation("waterBaseColor");      // idx 20
	smfShaderGLSL->SetUniformLocation("waterAbsorbColor");    // idx 21
	smfShaderGLSL->SetUniformLocation("splatDetailTex");      // idx 22
	smfShaderGLSL->SetUniformLocation("splatDistrTex");       // idx 23
	smfShaderGLSL->SetUniformLocation("splatTexScales");      // idx 24
	smfShaderGLSL->SetUniformLocation("splatTexMults");       // idx 25
	smfShaderGLSL->SetUniformLocation("skyReflectTex");       // idx 26
	smfShaderGLSL->SetUniformLocation("skyReflectModTex");    // idx 27
	smfShaderGLSL->SetUniformLocation("detailNormalTex");     // idx 28
	smfShaderGLSL->SetUniformLocation("lightEmissionTex");    // idx 29
	smfShaderGLSL->SetUniformLocation("parallaxHeightTex");   // idx 30
	smfShaderGLSL->SetUniformLocation("infoTexIntensityMul"); // idx 31
	smfShaderGLSL->SetUniformLocation("normalTexGen");        // idx 32
	smfShaderGLSL->SetUniformLocation("specularTexGen");      // idx 33
	smfShaderGLSL->SetUniformLocation("infoTexGen");          // idx 34

	smfShaderGLSL->Enable();
	smfShaderGLSL->SetUniform1i(0,  0); // diffuseTex  (idx 0, texunit  0)
	smfShaderGLSL->SetUniform1i(1,  5); // normalsTex  (idx 1, texunit  5)
	smfShaderGLSL->SetUniform1i(2,  4); // shadowTex   (idx 2, texunit  4)
	smfShaderGLSL->SetUniform1i(3,  2); // detailTex   (idx 3, texunit  2)
	smfShaderGLSL->SetUniform1i(4,  6); // specularTex (idx 4, texunit  6)
	smfShaderGLSL->SetUniform1i(5, 14); // infoTex     (idx 5, texunit 14)
	smfShaderGLSL->SetUniform2f(6, (gs->pwr2mapx * SQUARE_SIZE), (gs->pwr2mapy * SQUARE_SIZE));
	smfShaderGLSL->SetUniform2f(7, (gs->mapx * SQUARE_SIZE), (gs->mapy * SQUARE_SIZE));
	smfShaderGLSL->SetUniform4fv(10, &sky->GetLight()->GetLightDir().x);
	smfShaderGLSL->SetUniform3fv(15, &mapInfo->light.groundAmbientColor[0]);
	smfShaderGLSL->SetUniform3fv(16, &mapInfo->light.groundSunColor[0]);
	smfShaderGLSL->SetUniform3fv(17, &mapInfo->light.groundSpecularColor[0]);
	smfShaderGLSL->SetUniform1f(18, sky->GetLight()->GetGroundShadowDensity());
	smfShaderGLSL->SetUniform3fv(19, &mapInfo->water.minColor[0]);
	smfShaderGLSL->SetUniform3fv(20, &mapInfo->water.baseColor[0]);
	smfShaderGLSL->SetUniform3fv(21, &mapInfo->water.absorb[0]);
	smfShaderGLSL->SetUniform1i(22, 7); // splatDetailTex (idx 22, texunit 7)
	smfShaderGLSL->SetUniform1i(23, 8); // splatDistrTex (idx 23, texunit 8)
	smfShaderGLSL->SetUniform4fv(24, &mapInfo->splats.texScales[0]);
	smfShaderGLSL->SetUniform4fv(25, &mapInfo->splats.texMults[0]);
	smfShaderGLSL->SetUniform1i(26,  9); // skyReflectTex (idx 26, texunit 9)
	smfShaderGLSL->SetUniform1i(27, 10); // skyReflectModTex (idx 27, texunit 10)
	smfShaderGLSL->SetUniform1i(28, 11); // detailNormalTex (idx 28, texunit 11)
	smfShaderGLSL->SetUniform1i(29, 12); // lightEmisionTex (idx 29, texunit 12)
	smfShaderGLSL->SetUniform1i(30, 13); // parallaxHeightTex (idx 30, texunit 13)
	smfShaderGLSL->SetUniform1f(31, 1.0f); // infoTexIntensityMul
	smfShaderGLSL->SetUniform2f(32, 1.0f / ((smfMap->normalTexSize.x - 1) * SQUARE_SIZE), 1.0f / ((smfMap->normalTexSize.y - 1) * SQUARE_SIZE));
	smfShaderGLSL->SetUniform2f(33, 1.0f / (gs->mapx * SQUARE_SIZE), 1.0f / (gs->mapy * SQUARE_SIZE));
	smfShaderGLSL->SetUniform2f(34, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE));
	smfShaderGLSL->Disable();
	smfShaderGLSL->Validate();
	#undef sh

	return true;
}

void SMFRenderStateGLSL::Kill() {
	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");
}



bool SMFRenderStateFFP::CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const {
	return (!smfGroundDrawer->advShading || smfGroundDrawer->DrawExtraTex());
}

bool SMFRenderStateARB::CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const {
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

		if (smfGroundDrawer->drawMode == CBaseGroundDrawer::drawMetal) {
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
	if (smfShaderCurrARB != smfShaderBaseARB) {
		if (drawPass == DrawPass::WaterReflection) {
			glAlphaFunc(GL_GREATER, 0.9f);
			glEnable(GL_ALPHA_TEST);
		}
	}
	#endif

	smfShaderCurrARB->Enable();
	smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
	smfShaderCurrARB->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
	smfShaderCurrARB->SetUniform4f(12, 1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, 0, 1);
	smfShaderCurrARB->SetUniform4f(13, -math::floor(camera->pos.x * 0.02f), -math::floor(camera->pos.z * 0.02f), 0, 0);
	smfShaderCurrARB->SetUniform4f(14, 0.02f, 0.02f, 0, 1);
	smfShaderCurrARB->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
	smfShaderCurrARB->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1);
	smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, sky->GetLight()->GetGroundShadowDensity());

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
	smfShaderCurrARB->Disable();

	#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
	if (smfShaderCurrARB != smfShaderBaseARB) {
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

	// XXX
	GL::LightHandler* _lightHandler = const_cast<GL::LightHandler*>(lightHandler);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	smfShaderGLSL->SetFlag("HAVE_SHADOWS", int(shadowHandler->shadowsLoaded));
	smfShaderGLSL->SetFlag("HAVE_INFOTEX", int(smfGroundDrawer->DrawExtraTex()));

	smfShaderGLSL->Enable();
	smfShaderGLSL->SetUniform2f(9, readmap->currMinHeight, readmap->currMaxHeight);
	smfShaderGLSL->SetUniform3fv(11, &camera->pos[0]);
	smfShaderGLSL->SetUniformMatrix4fv(13, false, shadowHandler->shadowMatrix);
	smfShaderGLSL->SetUniform4fv(14, &(shadowHandler->GetShadowParams().x));
	smfShaderGLSL->SetUniform1f(31, float(smfGroundDrawer->drawMode == CBaseGroundDrawer::drawMetal) + 1.0f);

	// already on the MV stack at this point
	glLoadIdentity();
	_lightHandler->Update(smfShaderGLSL);
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
	smfShaderGLSL->Disable();

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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
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
	smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
	smfShaderCurrARB->SetUniform4f(11, -sqx, -sqy, 0, 0);
}

void SMFRenderStateGLSL::SetSquareTexGen(const int sqx, const int sqy) const {
	smfShaderGLSL->SetUniform2i(8, sqx, sqy);
}



void SMFRenderStateARB::SetCurrentShader(const DrawPass::e& drawPass) {
	switch (drawPass) {
		case DrawPass::Normal:          { smfShaderCurrARB = smfShaderBaseARB; } break;
		case DrawPass::WaterReflection: { smfShaderCurrARB = smfShaderReflARB; } break;
		case DrawPass::WaterRefraction: { smfShaderCurrARB = smfShaderRefrARB; } break;
		case DrawPass::Shadow:
		case DrawPass::UnitReflection: break;
	}
}



void SMFRenderStateARB::UpdateCurrentShader(const ISkyLight* skyLight) const {
	smfShaderCurrARB->Enable();
	smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, skyLight->GetGroundShadowDensity());
	smfShaderCurrARB->Disable();
}

void SMFRenderStateGLSL::UpdateCurrentShader(const ISkyLight* skyLight) const {
	smfShaderGLSL->Enable();
	smfShaderGLSL->SetUniform4fv(10, &skyLight->GetLightDir().x);
	smfShaderGLSL->SetUniform1f(18, skyLight->GetGroundShadowDensity());
	smfShaderGLSL->Disable();
}

