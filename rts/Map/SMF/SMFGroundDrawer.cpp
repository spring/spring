/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFReadMap.h"
#include "SMFGroundDrawer.h"
#include "SMFGroundTextures.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/SMF/Legacy/LegacyMeshDrawer.h"
#include "Map/SMF/ROAM/RoamMeshDrawer.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "System/Config/ConfigHandler.h"
#include "System/FastMath.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/mmgr.h"
#include "System/Util.h"


CONFIG(int, GroundDetail).defaultValue(40);

CONFIG(int, MaxDynamicMapLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(int, AdvMapShading).defaultValue(1);

CONFIG(int, ROAM)
	.defaultValue(VBO)
	.description("Use ROAM for terrain mesh rendering. 1=VBO mode, 2=DL mode, 3=VA mode");


CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* rm)
	: smfMap(rm)
	, meshDrawer(NULL)
{
	groundTextures = new CSMFGroundTextures(smfMap);

	groundDetail = configHandler->GetInt("GroundDetail");

	useShaders = false;
	waterDrawn = false;

	waterPlaneCamInDispList  = 0;
	waterPlaneCamOutDispList = 0;

	if (mapInfo->water.hasWaterPlane) {
		waterPlaneCamInDispList = glGenLists(1);
		glNewList(waterPlaneCamInDispList, GL_COMPILE);
		CreateWaterPlanes(false);
		glEndList();

		waterPlaneCamOutDispList = glGenLists(1);
		glNewList(waterPlaneCamOutDispList, GL_COMPILE);
		CreateWaterPlanes(true);
		glEndList();
	}

	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicMapLights"));
	advShading = LoadMapShaders();

	bool useROAM = !!configHandler->GetInt("ROAM");
	SwitchMeshDrawer(useROAM ? SMF_MESHDRAWER_ROAM : SMF_MESHDRAWER_LEGACY);
}


CSMFGroundDrawer::~CSMFGroundDrawer()
{
	bool roamUsed = dynamic_cast<CRoamMeshDrawer*>(meshDrawer);

	delete groundTextures;
	delete meshDrawer;

	if (!roamUsed) configHandler->Set("ROAM", 0); // if enabled, the configvar is written in CRoamMeshDrawer's dtor
	configHandler->Set("GroundDetail", groundDetail);

	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");

	if (waterPlaneCamInDispList) {
		glDeleteLists(waterPlaneCamInDispList, 1);
		glDeleteLists(waterPlaneCamOutDispList, 1);
	}

	lightHandler.Kill();
}


void CSMFGroundDrawer::SwitchMeshDrawer(int mode)
{
	const int curMode = (dynamic_cast<CRoamMeshDrawer*>(meshDrawer) ? SMF_MESHDRAWER_ROAM : SMF_MESHDRAWER_LEGACY);

	// mode == -1: toogle modes
	if (mode < 0) {
		mode = curMode + 1;
		mode %= SMF_MESHDRAWER_LAST;
	}

#ifdef USE_GML
	//FIXME: ROAM isn't GML ready yet
	mode = SMF_MESHDRAWER_LEGACY;
#endif
	
	if ((curMode == mode) && (meshDrawer != NULL))
		return;

	delete meshDrawer;

	switch (mode) {
		case SMF_MESHDRAWER_LEGACY:
			LOG("Switching to Legacy Mesh Rendering");
			meshDrawer = new CLegacyMeshDrawer(smfMap, this);
			break;
#ifndef USE_GML
		default:
			LOG("Switching to ROAM Mesh Rendering");
			meshDrawer = new CRoamMeshDrawer(smfMap, this);
			break;
#endif
	}
}


