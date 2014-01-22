/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief extended bump-mapping water shader
 */


#include "BumpWater.h"

#include <boost/format.hpp>

#include "ISky.h"
#include "Game/Game.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/FeatureDrawer.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/Wind.h"
#include "System/bitops.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/Util.h"

using std::string;
using std::vector;
using std::min;
using std::max;

CONFIG(int, BumpWaterTexSizeReflection).defaultValue(512).minimumValue(32).description("Sets the size of the framebuffer texture used to store the reflection in Bumpmapped water.");
CONFIG(int, BumpWaterReflection).defaultValue(1).minimumValue(0).maximumValue(2).description("Determines the amount of objects reflected in Bumpmapped water.\n0:=off, 1:=fast (skip terrain), 2:=full");
CONFIG(int, BumpWaterRefraction).defaultValue(1).minimumValue(0).maximumValue(2).description("Determines the method of refraction with Bumpmapped water.\n0:=off, 1:=screencopy, 2:=own rendering cycle");
CONFIG(float, BumpWaterAnisotropy).defaultValue(0.0f).minimumValue(0.0f);
CONFIG(bool, BumpWaterUseDepthTexture).defaultValue(true);
CONFIG(int, BumpWaterDepthBits).defaultValue(24).minimumValue(16).maximumValue(32);
CONFIG(bool, BumpWaterBlurReflection).defaultValue(false);
CONFIG(bool, BumpWaterShoreWaves).defaultValue(true).safemodeValue(false).description("Enables rendering of shorewaves.");
CONFIG(bool, BumpWaterEndlessOcean).defaultValue(true).description("Sets whether Bumpmapped water will be drawn beyond the map edge.");
CONFIG(bool, BumpWaterDynamicWaves).defaultValue(true);
CONFIG(bool, BumpWaterUseUniforms).defaultValue(false);
CONFIG(bool, BumpWaterOcclusionQuery).defaultValue(false); //FIXME doesn't work as expected (it's slower than w/o), needs fixing


#define LOG_SECTION_BUMP_WATER "BumpWater"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_BUMP_WATER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_BUMP_WATER

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void GLSLDefineConst4f(string& str, const string& name, const float x, const float y, const float z, const float w)
{
	str += boost::str(boost::format(string("#define ")+name+" vec4(%1$.12f,%2$.12f,%3$.12f,%4$.12f)\n") % (x) % (y) % (z) % (w));
}

static void GLSLDefineConstf4(string& str, const string& name, const float3& v, const float& alpha)
{
	str += boost::str(boost::format(string("#define ")+name+" vec4(%1$.12f,%2$.12f,%3$.12f,%4$.12f)\n") % (v.x) % (v.y) % (v.z) % (alpha));
}


static void GLSLDefineConstf3(string& str, const string& name, const float3& v)
{
	str += boost::str(boost::format(string("#define ")+name+" vec3(%1$.12f,%2$.12f,%3$.12f)\n") % (v.x) % (v.y) % (v.z));
}


static void GLSLDefineConstf2(string& str, const string& name, const float& x, const float& y)
{
	str += boost::str(boost::format(string("#define ")+name+" vec2(%1$.12f,%2$.12f)\n") % x % y);
}


static void GLSLDefineConstf1(string& str, const string& name, const float& x)
{
	str += boost::str(boost::format(string("#define ")+name+" %1$.12f\n") % x);
}


static GLuint LoadTexture(const string& filename, const float anisotropy = 0.0f, int* sizeX = NULL, int* sizeY = NULL)
{
	GLuint texID;
	CBitmap bm;
	if (!bm.Load(filename)) {
		throw content_error("[" LOG_SECTION_BUMP_WATER "] Could not load texture from file " + filename);
	}

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	if (anisotropy > 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);

	if (sizeY != NULL) {
		*sizeX = bm.xsize;
		*sizeY = bm.ysize;
	}

	return texID;
}