bool CSMFGroundDrawer::LoadMapShaders() {
	#define sh shaderHandler

	smfShaderBaseARB = NULL;
	smfShaderReflARB = NULL;
	smfShaderRefrARB = NULL;
	smfShaderCurrARB = NULL;

	smfShaderDefGLSL = NULL;
	smfShaderAdvGLSL = NULL;
	smfShaderCurGLSL = NULL;

	if (configHandler->GetInt("AdvMapShading") == 0) {
		// not allowed to do shader-based map rendering
		return false;
	}

	if (globalRendering->haveARB && !globalRendering->haveGLSL) {
		smfShaderBaseARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderBaseARB", true);
		smfShaderReflARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderReflARB", true);
		smfShaderRefrARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderRefrARB", true);
		smfShaderCurrARB = smfShaderBaseARB;

		// the ARB shaders always assume shadows are on
		smfShaderBaseARB->AttachShaderObject(sh->CreateShaderObject("ARB/ground.vp", "", GL_VERTEX_PROGRAM_ARB));
		smfShaderBaseARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
		smfShaderBaseARB->Link();

		smfShaderReflARB->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundreflectinverted.vp", "", GL_VERTEX_PROGRAM_ARB));
		smfShaderReflARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
		smfShaderReflARB->Link();

		smfShaderRefrARB->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundrefract.vp", "", GL_VERTEX_PROGRAM_ARB));
		smfShaderRefrARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
		smfShaderRefrARB->Link();
	} else {
		if (globalRendering->haveGLSL) {
			smfShaderDefGLSL = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderDefGLSL", false);
			smfShaderAdvGLSL = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderAdvGLSL", false);
			smfShaderCurGLSL = smfShaderDefGLSL;

			std::string extraDefs;
				extraDefs += (smfMap->HaveSpecularLighting())?
					"#define SMF_ARB_LIGHTING 0\n":
					"#define SMF_ARB_LIGHTING 1\n";
				extraDefs += (smfMap->HaveSplatTexture())?
					"#define SMF_DETAIL_TEXTURE_SPLATTING 1\n":
					"#define SMF_DETAIL_TEXTURE_SPLATTING 0\n";
				extraDefs += (readmap->initMinHeight > 0.0f || mapInfo->map.voidWater)?
					"#define SMF_WATER_ABSORPTION 0\n":
					"#define SMF_WATER_ABSORPTION 1\n";
				extraDefs += (smfMap->GetSkyReflectModTexture() != 0)?
					"#define SMF_SKY_REFLECTIONS 1\n":
					"#define SMF_SKY_REFLECTIONS 0\n";
				extraDefs += (smfMap->GetDetailNormalTexture() != 0)?
					"#define SMF_DETAIL_NORMALS 1\n":
					"#define SMF_DETAIL_NORMALS 0\n";
				extraDefs += (smfMap->GetLightEmissionTexture() != 0)?
					"#define SMF_LIGHT_EMISSION 1\n":
					"#define SMF_LIGHT_EMISSION 0\n";
				extraDefs +=
					("#define BASE_DYNAMIC_MAP_LIGHT " + IntToString(lightHandler.GetBaseLight()) + "\n") +
					("#define MAX_DYNAMIC_MAP_LIGHTS " + IntToString(lightHandler.GetMaxLights()) + "\n");

			static const std::string shadowDefs[2] = {
				"#define HAVE_SHADOWS 0\n",
				"#define HAVE_SHADOWS 1\n",
			};

			Shader::IProgramObject* smfShaders[2] = {
				smfShaderDefGLSL,
				smfShaderAdvGLSL,
			};

			for (unsigned int i = 0; i < 2; i++) {
				smfShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFVertProg.glsl", extraDefs + shadowDefs[i], GL_VERTEX_SHADER));
				smfShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFFragProg.glsl", extraDefs + shadowDefs[i], GL_FRAGMENT_SHADER));
				smfShaders[i]->Link();

				smfShaders[i]->SetUniformLocation("diffuseTex");          // idx  0
				smfShaders[i]->SetUniformLocation("normalsTex");          // idx  1
				smfShaders[i]->SetUniformLocation("shadowTex");           // idx  2
				smfShaders[i]->SetUniformLocation("detailTex");           // idx  3
				smfShaders[i]->SetUniformLocation("specularTex");         // idx  4
				smfShaders[i]->SetUniformLocation("mapSizePO2");          // idx  5
				smfShaders[i]->SetUniformLocation("mapSize");             // idx  6
				smfShaders[i]->SetUniformLocation("texSquare");           // idx  7
				smfShaders[i]->SetUniformLocation("$UNUSED$");            // idx  8
				smfShaders[i]->SetUniformLocation("lightDir");            // idx  9
				smfShaders[i]->SetUniformLocation("cameraPos");           // idx 10
				smfShaders[i]->SetUniformLocation("$UNUSED$");            // idx 11
				smfShaders[i]->SetUniformLocation("shadowMat");           // idx 12
				smfShaders[i]->SetUniformLocation("shadowParams");        // idx 13
				smfShaders[i]->SetUniformLocation("groundAmbientColor");  // idx 14
				smfShaders[i]->SetUniformLocation("groundDiffuseColor");  // idx 15
				smfShaders[i]->SetUniformLocation("groundSpecularColor"); // idx 16
				smfShaders[i]->SetUniformLocation("groundShadowDensity"); // idx 17
				smfShaders[i]->SetUniformLocation("waterMinColor");       // idx 18
				smfShaders[i]->SetUniformLocation("waterBaseColor");      // idx 19
				smfShaders[i]->SetUniformLocation("waterAbsorbColor");    // idx 20
				smfShaders[i]->SetUniformLocation("splatDetailTex");      // idx 21
				smfShaders[i]->SetUniformLocation("splatDistrTex");       // idx 22
				smfShaders[i]->SetUniformLocation("splatTexScales");      // idx 23
				smfShaders[i]->SetUniformLocation("splatTexMults");       // idx 24
				smfShaders[i]->SetUniformLocation("skyReflectTex");       // idx 25
				smfShaders[i]->SetUniformLocation("skyReflectModTex");    // idx 26
				smfShaders[i]->SetUniformLocation("detailNormalTex");     // idx 27
				smfShaders[i]->SetUniformLocation("lightEmissionTex");    // idx 28
				smfShaders[i]->SetUniformLocation("numMapDynLights");     // idx 29
				smfShaders[i]->SetUniformLocation("normalTexGen");        // idx 30
				smfShaders[i]->SetUniformLocation("specularTexGen");      // idx 31

				smfShaders[i]->Enable();
				smfShaders[i]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
				smfShaders[i]->SetUniform1i(1, 5); // normalsTex  (idx 1, texunit 5)
				smfShaders[i]->SetUniform1i(2, 4); // shadowTex   (idx 2, texunit 4)
				smfShaders[i]->SetUniform1i(3, 2); // detailTex   (idx 3, texunit 2)
				smfShaders[i]->SetUniform1i(4, 6); // specularTex (idx 4, texunit 6)
				smfShaders[i]->SetUniform2f(5, (gs->pwr2mapx * SQUARE_SIZE), (gs->pwr2mapy * SQUARE_SIZE));
				smfShaders[i]->SetUniform2f(6, (gs->mapx * SQUARE_SIZE), (gs->mapy * SQUARE_SIZE));
				smfShaders[i]->SetUniform4fv(9, &sky->GetLight()->GetLightDir().x);
				smfShaders[i]->SetUniform3fv(14, &mapInfo->light.groundAmbientColor[0]);
				smfShaders[i]->SetUniform3fv(15, &mapInfo->light.groundSunColor[0]);
				smfShaders[i]->SetUniform3fv(16, &mapInfo->light.groundSpecularColor[0]);
				smfShaders[i]->SetUniform1f(17, sky->GetLight()->GetGroundShadowDensity());
				smfShaders[i]->SetUniform3fv(18, &mapInfo->water.minColor[0]);
				smfShaders[i]->SetUniform3fv(19, &mapInfo->water.baseColor[0]);
				smfShaders[i]->SetUniform3fv(20, &mapInfo->water.absorb[0]);
				smfShaders[i]->SetUniform1i(21, 7); // splatDetailTex (idx 21, texunit 7)
				smfShaders[i]->SetUniform1i(22, 8); // splatDistrTex (idx 22, texunit 8)
				smfShaders[i]->SetUniform4fv(23, &mapInfo->splats.texScales[0]);
				smfShaders[i]->SetUniform4fv(24, &mapInfo->splats.texMults[0]);
				smfShaders[i]->SetUniform1i(25,  9); // skyReflectTex (idx 25, texunit 9)
				smfShaders[i]->SetUniform1i(26, 10); // skyReflectModTex (idx 26, texunit 10)
				smfShaders[i]->SetUniform1i(27, 11); // detailNormalTex (idx 27, texunit 11)
				smfShaders[i]->SetUniform1i(28, 12); // lightEmisionTex (idx 28, texunit 12)
				smfShaders[i]->SetUniform1i(29,  0); // numMapDynLights (unused)
				smfShaders[i]->SetUniform2f(30, 1.0f / ((smfMap->normalTexSize.x - 1) * SQUARE_SIZE), 1.0f / ((smfMap->normalTexSize.y - 1) * SQUARE_SIZE));
				smfShaders[i]->SetUniform2f(31, 1.0f / (gs->mapx * SQUARE_SIZE), 1.0f / (gs->mapy * SQUARE_SIZE));
				smfShaders[i]->Disable();
				smfShaders[i]->Validate();
			}
		}
	}

	#undef sh
	return (smfShaderCurrARB != NULL || smfShaderCurGLSL != NULL);
}