static void DrawRadialDisc()
{
	//! SAME ALGORITHM AS FOR WATER-PLANE IN BFGroundDrawer.cpp!
	const float xsize = (gs->mapx * SQUARE_SIZE) >> 2;
	const float ysize = (gs->mapy * SQUARE_SIZE) >> 2;

	CVertexArray* va = GetVertexArray();
	va->Initialize();

	float3 p;

	const float alphainc = fastmath::PI2 / 32;
	const float size = std::min(xsize, ysize);

	for (int n = 0; n < 4 ; ++n) {
		const float r1 = n*n * size;
		float r2 = (n + 1) * (n + 1) * size;
		if (n == 3) {
			r2 = (n + 0.5) * (n + 0.5) * size;
		}
		for (float alpha = 0.0f; (alpha - fastmath::PI2) < alphainc; alpha += alphainc) {
			p.x = r1 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r1 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertex0(p);
			p.x = r2 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r2 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertex0(p);
		}
	}

	va->DrawArray0(GL_TRIANGLE_STRIP);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// (DE-)CONSTRUCTOR
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CBumpWater::CBumpWater()
	: CEventClient("[CBumpWater]", 271923, false)
	, target(GL_TEXTURE_2D)
	, screenTextureX(globalRendering->viewSizeX)
	, screenTextureY(globalRendering->viewSizeY)
	, displayList(0)
	, refractTexture(0)
	, reflectTexture(0)
	, depthTexture(0)
	, waveRandTexture(0)
	, foamTexture(0)
	, coastTexture(0)
	, normalTexture(0)
	, normalTexture2(0)
	, coastUpdateTexture(0)
	, wasVisibleLastFrame(false)
{
	eventHandler.AddClient(this);

	// LOAD USER CONFIGS
	reflTexSize  = next_power_of_2(configHandler->GetInt("BumpWaterTexSizeReflection"));
	reflection   = configHandler->GetInt("BumpWaterReflection");
	refraction   = configHandler->GetInt("BumpWaterRefraction");
	anisotropy   = configHandler->GetFloat("BumpWaterAnisotropy");
	depthCopy    = configHandler->GetBool("BumpWaterUseDepthTexture");
	depthBits    = configHandler->GetInt("BumpWaterDepthBits");
	if ((depthBits == 24) && !globalRendering->support24bitDepthBuffers)
		depthBits = 16;
	blurRefl     = configHandler->GetBool("BumpWaterBlurReflection");
	shoreWaves   = (configHandler->GetBool("BumpWaterShoreWaves")) && mapInfo->water.shoreWaves;
	endlessOcean = (configHandler->GetBool("BumpWaterEndlessOcean")) && mapInfo->water.hasWaterPlane
	               && ((readMap->HasVisibleWater()) || (mapInfo->water.forceRendering));
	dynWaves     = (configHandler->GetBool("BumpWaterDynamicWaves")) && (mapInfo->water.numTiles > 1);
	useUniforms  = (configHandler->GetBool("BumpWaterUseUniforms"));

	// CHECK HARDWARE
	if (!globalRendering->haveGLSL) {
		throw content_error("[" LOG_SECTION_BUMP_WATER "] your hardware/driver setup does not support GLSL");
	}

	shoreWaves = shoreWaves && (GLEW_EXT_framebuffer_object);
	dynWaves   = dynWaves && (GLEW_EXT_framebuffer_object && GLEW_ARB_imaging);


	// LOAD TEXTURES
	foamTexture   = LoadTexture(mapInfo->water.foamTexture);
	normalTexture = LoadTexture(mapInfo->water.normalTexture , anisotropy , &normalTextureX, &normalTextureY);

	//! caustic textures
	const vector<string>& causticNames = mapInfo->water.causticTextures;
	if (causticNames.empty()) {
		throw content_error("[" LOG_SECTION_BUMP_WATER "] no caustic textures");
	}
	for (int i = 0; i < (int)causticNames.size(); ++i) {
		caustTextures.push_back(LoadTexture(causticNames[i]));
	}

	// CHECK SHOREWAVES TEXTURE SIZE
	if (shoreWaves) {
		GLint maxw, maxh;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA16F_ARB, 4096, 4096, 0, GL_RGBA, GL_FLOAT, NULL);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &maxw);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &maxh);
		if (gs->mapx>maxw || gs->mapy>maxh) {
			shoreWaves = false;
			LOG_L(L_WARNING, "Can not display shorewaves (map too large)!");
		}
	}


	// SHOREWAVES
	if (shoreWaves) {
		waveRandTexture = LoadTexture( "bitmaps/shorewaverand.png" );

		glGenTextures(1, &coastTexture);
		glBindTexture(GL_TEXTURE_2D, coastTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, gs->mapx, gs->mapy, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, gs->mapx, gs->mapy, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		//glGenerateMipmapEXT(GL_TEXTURE_2D);


		{
			blurShader = shaderHandler->CreateProgramObject("[BumpWater]", "CoastBlurShader", false);
			blurShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterCoastBlurFS.glsl", "", GL_FRAGMENT_SHADER));
			blurShader->Link();

			if (!blurShader->IsValid()) {
				const char* fmt = "shorewaves-shader compilation error: %s";
				const char* log = (blurShader->GetLog()).c_str();

				LOG_L(L_ERROR, fmt, log);

				//! string size is limited with content_error()
				throw content_error(string("[" LOG_SECTION_BUMP_WATER "] shorewaves-shader compilation error!"));
			}

			blurShader->SetUniformLocation("tex0"); // idx 0
			blurShader->SetUniformLocation("tex1"); // idx 1
			blurShader->Enable();
			blurShader->SetUniform1i(0, 0);
			blurShader->SetUniform1i(1, 1);
			blurShader->Disable();
			blurShader->Validate();

			if (!blurShader->IsValid()) {
				const char* fmt = "shorewaves-shader validation error: %s";
				const char* log = (blurShader->GetLog()).c_str();

				LOG_L(L_ERROR, fmt, log);
				throw content_error(string("[" LOG_SECTION_BUMP_WATER "] shorewaves-shader validation error!"));
			}
		}


		coastFBO.reloadOnAltTab = true;
		coastFBO.Bind();
		coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);

		if (coastFBO.CheckStatus("BUMPWATER(Coastmap)")) {
			//! initialize texture
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			//! fill with current heightmap/coastmap
			UnsyncedHeightMapUpdate(SRectangle(0, 0, gs->mapx, gs->mapy));
			UploadCoastline(true);
			UpdateCoastmap();
		} else
			shoreWaves = false;

		if (shoreWaves)
			eventHandler.InsertEvent(this, "UnsyncedHeightMapUpdate");

		//coastFBO.Unbind(); // gets done below
	}


	// CREATE TEXTURES
	if ((refraction > 0) || depthCopy) {
		//! ATIs do not have GLSL support for texrects
		if (GLEW_ARB_texture_rectangle && !globalRendering->atiHacks) {
			target = GL_TEXTURE_RECTANGLE_ARB;
		} else if (!globalRendering->supportNPOTs) {
			screenTextureX = next_power_of_2(screenTextureX);
			screenTextureY = next_power_of_2(screenTextureY);
		}
	}

	if (refraction > 0) {
		//! CREATE REFRACTION TEXTURE
		glGenTextures(1, &refractTexture);
		glBindTexture(target, refractTexture);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		if (GLEW_EXT_texture_edge_clamp) {
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
		glTexImage2D(target, 0, GL_RGBA8, screenTextureX, screenTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	if (reflection > 0) {
		//! CREATE REFLECTION TEXTURE
		glGenTextures(1, &reflectTexture);
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		if (GLEW_EXT_texture_edge_clamp) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	if (depthCopy) {
		//! CREATE DEPTH TEXTURE
		glGenTextures(1, &depthTexture);
		glBindTexture(target, depthTexture);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLuint depthFormat = GL_DEPTH_COMPONENT32;
		if (!globalRendering->atiHacks) { depthFormat = GL_DEPTH_COMPONENT24; }
		glTexImage2D(target, 0, depthFormat, screenTextureX, screenTextureY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}

	if (dynWaves) {
		//! SETUP DYNAMIC WAVES
		tileOffsets = new unsigned char[mapInfo->water.numTiles * mapInfo->water.numTiles];

		normalTexture2 = normalTexture;
		glBindTexture(GL_TEXTURE_2D, normalTexture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glGenTextures(1, &normalTexture);
		glBindTexture(GL_TEXTURE_2D, normalTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (anisotropy > 0.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, normalTextureX, normalTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmapEXT(GL_TEXTURE_2D);
	}

	// CREATE FBOs
	if (GLEW_EXT_framebuffer_object) {
		GLuint depthRBOFormat = GL_DEPTH_COMPONENT;
		switch (depthBits) {
			case 16: depthRBOFormat = GL_DEPTH_COMPONENT16; break;
			case 24: depthRBOFormat = GL_DEPTH_COMPONENT24; break;
			case 32: depthRBOFormat = GL_DEPTH_COMPONENT32; break;
		}

		if (reflection>0) {
			reflectFBO.Bind();
			reflectFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, depthRBOFormat, reflTexSize, reflTexSize);
			reflectFBO.AttachTexture(reflectTexture);
			if (!reflectFBO.CheckStatus("BUMPWATER(reflection)")) {
				reflection = 0;
			}
		}

		if (refraction>0) {
			refractFBO.Bind();
			refractFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, depthRBOFormat, screenTextureX, screenTextureY);
			refractFBO.AttachTexture(refractTexture,target);
			if (!refractFBO.CheckStatus("BUMPWATER(refraction)")) {
				refraction = 0;
			}
		}

		if (dynWaves) {
			dynWavesFBO.reloadOnAltTab = true;
			dynWavesFBO.Bind();
			dynWavesFBO.AttachTexture(normalTexture);
			if (dynWavesFBO.CheckStatus("BUMPWATER(DynWaves)")) {
				UpdateDynWaves(true); //! initialize
			}
		}
		FBO::Unbind();
	}


	/*
	 * DEFINE SOME SHADER RUNTIME CONSTANTS
	 * (I do not use Uniforms for that, because the GLSL compiler can not
	 * optimize those!)
	 */
	string definitions;
	if (reflection>0) definitions += "#define opt_reflection\n";
	if (refraction>0) definitions += "#define opt_refraction\n";
	if (shoreWaves)   definitions += "#define opt_shorewaves\n";
	if (depthCopy)    definitions += "#define opt_depth\n";
	if (blurRefl)     definitions += "#define opt_blurreflection\n";
	if (endlessOcean) definitions += "#define opt_endlessocean\n";
	if (target == GL_TEXTURE_RECTANGLE_ARB) definitions += "#define opt_texrect\n";

	GLSLDefineConstf3(definitions, "MapMid",                    float3(gs->mapx * SQUARE_SIZE * 0.5f, 0.0f, gs->mapy * SQUARE_SIZE * 0.5f));
	GLSLDefineConstf2(definitions, "ScreenInverse",             1.0f / globalRendering->viewSizeX, 1.0f / globalRendering->viewSizeY);
	GLSLDefineConstf2(definitions, "ScreenTextureSizeInverse",  1.0f / screenTextureX, 1.0f / screenTextureY);
	GLSLDefineConstf2(definitions, "ViewPos",                   globalRendering->viewPosX, globalRendering->viewPosY);

	if (useUniforms) {
		SetupUniforms(definitions);
	} else {
		GLSLDefineConstf4(definitions, "SurfaceColor",   mapInfo->water.surfaceColor*0.4, mapInfo->water.surfaceAlpha );
		GLSLDefineConstf4(definitions, "PlaneColor",     mapInfo->water.planeColor*0.4, mapInfo->water.surfaceAlpha );
		GLSLDefineConstf3(definitions, "DiffuseColor",   mapInfo->water.diffuseColor);
		GLSLDefineConstf3(definitions, "SpecularColor",  mapInfo->water.specularColor);
		GLSLDefineConstf1(definitions, "SpecularPower",  mapInfo->water.specularPower);
		GLSLDefineConstf1(definitions, "SpecularFactor", mapInfo->water.specularFactor);
		GLSLDefineConstf1(definitions, "AmbientFactor",  mapInfo->water.ambientFactor);
		GLSLDefineConstf1(definitions, "DiffuseFactor",  mapInfo->water.diffuseFactor * 15.0f);
		GLSLDefineConstf3(definitions, "SunDir",         sky->GetLight()->GetLightDir()); // FIXME: not a constant
		GLSLDefineConstf1(definitions, "FresnelMin",     mapInfo->water.fresnelMin);
		GLSLDefineConstf1(definitions, "FresnelMax",     mapInfo->water.fresnelMax);
		GLSLDefineConstf1(definitions, "FresnelPower",   mapInfo->water.fresnelPower);
		GLSLDefineConstf1(definitions, "ReflDistortion", mapInfo->water.reflDistortion);
		GLSLDefineConstf2(definitions, "BlurBase",       0.0f, mapInfo->water.blurBase / globalRendering->viewSizeY);
		GLSLDefineConstf1(definitions, "BlurExponent",   mapInfo->water.blurExponent);
		GLSLDefineConstf1(definitions, "PerlinStartFreq",  mapInfo->water.perlinStartFreq);
		GLSLDefineConstf1(definitions, "PerlinLacunarity", mapInfo->water.perlinLacunarity);
		GLSLDefineConstf1(definitions, "PerlinAmp",        mapInfo->water.perlinAmplitude);
		GLSLDefineConstf1(definitions, "WindSpeed",        mapInfo->water.windSpeed);
		GLSLDefineConstf1(definitions, "shadowDensity",  sky->GetLight()->GetGroundShadowDensity());
	}

	{
		const int mapX = gs->mapx  * SQUARE_SIZE;
		const int mapZ = gs->mapy * SQUARE_SIZE;
		const float shadingX = (float)gs->mapx / gs->pwr2mapx;
		const float shadingZ = (float)gs->mapy / gs->pwr2mapy;
		const float scaleX = (mapX > mapZ) ? (gs->mapy/64)/16.0f * (float)mapX/mapZ : (gs->mapx/64)/16.0f;
		const float scaleZ = (mapX > mapZ) ? (gs->mapy/64)/16.0f : (gs->mapx/64)/16.0f * (float)mapZ/mapX;
		GLSLDefineConst4f(definitions, "TexGenPlane", 1.0f/mapX, 1.0f/mapZ, scaleX/mapX, scaleZ/mapZ);
		GLSLDefineConst4f(definitions, "ShadingPlane", shadingX/mapX, shadingZ/mapZ, shadingX, shadingZ);
	}



	// LOAD SHADERS
	{
		waterShader = shaderHandler->CreateProgramObject("[BumpWater]", "WaterShader", false);
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterVS.glsl", definitions, GL_VERTEX_SHADER));
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterFS.glsl", definitions, GL_FRAGMENT_SHADER));
		waterShader->Link();
		waterShader->SetUniformLocation("eyePos");      // idx  0
		waterShader->SetUniformLocation("frame");       // idx  1
		waterShader->SetUniformLocation("normalmap");   // idx  2, texunit 0
		waterShader->SetUniformLocation("heightmap");   // idx  3, texunit 1
		waterShader->SetUniformLocation("caustic");     // idx  4, texunit 2
		waterShader->SetUniformLocation("foam");        // idx  5, texunit 3
		waterShader->SetUniformLocation("reflection");  // idx  6, texunit 4
		waterShader->SetUniformLocation("refraction");  // idx  7, texunit 5
		waterShader->SetUniformLocation("depthmap");    // idx  8, texunit 7
		waterShader->SetUniformLocation("coastmap");    // idx  9, texunit 6
		waterShader->SetUniformLocation("waverand");    // idx 10, texunit 8
		waterShader->SetUniformLocation("infotex");     // idx 11, texunit 10
		waterShader->SetUniformLocation("shadowmap");   // idx 12, texunit 9
		waterShader->SetUniformLocation("shadowMatrix");   // idx 13

		if (!waterShader->IsValid()) {
			const char* fmt = "water-shader compilation error: %s";
			const char* log = (waterShader->GetLog()).c_str();
			LOG_L(L_ERROR, fmt, log);
			throw content_error(string("[" LOG_SECTION_BUMP_WATER "] water-shader compilation error!"));
		}

		if (useUniforms) {
			GetUniformLocations(waterShader);
		}

		// BIND TEXTURE UNIFORMS
		// NOTE: ATI shader validation code is stricter wrt. state,
		// so postpone the call until all texture uniforms are set
		waterShader->Enable();
		waterShader->SetUniform1i( 2, 0);
		waterShader->SetUniform1i( 3, 1);
		waterShader->SetUniform1i( 4, 2);
		waterShader->SetUniform1i( 5, 3);
		waterShader->SetUniform1i( 6, 4);
		waterShader->SetUniform1i( 7, 5);
		waterShader->SetUniform1i( 8, 7);
		waterShader->SetUniform1i( 9, 6);
		waterShader->SetUniform1i(10, 8);
		waterShader->SetUniform1i(11, 10);
		waterShader->SetUniform1i(12, 9);
		waterShader->Disable();
		waterShader->Validate();

		if (!waterShader->IsValid()) {
			const char* fmt = "water-shader validation error: %s";
			const char* log = (waterShader->GetLog()).c_str();

			LOG_L(L_ERROR, fmt, log);
			throw content_error(string("[" LOG_SECTION_BUMP_WATER "] water-shader validation error!"));
		}
	}


	// CREATE DISPLAYLIST
	displayList = glGenLists(1);
	glNewList(displayList, GL_COMPILE);
	if (endlessOcean) {
		DrawRadialDisc();
	} else {
		const int mapX = gs->mapx * SQUARE_SIZE;
		const int mapZ = gs->mapy * SQUARE_SIZE;
		CVertexArray* va = GetVertexArray();
		va->Initialize();
		for (int z = 0; z < 9; z++) {
			for (int x = 0; x < 10; x++) {
				for (int zs = 0; zs <= 1; zs++) {
					va->AddVertex0(float3(x*(mapX/9.0f), 0.0f, (z + zs)*(mapZ/9.0f)));
				}
			}
			va->EndStrip();
		}
		va->DrawArray0(GL_TRIANGLE_STRIP);
	}
	glEndList();

/*
	windndir = wind.GetCurrentDirection();
	windStrength = (smoothstep(0.0f, 12.0f, wind.GetCurrentStrength()) * 0.5f + 4.0f);
	windVec = windndir * windStrength;
*/
	windVec = float3(20.0, 0.0, 20.0);

	occlusionQuery = 0;
	occlusionQueryResult = GL_TRUE;
	wasVisibleLastFrame = true;
#ifdef GLEW_ARB_occlusion_query2
	bool useOcclQuery  = (configHandler->GetBool("BumpWaterOcclusionQuery"));
	if (useOcclQuery && GLEW_ARB_occlusion_query2 && (refraction < 2)) { //! in the case of a separate refraction pass, there isn't enough time for a occlusion query
		GLint bitsSupported;
		glGetQueryiv(GL_ANY_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &bitsSupported);
		if (bitsSupported > 0) {
			glGenQueries(1, &occlusionQuery);
		}
	}
#endif

	if (refraction > 1) {
		drawSolid = true;
	}
}


CBumpWater::~CBumpWater()
{
	if (reflectTexture) {
		glDeleteTextures(1, &reflectTexture);
	}
	if (refractTexture) {
		glDeleteTextures(1, &refractTexture);
	}
	if (depthCopy) {
		glDeleteTextures(1, &depthTexture);
	}

	glDeleteTextures(1, &foamTexture);
	glDeleteTextures(1, &normalTexture);
	for (int i = 0; i < (int)caustTextures.size(); ++i) {
		glDeleteTextures(1, &caustTextures[i]);
	}

	glDeleteLists(displayList, 1);

	if (shoreWaves) {
		glDeleteTextures(1, &coastTexture);
		glDeleteTextures(1, &waveRandTexture);
	}

	if (dynWaves) {
		glDeleteTextures(1, &normalTexture2);
		delete[] tileOffsets;
	}

#ifdef GLEW_ARB_occlusion_query2
	if (occlusionQuery) {
		glDeleteQueries(1, &occlusionQuery);
	}
#endif

	shaderHandler->ReleaseProgramObjects("[BumpWater]");
}


void CBumpWater::SetupUniforms(string& definitions)
{
	definitions += "uniform vec4  SurfaceColor;\n";
	definitions += "uniform vec4  PlaneColor;\n";
	definitions += "uniform vec3  DiffuseColor;\n";
	definitions += "uniform vec3  SpecularColor;\n";
	definitions += "uniform float SpecularPower;\n";
	definitions += "uniform float SpecularFactor;\n";
	definitions += "uniform float AmbientFactor;\n";
	definitions += "uniform float DiffuseFactor;\n";
	definitions += "uniform vec3  SunDir;\n";
	definitions += "uniform float FresnelMin;\n";
	definitions += "uniform float FresnelMax;\n";
	definitions += "uniform float FresnelPower;\n";
	definitions += "uniform float ReflDistortion;\n";
	definitions += "uniform vec2  BlurBase;\n";
	definitions += "uniform float BlurExponent;\n";
	definitions += "uniform float PerlinStartFreq;\n";
	definitions += "uniform float PerlinLacunarity;\n";
	definitions += "uniform float PerlinAmp;\n";
	definitions += "uniform float WindSpeed;\n";
	definitions += "uniform float shadowDensity;\n";
}

void CBumpWater::GetUniformLocations(const Shader::IProgramObject* shader)
{
	uniforms[ 0] = glGetUniformLocation( shader->GetObjID(), "SurfaceColor" );
	uniforms[ 1] = glGetUniformLocation( shader->GetObjID(), "PlaneColor" );
	uniforms[ 2] = glGetUniformLocation( shader->GetObjID(), "DiffuseColor" );
	uniforms[ 3] = glGetUniformLocation( shader->GetObjID(), "SpecularColor" );
	uniforms[ 4] = glGetUniformLocation( shader->GetObjID(), "SpecularPower" );
	uniforms[ 5] = glGetUniformLocation( shader->GetObjID(), "SpecularFactor" );
	uniforms[ 6] = glGetUniformLocation( shader->GetObjID(), "AmbientFactor" );
	uniforms[ 7] = glGetUniformLocation( shader->GetObjID(), "DiffuseFactor" );
	uniforms[ 8] = glGetUniformLocation( shader->GetObjID(), "SunDir" );
	uniforms[ 9] = glGetUniformLocation( shader->GetObjID(), "FresnelMin" );
	uniforms[10] = glGetUniformLocation( shader->GetObjID(), "FresnelMax" );
	uniforms[11] = glGetUniformLocation( shader->GetObjID(), "FresnelPower" );
	uniforms[12] = glGetUniformLocation( shader->GetObjID(), "ReflDistortion" );
	uniforms[13] = glGetUniformLocation( shader->GetObjID(), "BlurBase" );
	uniforms[14] = glGetUniformLocation( shader->GetObjID(), "BlurExponent" );
	uniforms[15] = glGetUniformLocation( shader->GetObjID(), "PerlinStartFreq" );
	uniforms[16] = glGetUniformLocation( shader->GetObjID(), "PerlinLacunarity" );
	uniforms[17] = glGetUniformLocation( shader->GetObjID(), "PerlinAmp" );
	uniforms[18] = glGetUniformLocation( shader->GetObjID(), "WindSpeed" );
	uniforms[19] = glGetUniformLocation( shader->GetObjID(), "shadowDensity" );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  UPDATE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::Update()
{
	if (!mapInfo->water.forceRendering && !readMap->HasVisibleWater())
		return;
	if (!wasVisibleLastFrame)
		return;

/*
	windndir *= 0.995f;
	windndir -= wind.GetCurrentDirection() * 0.005f;
	windStrength *= 0.9999f;
	windStrength += (smoothstep(0.0f, 12.0f, wind.GetCurrentStrength()) * 0.5f + 4.0f) * 0.0001f;
	windVec   = windndir * windStrength;
*/

	if (dynWaves) {
		UpdateDynWaves();
	}

	if (!shoreWaves) {
		return;
	}

	SCOPED_TIMER("BumpWater::Update (Coastmap)");

	if ((gs->frameNum % 10) == 0 && !heightmapUpdates.empty()) {
		UploadCoastline();
	}

	if ((gs->frameNum % 10) == 5 && !coastmapAtlasRects.empty()) {
		UpdateCoastmap();
	}
}


void CBumpWater::UpdateWater(CGame* game)
{
	if (!mapInfo->water.forceRendering && !readMap->HasVisibleWater())
		return;

#ifdef GLEW_ARB_occlusion_query2
	if (occlusionQuery && !wasVisibleLastFrame) {
		SCOPED_TIMER("BumpWater::UpdateWater (Occlcheck)");

		glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &occlusionQueryResult);
		if (occlusionQueryResult) {
			glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT, &occlusionQueryResult);

			wasVisibleLastFrame = !!occlusionQueryResult;

			if (!occlusionQueryResult) {
				return;
			}
		}

		wasVisibleLastFrame = true;
	}
#endif

	glPushAttrib(GL_FOG_BIT);
	if (refraction > 1) DrawRefraction(game);
	if (reflection > 0) DrawReflection(game);
	if (reflection || refraction) {
		FBO::Unbind();
		glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}
	glPopAttrib();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  SHOREWAVES/COASTMAP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CBumpWater::CoastAtlasRect::CoastAtlasRect(const SRectangle& rect)
{
	ix1 = std::max(rect.x1 - 15, 0);
	ix2 = std::min(rect.x2 + 15, gs->mapx);
	iy1 = std::max(rect.y1 - 15, 0);
	iy2 = std::min(rect.y2 + 15, gs->mapy);

	xsize = ix2 - ix1;
	ysize = iy2 - iy1;

	x1 = (ix1 + 0.5f) / (float)gs->mapx;
	x2 = (ix2 + 0.5f) / (float)gs->mapx;
	y1 = (iy1 + 0.5f) / (float)gs->mapy;
	y2 = (iy2 + 0.5f) / (float)gs->mapy;
	tx1 = tx2 = ty1 = ty2 = 0.f;
	isCoastline = true;
}

void CBumpWater::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	if (!shoreWaves || !readMap->HasVisibleWater())
		return;

	heightmapUpdates.push_back(rect);
}


void CBumpWater::UploadCoastline(const bool forceFull)
{
	//! optimize update area (merge overlapping areas etc.)
	heightmapUpdates.Optimize();

	//! limit the to be updated areas
	unsigned int currentPixels = 0;

	//! select the to be updated areas
	while (!heightmapUpdates.empty()) {
		SRectangle& cuRect1 = heightmapUpdates.front();

		if ((currentPixels + cuRect1.GetArea() <= 512 * 512) || forceFull) {
			currentPixels += cuRect1.GetArea();
			coastmapAtlasRects.push_back(cuRect1);
			heightmapUpdates.pop_front();
		} else {
			break;
		}
	}


	//! create a texture atlas for the to-be-updated areas
	CTextureAtlas atlas;
	atlas.SetFreeTexture(false);

	const float* heightMap = (gs->frameNum > 0) ? readMap->GetCornerHeightMapUnsynced() : readMap->GetCornerHeightMapSynced();

	for (size_t i = 0; i < coastmapAtlasRects.size(); i++) {
		CoastAtlasRect& caRect = coastmapAtlasRects[i];

		unsigned int a = 0;
		unsigned char* texpixels = (unsigned char*) atlas.AddTex(IntToString(i), caRect.xsize, caRect.ysize);

		for (int y = 0; y < caRect.ysize; ++y) {
			const int yindex  = (y + caRect.iy1) * (gs->mapx+1) + caRect.ix1;
			const int yindex2 = y * caRect.xsize;

			for (int x = 0; x < caRect.xsize; ++x) {
				const int index  = yindex + x;
				const int index2 = (yindex2 + x) << 2;
				const float& height = heightMap[index];

				texpixels[index2    ] = (height > 10.0f)? 255 : 0; //! isground
				texpixels[index2 + 1] = (height >  0.0f)? 255 : 0; //! coastdist
				texpixels[index2 + 2] = (height <  0.0f)? CReadMap::EncodeHeight(height) : 255; //! waterdepth
				texpixels[index2 + 3] = 0;
				a += (height > 0.0f)? 1: 0;
			}
		}

		if (a == 0 || a == caRect.ysize * caRect.xsize) {
			caRect.isCoastline = false;
		}
	}

	//! create the texture atlas
	if (!atlas.Finalize()) {
		coastmapAtlasRects.clear();
		return;
	}

	coastUpdateTexture = atlas.GetTexID();
	atlasX = (atlas.GetSize()).x;
	atlasY = (atlas.GetSize()).y;

	//! save the area positions in the texture atlas
	for (size_t i = 0; i < coastmapAtlasRects.size(); i++) {
		CoastAtlasRect& r = coastmapAtlasRects[i];
		const AtlasedTexture& tex = atlas.GetTexture(IntToString(i));
		r.tx1 = tex.xstart;
		r.tx2 = tex.xend;
		r.ty1 = tex.ystart;
		r.ty2 = tex.yend;
	}
}


void CBumpWater::UpdateCoastmap()
{
	coastFBO.Bind();
	blurShader->Enable();

	glDisable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, coastUpdateTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, coastTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);

	glViewport(0, 0, gs->mapx, gs->mapy);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
	glMultiTexCoord2i(GL_TEXTURE2, 0, 0);

	glBegin(GL_QUADS);
	for (size_t i = 0; i < coastmapAtlasRects.size(); i++) {
		const CoastAtlasRect& r = coastmapAtlasRects[i];
		glTexCoord4f(r.tx1, r.ty1, 0.0f, 0.0f); glVertex2f(r.x1, r.y1);
		glTexCoord4f(r.tx1, r.ty2, 0.0f, 1.0f); glVertex2f(r.x1, r.y2);
		glTexCoord4f(r.tx2, r.ty2, 1.0f, 1.0f); glVertex2f(r.x2, r.y2);
		glTexCoord4f(r.tx2, r.ty1, 1.0f, 0.0f); glVertex2f(r.x2, r.y1);
	}
	glEnd();

	if (atlasX > 0 && atlasY > 0) {
		int n = 0;
		for (int i = 0; i < 5; ++i) {
			coastFBO.AttachTexture(coastUpdateTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
			glViewport(0, 0, atlasX, atlasY);
			glMultiTexCoord2i(GL_TEXTURE2, 1, ++n);

			glBegin(GL_QUADS);
			for (size_t j = 0; j < coastmapAtlasRects.size(); j++) {
				const CoastAtlasRect& r = coastmapAtlasRects[j];
				if (!r.isCoastline) continue;
				glTexCoord4f(r.x1, r.y1, 0.0f, 0.0f); glVertex2f(r.tx1, r.ty1);
				glTexCoord4f(r.x1, r.y2, 0.0f, 1.0f); glVertex2f(r.tx1, r.ty2);
				glTexCoord4f(r.x2, r.y2, 1.0f, 1.0f); glVertex2f(r.tx2, r.ty2);
				glTexCoord4f(r.x2, r.y1, 1.0f, 0.0f); glVertex2f(r.tx2, r.ty1);
			}
			glEnd();

			coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
			glViewport(0, 0, gs->mapx, gs->mapy);
			glMultiTexCoord2i(GL_TEXTURE2, 0, ++n);

			glBegin(GL_QUADS);
			for (size_t j = 0; j < coastmapAtlasRects.size(); j++) {
				const CoastAtlasRect& r = coastmapAtlasRects[j];
				if (!r.isCoastline) continue;
				glTexCoord4f(r.tx1, r.ty1, 0.0f, 0.0f); glVertex2f(r.x1, r.y1);
				glTexCoord4f(r.tx1, r.ty2, 0.0f, 1.0f); glVertex2f(r.x1, r.y2);
				glTexCoord4f(r.tx2, r.ty2, 1.0f, 1.0f); glVertex2f(r.x2, r.y2);
				glTexCoord4f(r.tx2, r.ty1, 1.0f, 0.0f); glVertex2f(r.x2, r.y1);
			}
			glEnd();
		}
	}

	//glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

	blurShader->Disable();
	coastFBO.Detach(GL_COLOR_ATTACHMENT1_EXT);
	//coastFBO.Unbind();

	//! generate mipmaps
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, coastTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glGenerateMipmapEXT(GL_TEXTURE_2D);

	//! delete UpdateAtlas
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &coastUpdateTexture);
	coastmapAtlasRects.clear();

	//glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glActiveTexture(GL_TEXTURE0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  DYNAMIC WAVES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::UpdateDynWaves(const bool initialize)
{
	if (!dynWaves || !dynWavesFBO.IsValid()) {
		return;
	}

	static const unsigned char tiles  = mapInfo->water.numTiles; //! (numTiles <= 16)
	static const unsigned char ntiles = mapInfo->water.numTiles * mapInfo->water.numTiles;
	static const float tilesize = 1.0f/mapInfo->water.numTiles;

	const int f = (gs->frameNum+1) % 60;

	if (f == 0) {
		for (unsigned char i = 0; i < ntiles; ++i) {
			do {
				tileOffsets[i] = (unsigned char)(gu->RandFloat()*ntiles);
			} while (tileOffsets[i] == i);
		}
	}

	dynWavesFBO.Bind();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, normalTexture2);
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	glBlendColor(1.0f, 1.0f, 1.0f, (initialize) ? 1.0f : (f + 1)/600.0f );
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, normalTextureX, normalTextureY);
	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glLoadIdentity();

	glBegin(GL_QUADS);
		unsigned char offset, tx, ty;
		for (unsigned char y = 0; y < tiles; ++y) {
			for (unsigned char x = 0; x < tiles; ++x) {
				offset = tileOffsets[y * tiles + x];
				tx = offset % tiles;
				ty = (offset - tx)/tiles;
				glTexCoord2f(     tx * tilesize,     ty * tilesize ); glVertex2f(     x * tilesize,     y * tilesize );
				glTexCoord2f(     tx * tilesize, (ty+1) * tilesize ); glVertex2f(     x * tilesize, (y+1) * tilesize );
				glTexCoord2f( (tx+1) * tilesize, (ty+1) * tilesize ); glVertex2f( (x+1) * tilesize, (y+1) * tilesize );
				glTexCoord2f( (tx+1) * tilesize,     ty * tilesize ); glVertex2f( (x+1) * tilesize,     y * tilesize );
			}
		}
	glEnd();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPopMatrix();
	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	dynWavesFBO.Unbind();

	glBindTexture(GL_TEXTURE_2D, normalTexture);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  DRAW FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::SetUniforms()
{
	glUniform4f( uniforms[ 0], mapInfo->water.surfaceColor.x*0.4, mapInfo->water.surfaceColor.y*0.4, mapInfo->water.surfaceColor.z*0.4, mapInfo->water.surfaceAlpha);
	glUniform4f( uniforms[ 1], mapInfo->water.planeColor.x*0.4, mapInfo->water.planeColor.y*0.4, mapInfo->water.planeColor.z*0.4, mapInfo->water.surfaceAlpha);
	glUniform3f( uniforms[ 2], mapInfo->water.diffuseColor.x, mapInfo->water.diffuseColor.y, mapInfo->water.diffuseColor.z);
	glUniform3f( uniforms[ 3], mapInfo->water.specularColor.x, mapInfo->water.specularColor.y, mapInfo->water.specularColor.z);
	glUniform1f( uniforms[ 4], mapInfo->water.specularPower);
	glUniform1f( uniforms[ 5], mapInfo->water.specularFactor);
	glUniform1f( uniforms[ 6], mapInfo->water.ambientFactor);
	glUniform1f( uniforms[ 7], mapInfo->water.diffuseFactor * 15.0f);
	glUniform3fv(uniforms[ 8], 1, &sky->GetLight()->GetLightDir().x);
	glUniform1f( uniforms[ 9], mapInfo->water.fresnelMin);
	glUniform1f( uniforms[10], mapInfo->water.fresnelMax);
	glUniform1f( uniforms[11], mapInfo->water.fresnelPower);
	glUniform1f( uniforms[12], mapInfo->water.reflDistortion);
	glUniform2f( uniforms[13], 0.0f, mapInfo->water.blurBase / globalRendering->viewSizeY);
	glUniform1f( uniforms[14], mapInfo->water.blurExponent);
	glUniform1f( uniforms[15], mapInfo->water.perlinStartFreq);
	glUniform1f( uniforms[16], mapInfo->water.perlinLacunarity);
	glUniform1f( uniforms[17], mapInfo->water.perlinAmplitude);
	glUniform1f( uniforms[18], mapInfo->water.windSpeed);
	glUniform1f( uniforms[19], sky->GetLight()->GetGroundShadowDensity());
}


void CBumpWater::Draw()
{
	if (!occlusionQueryResult || (!mapInfo->water.forceRendering && !readMap->HasVisibleWater()))
		return;

#ifdef GLEW_ARB_occlusion_query2
	if (occlusionQuery) {
		glBeginConditionalRenderNV(occlusionQuery, GL_QUERY_BY_REGION_WAIT_NV);
		glBeginQuery(GL_ANY_SAMPLES_PASSED,occlusionQuery);
	}
#endif

	if (refraction == 1) {
		//! _SCREENCOPY_ REFRACT TEXTURE
		glBindTexture(target, refractTexture);
		glCopyTexSubImage2D(target, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	if (depthCopy) {
		//! _SCREENCOPY_ DEPTH TEXTURE
		glBindTexture(target, depthTexture);
		glCopyTexSubImage2D(target, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	glDisable(GL_ALPHA_TEST);
	if (refraction < 2) {
		glDepthMask(GL_FALSE);
	}
	if (refraction > 0) {
		glDisable(GL_BLEND);
	}

	const CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	waterShader->SetFlag("opt_shadows", (shadowHandler && shadowHandler->shadowsLoaded));
	waterShader->SetFlag("opt_infotex", gd->DrawExtraTex());

	waterShader->Enable();
	waterShader->SetUniform3fv(0, &camera->GetPos()[0]);
	waterShader->SetUniform1f(1, (gs->frameNum + globalRendering->timeOffset) / 15000.0f);

	if (shadowHandler && shadowHandler->shadowsLoaded) {
		waterShader->SetUniformMatrix4fv(13, false, &shadowHandler->shadowMatrix.m[0]);

		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, (shadowHandler && shadowHandler->shadowsLoaded) ? shadowHandler->shadowTexture : 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	}

	const int causticTexNum = (gs->frameNum % caustTextures.size());
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, caustTextures[causticTexNum]);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, foamTexture);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(target,        refractTexture);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, coastTexture);
	glActiveTexture(GL_TEXTURE7); glBindTexture(target,        depthTexture);
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, waveRandTexture);
	//glActiveTexture(GL_TEXTURE9); see above
	glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, gd->GetActiveInfoTexture());
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, normalTexture);

	if (useUniforms) {
		SetUniforms();
	}

	glMultiTexCoord2f(GL_TEXTURE1, windVec.x, windVec.z);
	glCallList(displayList);

	waterShader->Disable();

	if (shadowHandler && shadowHandler->shadowsLoaded) {
		glActiveTexture(GL_TEXTURE9); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glActiveTexture(GL_TEXTURE0);
	}
	if (refraction < 2) {
		glDepthMask(GL_TRUE);
	}
	if (refraction > 0) {
		glEnable(GL_BLEND);
	}

#ifdef GLEW_ARB_occlusion_query2
	if (occlusionQuery) {
		glEndQuery(GL_ANY_SAMPLES_PASSED);
		glEndConditionalRenderNV();
	}
#endif
}


void CBumpWater::DrawRefraction(CGame* game)
{
	//! _RENDER_ REFRACTION TEXTURE
	refractFBO.Bind();

	camera->Update();

	game->SetDrawMode(CGame::gameRefractionDraw);
	glViewport(0, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//sky->Draw();
	glDisable(GL_FOG); // fog has overground settings, if at all we should add special underwater settings

	const float3 oldsun = unitDrawer->unitSunColor;
	const float3 oldambient = unitDrawer->unitAmbientColor;

	unitDrawer->unitSunColor *= float3(0.5f, 0.7f, 0.9f);
	unitDrawer->unitAmbientColor *= float3(0.6f, 0.8f, 1.0f);

	drawRefraction = true;

	glEnable(GL_CLIP_PLANE2);
	const double plane[4] = {0, -1, 0, 5};
	glClipPlane(GL_CLIP_PLANE2, plane);

	// opaque
	readMap->GetGroundDrawer()->Draw(DrawPass::WaterRefraction);
	unitDrawer->Draw(false, true);
	featureDrawer->Draw();

	// transparent stuff
	unitDrawer->DrawCloakedUnits();
	featureDrawer->DrawFadeFeatures();
	projectileDrawer->Draw(false, true);
	eventHandler.DrawWorldRefraction();

	glDisable(GL_CLIP_PLANE2);
	game->SetDrawMode(CGame::gameNormalDraw);
	drawRefraction = false;

	glEnable(GL_FOG);

	unitDrawer->unitSunColor=oldsun;
	unitDrawer->unitAmbientColor=oldambient;
}


void CBumpWater::DrawReflection(CGame* game)
{
	//! CREATE REFLECTION TEXTURE
	reflectFBO.Bind();

//	CCamera* realCam = camera;
//	camera = new CCamera(*realCam);
	char realCam[sizeof(CCamera)];
	new (realCam) CCamera(*camera); // anti-crash workaround for multithreading

	camera->forward.y *= -1.0f;
	camera->SetPos(camera->GetPos() * float3(1.0f, -1.0f, 1.0f));
	camera->Update();

	game->SetDrawMode(CGame::gameReflectionDraw);
	glViewport(0, 0, reflTexSize, reflTexSize);
	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	sky->Draw();

	glEnable(GL_CLIP_PLANE2);
	const double plane[4] = {0, 1, 0, 1};
	glClipPlane(GL_CLIP_PLANE2, plane);
	drawReflection = true;

	// opaque
	if (reflection > 1) {
		readMap->GetGroundDrawer()->Draw(DrawPass::WaterReflection);
	}
	unitDrawer->Draw(true);
	featureDrawer->Draw();

	// transparent
	unitDrawer->DrawCloakedUnits(true);
	featureDrawer->DrawFadeFeatures(true);
	projectileDrawer->Draw(true);
	eventHandler.DrawWorldReflection();

	game->SetDrawMode(CGame::gameNormalDraw);
	drawReflection = false;
	glDisable(GL_CLIP_PLANE2);

//	delete camera;
//	camera = realCam;
	camera->~CCamera();
	new (camera) CCamera(*(reinterpret_cast<CCamera*>(realCam)));
	reinterpret_cast<CCamera*>(realCam)->~CCamera();
	camera->Update();
}


void CBumpWater::OcclusionQuery()
{
	if (!occlusionQuery || (!mapInfo->water.forceRendering && !readMap->HasVisibleWater()))
		return;

#ifdef GLEW_ARB_occlusion_query2
	glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &occlusionQueryResult);

	if (occlusionQueryResult || !wasVisibleLastFrame) {
		glGetQueryObjectuiv(occlusionQuery,GL_QUERY_RESULT, &occlusionQueryResult);
		wasVisibleLastFrame = !!occlusionQueryResult;
	} else {
		occlusionQueryResult = true;
		wasVisibleLastFrame = true;
	}

	if (!wasVisibleLastFrame) {
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glEnable(GL_DEPTH_TEST);

		glPushMatrix();
			glTranslatef(0.0, 10.0, 0.0);
			glBeginQuery(GL_ANY_SAMPLES_PASSED, occlusionQuery);
				glCallList(displayList);
			glEndQuery(GL_ANY_SAMPLES_PASSED);
		glPopMatrix();

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
	}
#endif
}