void CSMFGroundDrawer::UpdateSunDir() {
	/* the GLSL shader may run even w/o shadows and depends on a correct sunDir
	if (!shadowHandler->shadowsLoaded) {
		return;
	}
	*/

	if (smfShaderCurGLSL != NULL) {
		smfShaderCurGLSL->Enable();
		smfShaderCurGLSL->SetUniform4fv(9, &sky->GetLight()->GetLightDir().x);
		smfShaderCurGLSL->SetUniform1f(17, sky->GetLight()->GetGroundShadowDensity());
		smfShaderCurGLSL->Disable();
	} else {
		if (smfShaderCurrARB != NULL) {
			smfShaderCurrARB->Enable();
			smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, sky->GetLight()->GetGroundShadowDensity());
			smfShaderCurrARB->Disable();
		}
	}
}


void CSMFGroundDrawer::CreateWaterPlanes(bool camOufOfMap) {
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);

	const float xsize = (smfMap->mapSizeX) >> 2;
	const float ysize = (smfMap->mapSizeZ) >> 2;
	const float size = std::min(xsize, ysize);

	CVertexArray* va = GetVertexArray();
	va->Initialize();

	const unsigned char fogColor[4] = {
		(255 * mapInfo->atmosphere.fogColor[0]),
		(255 * mapInfo->atmosphere.fogColor[1]),
		(255 * mapInfo->atmosphere.fogColor[2]),
		 255
	};

	const unsigned char planeColor[4] = {
		(255 * mapInfo->water.planeColor[0]),
		(255 * mapInfo->water.planeColor[1]),
		(255 * mapInfo->water.planeColor[2]),
		 255
	};

	const float alphainc = fastmath::PI2 / 32;
	float alpha,r1,r2;

	float3 p(0.0f, std::min(-200.0f, smfMap->initMinHeight - 400.0f), 0.0f);

	for (int n = (camOufOfMap) ? 0 : 1; n < 4 ; ++n) {
		if ((n == 1) && !camOufOfMap) {
			// don't render vertices under the map
			r1 = 2 * size;
		} else {
			r1 = n*n * size;
		}

		if (n == 3) {
			// last stripe: make it thinner (looks better with fog)
			r2 = (n+0.5)*(n+0.5) * size;
		} else {
			r2 = (n+1)*(n+1) * size;
		}
		for (alpha = 0.0f; (alpha - fastmath::PI2) < alphainc ; alpha += alphainc) {
			p.x = r1 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r1 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertexC(p, planeColor );
			p.x = r2 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r2 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertexC(p, (n==3) ? fogColor : planeColor);
		}
	}
	va->DrawArrayC(GL_TRIANGLE_STRIP);

	glDepthMask(GL_TRUE);
}


inline void CSMFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (!drawWaterReflection) {
		glCallList(camera->pos.IsInBounds()? waterPlaneCamInDispList: waterPlaneCamOutDispList);
	}
}


void CSMFGroundDrawer::Draw(const DrawPass::e& drawPass)
{
	if (mapInfo->map.voidWater && readmap->currMaxHeight < 0.0f) {
		return;
	}

	smfShaderCurGLSL = shadowHandler->shadowsLoaded? smfShaderAdvGLSL: smfShaderDefGLSL;
	useShaders = advShading && (!DrawExtraTex() && ((smfShaderCurrARB != NULL && shadowHandler->shadowsLoaded) || (smfShaderCurGLSL != NULL)));
	waterDrawn = (drawPass == DrawPass::WaterReflection); //FIXME remove. save drawPass somewhere instead


	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_2D); // needed for the non-shader case

	UpdateCamRestraints(cam2);
	SetupTextureUnits(waterDrawn);

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
			if (mapInfo->map.voidWater && (drawPass != DrawPass::WaterReflection)) {
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER, 0.9f);
			}

				meshDrawer->DrawMesh(drawPass);
	
			if (mapInfo->map.voidWater && (drawPass != DrawPass::WaterReflection)) {
				glDisable(GL_ALPHA_TEST);
			}

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

	ResetTextureUnits(waterDrawn);
	glDisable(GL_CULL_FACE);

	if (drawPass == DrawPass::Normal) {
		if (mapInfo->water.hasWaterPlane) {
			DrawWaterPlane(waterDrawn);
		}

		groundDecals->Draw();
		projectileDrawer->DrawGroundFlashes();
	}
}


void CSMFGroundDrawer::DrawShadowPass(void)
{
	if (mapInfo->map.voidWater && readmap->currMaxHeight < 0.0f) {
		return;
	}

	Shader::IProgramObject* po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	glEnable(GL_POLYGON_OFFSET_FILL);

	glPolygonOffset(-1.f, -1.f);
		po->Enable();
			meshDrawer->DrawMesh(DrawPass::Shadow);
		po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
}


void CSMFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	groundTextures->BindSquareTexture(bigSquareX, bigSquareY);

	if (useShaders) {
		if (smfShaderCurGLSL != NULL) {
			smfShaderCurGLSL->SetUniform2i(7, bigSquareX, bigSquareY);
		} else {
			if (smfShaderCurrARB != NULL && shadowHandler->shadowsLoaded) {
				smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				smfShaderCurrARB->SetUniform4f(11, -bigSquareX, -bigSquareY, 0, 0);
			}
		}
	} else {
		SetTexGen(1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, -bigSquareX, -bigSquareY);
	}
}


void CSMFGroundDrawer::SetupTextureUnits(bool drawReflection)
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	if (DrawExtraTex()) {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, infoTex);
		glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
		if (drawMode == drawMetal) { // increased brightness for metal spots
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		}
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		glActiveTexture(GL_TEXTURE3);
		if (smfMap->GetDetailTexture()) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
			glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			static const GLfloat planeX[] = {0.02f, 0.0f, 0.0f, 0.0f};
			static const GLfloat planeZ[] = {0.0f, 0.0f, 0.02f, 0.0f};

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glDisable(GL_TEXTURE_2D);
		}

		#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
		// CDynamicWater overrides smfShaderBaseARB during the reflection / refraction
		// pass to distort underwater geometry, but because it's hard to maintain only
		// a vertex shader when working with texture combiners we don't enable this
		// note: we also want to disable culling for these passes
		if (smfShaderCurrARB != smfShaderBaseARB) {
			smfShaderCurrARB->Enable();
			smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);

			smfShaderCurrARB->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
			smfShaderCurrARB->SetUniform4f(12, 1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, 0, 1);
			smfShaderCurrARB->SetUniform4f(13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
			smfShaderCurrARB->SetUniform4f(14, 0.02f, 0.02f, 0, 1);

			if (drawReflection) {
				glAlphaFunc(GL_GREATER, 0.9f);
				glEnable(GL_ALPHA_TEST);
			}
		}
		#endif
	} else {
		if (useShaders) {
			const float3 ambientColor = mapInfo->light.groundAmbientColor * (210.0f / 255.0f);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());

			if (drawReflection) {
				// FIXME: why?
				glAlphaFunc(GL_GREATER, 0.8f);
				glEnable(GL_ALPHA_TEST);
			}

			if (smfShaderCurGLSL != NULL) {
				smfShaderCurGLSL->Enable();
				smfShaderCurGLSL->SetUniform3fv(10, &camera->pos[0]);
				smfShaderCurGLSL->SetUniformMatrix4fv(12, false, shadowHandler->shadowMatrix);
				smfShaderCurGLSL->SetUniform4fv(13, &(shadowHandler->GetShadowParams().x));

				// already on the MV stack
				glLoadIdentity();
				lightHandler.Update(smfShaderCurGLSL);
				glMultMatrixf(camera->GetViewMatrix());

				glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());
				glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, smfMap->GetSpecularTexture());
				glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDetailTexture());
				glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDistrTexture());
				glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSkyReflectionTextureID());
				glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, smfMap->GetSkyReflectModTexture());
				glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailNormalTexture());
				glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, smfMap->GetLightEmissionTexture());

				// setup for shadow2DProj
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
				glActiveTexture(GL_TEXTURE0);
			} else {
				smfShaderCurrARB->Enable();
				smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				smfShaderCurrARB->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
				smfShaderCurrARB->SetUniform4f(12, 1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, 0, 1);
				smfShaderCurrARB->SetUniform4f(13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
				smfShaderCurrARB->SetUniform4f(14, 0.02f, 0.02f, 0, 1);
				smfShaderCurrARB->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
				smfShaderCurrARB->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1);
				smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, sky->GetLight()->GetGroundShadowDensity());

				glMatrixMode(GL_MATRIX0_ARB);
				glLoadMatrixf(shadowHandler->shadowMatrix);
				glMatrixMode(GL_MODELVIEW);

				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
				glActiveTexture(GL_TEXTURE0);
			}
		} else {
			if (smfMap->GetDetailTexture()) {
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
				glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

				static const GLfloat planeX[] = {0.02f, 0.0f, 0.00f, 0.0f};
				static const GLfloat planeZ[] = {0.00f, 0.0f, 0.02f, 0.0f};

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
			glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
			SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), -0.5f / gs->pwr2mapx, -0.5f / gs->pwr2mapy);

			// bind the detail texture a 2nd time to increase the details (-> GL_ADD_SIGNED_ARB is limited -0.5 to +0.5)
			// (also do this after the shading texture cause of color clamping issues)
			if (smfMap->GetDetailTexture()) {
				glActiveTexture(GL_TEXTURE3);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
				glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

				static const GLfloat planeX[] = {0.02f, 0.0f, 0.0f, 0.0f};
				static const GLfloat planeZ[] = {0.0f, 0.0f, 0.02f, 0.0f};

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

			#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
			// see comment above
			#endif
		}
	}

	glActiveTexture(GL_TEXTURE0);
}


void CSMFGroundDrawer::ResetTextureUnits(bool drawReflection)
{
	if (!useShaders) {
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(GL_TEXTURE0);
	} else {
		glActiveTexture(GL_TEXTURE4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
		glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);

		if (smfShaderCurGLSL != NULL) {
			smfShaderCurGLSL->Disable();
		} else {
			if (smfShaderCurrARB != NULL && shadowHandler->shadowsLoaded) {
				smfShaderCurrARB->Disable();
			}
		}
	}

	if (drawReflection) {
		glDisable(GL_ALPHA_TEST);
	}
}


void CSMFGroundDrawer::Update()
{
	if (mapInfo->map.voidWater && (readmap->currMaxHeight < 0.0f)) {
		return;
	}

	groundTextures->DrawUpdate();
	meshDrawer->Update();
}


void CSMFGroundDrawer::IncreaseDetail()
{
	groundDetail += 2;
	LOG("GroundDetail is now %i", groundDetail);
}

void CSMFGroundDrawer::DecreaseDetail()
{
	if (groundDetail > 4) {
		groundDetail -= 2;
		LOG("GroundDetail is now %i", groundDetail);
	}
}

int CSMFGroundDrawer::GetGroundDetail(const DrawPass::e& drawPass) const
{
	int detail = groundDetail;

	switch (drawPass) {
		case DrawPass::UnitReflection:
			detail *= LODScaleUnitReflection;
			break;
		case DrawPass::WaterReflection:
			detail *= LODScaleReflection;
			break;
		case DrawPass::WaterRefraction:
			detail *= LODScaleRefraction;
			break;
		//TODO: currently the shadow mesh needs to be idential with the normal pass one
		//  else we get z-fighting issues in the shadows. Ideal would be a special
		//  shadow pass mesh renderer that reduce the mesh to `walls`/contours that cause the
		//  same shadows as the original terrain
		//case DrawPass::Shadow:
		//	detail *= LODScaleShadow;
		//	break;
		default:
			break;
	}

	return std::max(4, detail);
}
