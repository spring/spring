/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief extended bump-mapping water shader
 */

#include "BumpWater.h"

#include "ISky.h"
#include "SunLighting.h"
#include "WaterRendering.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/Wind.h"
#include "System/bitops.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FastMath.h"
#include "System/SpringMath.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/SpringFormat.h"
#include "System/StringUtil.h"

using std::string;
using std::vector;
using std::min;
using std::max;

CONFIG(int, BumpWaterTexSizeReflection).defaultValue(512).headlessValue(32).minimumValue(32).description("Sets the size of the framebuffer texture used to store the reflection in Bumpmapped water.");
CONFIG(int, BumpWaterReflection).defaultValue(1).headlessValue(0).minimumValue(0).maximumValue(2).description("Determines the amount of objects reflected in Bumpmapped water.\n0:=off, 1:=fast (skip terrain), 2:=full");
CONFIG(int, BumpWaterRefraction).defaultValue(1).headlessValue(0).minimumValue(0).maximumValue(1).description("Determines the method of refraction with Bumpmapped water.\n0:=off, 1:=screencopy, 2:=own rendering cycle (disabled)");
CONFIG(float, BumpWaterAnisotropy).defaultValue(0.0f).minimumValue(0.0f);
CONFIG(bool, BumpWaterUseDepthTexture).defaultValue(true).headlessValue(false);
CONFIG(int, BumpWaterDepthBits).defaultValue(24).minimumValue(16).maximumValue(32);
CONFIG(bool, BumpWaterBlurReflection).defaultValue(false);
CONFIG(bool, BumpWaterShoreWaves).defaultValue(true).headlessValue(false).safemodeValue(false).description("Enables rendering of shorewaves.");
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
	str += spring::format(string("#define ") + name + " vec4(%.12f,%.12f,%.12f,%.12f)\n", x, y, z, w);
}

static void GLSLDefineConstf4(string& str, const string& name, const float3& v, float alpha)
{
	str += spring::format(string("#define ") + name + " vec4(%.12f,%.12f,%.12f,%.12f)\n", v.x, v.y, v.z, alpha);
}

static void GLSLDefineConstf3(string& str, const string& name, const float3& v)
{
	str += spring::format(string("#define ") + name + " vec3(%.12f,%.12f,%.12f)\n", v.x, v.y, v.z);
}

static void GLSLDefineConstf2(string& str, const string& name, float x, float y)
{
	str += spring::format(string("#define ") + name + " vec2(%.12f,%.12f)\n", x, y);
}

static void GLSLDefineConstf1(string& str, const string& name, float x)
{
	str += spring::format(string("#define ") + name + " %.12f\n", x);
}


static GLuint LoadTexture(const string& filename, const float anisotropy = 0.0f, int* sizeX = nullptr, int* sizeY = nullptr)
{
	CBitmap bm;

	if (!bm.Load(filename))
		throw content_error("[" LOG_SECTION_BUMP_WATER "] Could not load texture from file " + filename);

	const unsigned int texID = bm.CreateMipMapTexture(anisotropy);

	if (sizeY != nullptr) {
		*sizeX = bm.xsize;
		*sizeY = bm.ysize;
	}

	return texID;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// (DE-)CONSTRUCTOR
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CBumpWater::CBumpWater()
	: CEventClient("[CBumpWater]", 271923, false)
	, screenTextureX(globalRendering->viewSizeX)
	, screenTextureY(globalRendering->viewSizeY)
	, screenCopyTarget(GL_TEXTURE_RECTANGLE)
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

	blurRefl     = configHandler->GetBool("BumpWaterBlurReflection");
	shoreWaves   = (configHandler->GetBool("BumpWaterShoreWaves")) && waterRendering->shoreWaves;
	endlessOcean = (configHandler->GetBool("BumpWaterEndlessOcean")) && waterRendering->hasWaterPlane;
	dynWaves     = (configHandler->GetBool("BumpWaterDynamicWaves")) && (waterRendering->numTiles > 1);
	useUniforms  = (configHandler->GetBool("BumpWaterUseUniforms"));

	if ((depthBits == 24) && !globalRendering->support24bitDepthBuffer)
		depthBits = 16;
	if (!readMap->HasVisibleWater() && !waterRendering->forceRendering)
		endlessOcean = false;

	// LOAD TEXTURES
	foamTexture   = LoadTexture(waterRendering->foamTexture);
	normalTexture = LoadTexture(waterRendering->normalTexture, anisotropy, &normalTextureX, &normalTextureY);

	// caustic textures
	if (waterRendering->causticTextures.empty())
		throw content_error("[" LOG_SECTION_BUMP_WATER "] no caustic textures");

	for (const std::string& causticName: waterRendering->causticTextures) {
		caustTextures.push_back(LoadTexture(causticName));
	}

	// CHECK SHOREWAVES TEXTURE SIZE
	if (shoreWaves) {
		GLint maxw, maxh;
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA16F, 4096, 4096, 0, GL_RGBA, GL_FLOAT, nullptr);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &maxw);
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &maxh);
		if (mapDims.mapx > maxw || mapDims.mapy > maxh) {
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
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mapDims.mapx, mapDims.mapy, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, mapDims.mapx, mapDims.mapy, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		//glGenerateMipmap(GL_TEXTURE_2D);


		{
			blurShader = shaderHandler->CreateProgramObject("[BumpWater]", "CoastBlurShader");
			blurShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterCoastBlurVS.glsl", "", GL_VERTEX_SHADER));
			blurShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterCoastBlurFS.glsl", "", GL_FRAGMENT_SHADER));
			blurShader->Link();

			if (!blurShader->IsValid()) {
				const char* fmt = "shorewaves-shader compilation error: %s";
				const char* log = (blurShader->GetLog()).c_str();

				LOG_L(L_ERROR, fmt, log);

				// string size is limited with content_error()
				throw content_error(string("[" LOG_SECTION_BUMP_WATER "] shorewaves-shader compilation error!"));
			}

			blurShader->SetUniformLocation("tex0"); // idx 0
			blurShader->SetUniformLocation("tex1"); // idx 1
			blurShader->SetUniformLocation("args"); // idx 2
			blurShader->Enable();
			blurShader->SetUniform1i(0, 0);
			blurShader->SetUniform1i(1, 1);
			blurShader->SetUniform2i(2, 0, 0);
			blurShader->SetUniformMatrix4x4<float>("uViewMat", false, CMatrix44f::Identity());
			blurShader->SetUniformMatrix4x4<float>("uProjMat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
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
		coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);

		if ((shoreWaves = coastFBO.CheckStatus("BUMPWATER(Coastmap)"))) {
			// initialize texture
			glAttribStatePtr->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);

			// fill with current heightmap/coastmap
			UnsyncedHeightMapUpdate(SRectangle(0, 0, mapDims.mapx, mapDims.mapy));
			UploadCoastline(true);
			UpdateCoastmap(true);

			eventHandler.InsertEvent(this, "UnsyncedHeightMapUpdate");
		}

		//coastFBO.Unbind(); // gets done below
	}


	// CREATE TEXTURES
	if (refraction > 0) {
		// CREATE REFRACTION TEXTURE
		glGenTextures(1, &refractTexture);
		glBindTexture(screenCopyTarget, refractTexture);
		glTexParameteri(screenCopyTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(screenCopyTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(screenCopyTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(screenCopyTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(screenCopyTarget, 0, GL_RGBA8, screenTextureX, screenTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	if (reflection > 0) {
		// CREATE REFLECTION TEXTURE
		glGenTextures(1, &reflectTexture);
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}

	if (depthCopy) {
		// CREATE DEPTH TEXTURE
		glGenTextures(1, &depthTexture);
		glBindTexture(screenCopyTarget, depthTexture);
		glTexParameteri(screenCopyTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(screenCopyTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(screenCopyTarget, 0, GL_DEPTH_COMPONENT32, screenTextureX, screenTextureY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	if (dynWaves) {
		// SETUP DYNAMIC WAVES
		std::memset(tileOffsets, 0, sizeof(tileOffsets));

		normalTexture2 = normalTexture;
		glBindTexture(GL_TEXTURE_2D, normalTexture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glGenTextures(1, &normalTexture);
		glBindTexture(GL_TEXTURE_2D, normalTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (anisotropy > 0.0f)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, normalTextureX, normalTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	// CREATE FBOs
	{
		GLuint depthRBOFormat = GL_DEPTH_COMPONENT;
		switch (depthBits) {
			case 16: depthRBOFormat = GL_DEPTH_COMPONENT16; break;
			case 24: depthRBOFormat = GL_DEPTH_COMPONENT24; break;
			case 32: depthRBOFormat = GL_DEPTH_COMPONENT32; break;
		}

		if (reflection > 0) {
			reflectFBO.Bind();
			reflectFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT, depthRBOFormat, reflTexSize, reflTexSize);
			reflectFBO.AttachTexture(reflectTexture);
			if (!reflectFBO.CheckStatus("BUMPWATER(reflection)"))
				reflection = 0;
		}

		if (refraction > 0) {
			refractFBO.Bind();
			refractFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT, depthRBOFormat, screenTextureX, screenTextureY);
			refractFBO.AttachTexture(refractTexture, screenCopyTarget);
			if (!refractFBO.CheckStatus("BUMPWATER(refraction)"))
				refraction = 0;
		}

		if (dynWaves) {
			dynWavesFBO.reloadOnAltTab = true;
			dynWavesFBO.Bind();
			dynWavesFBO.AttachTexture(normalTexture);
			if (dynWavesFBO.CheckStatus("BUMPWATER(DynWaves)"))
				UpdateDynWaves(true); // initialize
		}

		FBO::Unbind();
	}


	/*
	 * DEFINE SOME SHADER RUNTIME CONSTANTS
	 * (GLSL compiler can not optimize uniforms)
	 */
	string definitions = "#version 410 core\n";

	if (reflection > 0) definitions += "#define opt_reflection\n";
	if (refraction > 0) definitions += "#define opt_refraction\n";
	if (shoreWaves    ) definitions += "#define opt_shorewaves\n";
	if (depthCopy     ) definitions += "#define opt_depth\n";
	if (blurRefl      ) definitions += "#define opt_blurreflection\n";
	if (endlessOcean  ) definitions += "#define opt_endlessocean\n";

	if (screenCopyTarget == GL_TEXTURE_RECTANGLE)
		definitions += "#define opt_texrect\n";

	GLSLDefineConstf3(definitions, "MapMid",                    float3(mapDims.mapx * SQUARE_SIZE * 0.5f, 0.0f, mapDims.mapy * SQUARE_SIZE * 0.5f));
	GLSLDefineConstf2(definitions, "ScreenInverse",             1.0f / globalRendering->viewSizeX, 1.0f / globalRendering->viewSizeY);
	GLSLDefineConstf2(definitions, "ScreenTextureSizeInverse",  1.0f / screenTextureX, 1.0f / screenTextureY);
	GLSLDefineConstf2(definitions, "ViewPos",                   globalRendering->viewPosX, globalRendering->viewPosY);

	if (useUniforms) {
		SetupUniforms(definitions);
	} else {
		GLSLDefineConstf4(definitions, "SurfaceColor",   waterRendering->surfaceColor * 0.4f, waterRendering->surfaceAlpha );
		GLSLDefineConstf4(definitions, "PlaneColor",     waterRendering->planeColor * 0.4f, waterRendering->surfaceAlpha );
		GLSLDefineConstf3(definitions, "DiffuseColor",   waterRendering->diffuseColor);
		GLSLDefineConstf3(definitions, "SpecularColor",  waterRendering->specularColor);
		GLSLDefineConstf1(definitions, "SpecularPower",  waterRendering->specularPower);
		GLSLDefineConstf1(definitions, "SpecularFactor", waterRendering->specularFactor);
		GLSLDefineConstf1(definitions, "AmbientFactor",  waterRendering->ambientFactor);
		GLSLDefineConstf1(definitions, "DiffuseFactor",  waterRendering->diffuseFactor * 15.0f);
		GLSLDefineConstf3(definitions, "SunDir",         sky->GetLight()->GetLightDir()); // FIXME: not a constant
		GLSLDefineConstf1(definitions, "FresnelMin",     waterRendering->fresnelMin);
		GLSLDefineConstf1(definitions, "FresnelMax",     waterRendering->fresnelMax);
		GLSLDefineConstf1(definitions, "FresnelPower",   waterRendering->fresnelPower);
		GLSLDefineConstf1(definitions, "ReflDistortion", waterRendering->reflDistortion);
		GLSLDefineConstf2(definitions, "BlurBase",       0.0f, waterRendering->blurBase / globalRendering->viewSizeY);
		GLSLDefineConstf1(definitions, "BlurExponent",   waterRendering->blurExponent);
		GLSLDefineConstf1(definitions, "PerlinStartFreq",  waterRendering->perlinStartFreq);
		GLSLDefineConstf1(definitions, "PerlinLacunarity", waterRendering->perlinLacunarity);
		GLSLDefineConstf1(definitions, "PerlinAmp",        waterRendering->perlinAmplitude);
		GLSLDefineConstf1(definitions, "WindSpeed",        waterRendering->windSpeed);
		GLSLDefineConstf1(definitions, "shadowDensity",  sunLighting->groundShadowDensity);
	}

	{
		const int mapX = mapDims.mapx  * SQUARE_SIZE;
		const int mapZ = mapDims.mapy * SQUARE_SIZE;
		const float shadingX = (float)mapDims.mapx / mapDims.pwr2mapx;
		const float shadingZ = (float)mapDims.mapy / mapDims.pwr2mapy;
		const float scaleX = (mapX > mapZ) ? (mapDims.mapy/64)/16.0f * (float)mapX/mapZ : (mapDims.mapx/64)/16.0f;
		const float scaleZ = (mapX > mapZ) ? (mapDims.mapy/64)/16.0f : (mapDims.mapx/64)/16.0f * (float)mapZ/mapX;
		GLSLDefineConst4f(definitions, "TexGenPlane", 1.0f/mapX, 1.0f/mapZ, scaleX/mapX, scaleZ/mapZ);
		GLSLDefineConst4f(definitions, "ShadingPlane", shadingX/mapX, shadingZ/mapZ, shadingX, shadingZ);
	}



	// LOAD SHADERS
	{
		waterShader = shaderHandler->CreateProgramObject("[BumpWater]", "WaterShader");
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterVS.glsl", definitions, GL_VERTEX_SHADER));
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/bumpWaterFS.glsl", definitions, GL_FRAGMENT_SHADER));
		waterShader->Link();
		waterShader->SetUniformLocation("eyePos");        // idx  0
		waterShader->SetUniformLocation("frame");         // idx  1
		waterShader->SetUniformLocation("normalmap");     // idx  2, texunit 0
		waterShader->SetUniformLocation("heightmap");     // idx  3, texunit 1
		waterShader->SetUniformLocation("caustic");       // idx  4, texunit 2
		waterShader->SetUniformLocation("foam");          // idx  5, texunit 3
		waterShader->SetUniformLocation("reflection");    // idx  6, texunit 4
		waterShader->SetUniformLocation("refraction");    // idx  7, texunit 5
		waterShader->SetUniformLocation("depthmap");      // idx  8, texunit 7
		waterShader->SetUniformLocation("coastmap");      // idx  9, texunit 6
		waterShader->SetUniformLocation("waverand");      // idx 10, texunit 8
		waterShader->SetUniformLocation("infotex");       // idx 11, texunit 10
		waterShader->SetUniformLocation("shadowmap");     // idx 12, texunit 9
		waterShader->SetUniformLocation("shadowMatrix");  // idx 13
		waterShader->SetUniformLocation("viewMatrix");    // idx 14
		waterShader->SetUniformLocation("projMatrix");    // idx 15

		waterShader->SetUniformLocation("windVector");    // idx 16
		waterShader->SetUniformLocation("fogColor");      // idx 17
		waterShader->SetUniformLocation("fogParams");     // idx 18
		waterShader->SetUniformLocation("gammaExponent"); // idx 19

		if (!waterShader->IsValid()) {
			const char* fmt = "water-shader compilation error: %s";
			const char* log = (waterShader->GetLog()).c_str();
			LOG_L(L_ERROR, fmt, log);
			throw content_error(string("[" LOG_SECTION_BUMP_WATER "] water-shader compilation error!"));
		}

		if (useUniforms)
			GetUniformLocations(waterShader);

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
		waterShader->SetUniform2f(16, 0.0f, 0.0f);
		waterShader->SetUniform3f(17, sky->fogColor.x, sky->fogColor.y, sky->fogColor.z);
		waterShader->SetUniform3f(18, sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist());
		waterShader->SetUniform1f(19, globalRendering->gammaExponent);
		waterShader->Disable();
		waterShader->Validate();

		if (!waterShader->IsValid()) {
			const char* fmt = "water-shader validation error: %s";
			const char* log = (waterShader->GetLog()).c_str();

			LOG_L(L_ERROR, fmt, log);
			throw content_error(string("[" LOG_SECTION_BUMP_WATER "] water-shader validation error!"));
		}
	}

	GenWaterPlaneBuffer(endlessOcean);
	GenOcclusionQuery(configHandler->GetBool("BumpWaterOcclusionQuery"));

	#if 0
	windDir = envResHandler.GetCurrentWindDir();
	windVec = windDir * (windStrength = (smoothstep(0.0f, 12.0f, envResHandler.GetCurrentWindStrength()) * 0.5f + 4.0f));
	#else
	windVec = guRNG.NextVector(0.0f) * mix(envResHandler.GetMinWindStrength(), envResHandler.GetMaxWindStrength(), guRNG.NextFloat());
	#endif
}


CBumpWater::~CBumpWater()
{
	if (reflectTexture != 0)
		glDeleteTextures(1, &reflectTexture);

	if (refractTexture != 0)
		glDeleteTextures(1, &refractTexture);

	if (depthCopy != 0)
		glDeleteTextures(1, &depthTexture);

	glDeleteTextures(1, &foamTexture);
	glDeleteTextures(1, &normalTexture);
	// never empty
	glDeleteTextures(caustTextures.size(), &caustTextures[0]);

	waterPlaneBuffer.Kill();

	if (shoreWaves) {
		glDeleteTextures(1, &coastTexture);
		glDeleteTextures(1, &waveRandTexture);
	}

	if (dynWaves)
		glDeleteTextures(1, &normalTexture2);

	if (occlusionQuery != 0)
		glDeleteQueries(1, &occlusionQuery);

	shaderHandler->ReleaseProgramObjects("[BumpWater]");
}



void CBumpWater::GenWaterPlaneBuffer(bool radial)
{
	std::vector<VA_TYPE_0> verts;

	verts.clear();
	verts.reserve(4 * (32 + 1) * 2);

	if (radial) {
		// FIXME: more or less copied from SMFGroundDrawer
		const float xsize = (mapDims.mapx * SQUARE_SIZE) >> 2;
		const float ysize = (mapDims.mapy * SQUARE_SIZE) >> 2;

		const float alphainc = math::TWOPI / 32.0f;
		const float size = std::min(xsize, ysize);

		float3 p;

		for (int n = 0; n < 4; ++n) {
			const float k = (n == 3)? 0.5f: 1.0f;

			const float r1 = (n    ) * (n    ) * size;
			const float r2 = (n + k) * (n + k) * size;

			for (float alpha = 0.0f; (alpha - math::TWOPI) < alphainc; alpha += alphainc) {
				p.x = r1 * fastmath::sin(alpha) + 2 * xsize;
				p.z = r1 * fastmath::cos(alpha) + 2 * ysize;
				verts.push_back({p});

				p.x = r2 * fastmath::sin(alpha) + 2 * xsize;
				p.z = r2 * fastmath::cos(alpha) + 2 * ysize;
				verts.push_back({p});
			}
		}
	} else {
		const int mapX = mapDims.mapx * SQUARE_SIZE;
		const int mapZ = mapDims.mapy * SQUARE_SIZE;

		for (int z = 0; z < 9; z++) {
			for (int x = 0; x < 9; x++) {
				const float3 v0{(x + 0) * (mapX / 9.0f), 0.0f, (z + 0) * (mapZ / 9.0f)};
				const float3 v1{(x + 0) * (mapX / 9.0f), 0.0f, (z + 1) * (mapZ / 9.0f)};
				const float3 v2{(x + 1) * (mapX / 9.0f), 0.0f, (z + 0) * (mapZ / 9.0f)};
				const float3 v3{(x + 1) * (mapX / 9.0f), 0.0f, (z + 1) * (mapZ / 9.0f)};

				verts.push_back({v0});
				verts.push_back({v1});
				verts.push_back({v2});

				verts.push_back({v1});
				verts.push_back({v3});
				verts.push_back({v2});
			}
		}
	}

	waterPlaneBuffer.Init();
	waterPlaneBuffer.Upload0(verts.size(), 0, verts.data(), nullptr);

	{
		char vsBuf[65536];
		char fsBuf[65536];

		const char* vsVars = "";
		const char* vsCode = "\tgl_Position = u_proj_mat * u_movi_mat * vec4(a_vertex_xyz, 1.0);\n";
		const char* fsVars = "";
		const char* fsCode = "\tf_color_rgba = vec4(1.0, 1.0, 1.0, 1.0);\n";

		GL::RenderDataBuffer::FormatShader0(vsBuf, vsBuf + sizeof(vsBuf), "", vsVars, vsCode, "VS");
		GL::RenderDataBuffer::FormatShader0(fsBuf, fsBuf + sizeof(fsBuf), "", fsVars, fsCode, "FS");

		Shader::GLSLShaderObject shaderObjs[2] = {{GL_VERTEX_SHADER, &vsBuf[0], ""}, {GL_FRAGMENT_SHADER, &fsBuf[0], ""}};
		Shader::IProgramObject* shaderProg = waterPlaneBuffer.CreateShader((sizeof(shaderObjs) / sizeof(shaderObjs[0])), 0, &shaderObjs[0], nullptr);

		shaderProg->Enable();
		shaderProg->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shaderProg->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::Identity());
		shaderProg->Disable();
	}
}

void CBumpWater::GenOcclusionQuery(bool enabled)
{
	occlusionQuery = 0;
	occlusionQueryResult = GL_TRUE;
	wasVisibleLastFrame = true;

	if (!enabled)
		return;

	GLint bitsSupported;
	glGetQueryiv(GL_ANY_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS, &bitsSupported);

	if (bitsSupported == 0)
		return;

	glGenQueries(1, &occlusionQuery);
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
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;
	if (!wasVisibleLastFrame)
		return;

	UpdateWind();

	if (dynWaves)
		UpdateDynWaves();

	if (shoreWaves) {
		if ((gs->frameNum % 10) == 0 && !heightmapUpdates.empty())
			UploadCoastline();

		if ((gs->frameNum % 10) == 5 && !coastmapAtlasRects.empty())
			UpdateCoastmap();
	}
}

void CBumpWater::UpdateWind()
{
	#if 0
	windStrength *= 0.9999f;
	windStrength += (smoothstep(0.0f, 12.0f, envResHandler.GetCurrentWindStrength()) * 0.5f + 4.0f) * 0.0001f;

	windDir *= 0.995f;
	windDir -= (envResHandler.GetCurrentWindDir() * 0.005f);
	windVec  = windDir * windStrength;
	#endif
}

void CBumpWater::UpdateWater(CGame* game)
{
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;


	if (occlusionQuery != 0 && !wasVisibleLastFrame) {
		glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &occlusionQueryResult);

		if (occlusionQueryResult) {
			glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT, &occlusionQueryResult);

			wasVisibleLastFrame = !!occlusionQueryResult;

			if (!occlusionQueryResult)
				return;
		}

		wasVisibleLastFrame = true;
	}


	if (refraction > 1) DrawRefraction(game);
	if (reflection > 0) DrawReflection(game);
	if (reflection || refraction) {
		FBO::Unbind();
		glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  SHOREWAVES/COASTMAP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CBumpWater::CoastAtlasRect::CoastAtlasRect(const SRectangle& rect)
{
	ix1 = std::max(rect.x1 - 15,            0);
	iy1 = std::max(rect.y1 - 15,            0);
	ix2 = std::min(rect.x2 + 15, mapDims.mapx);
	iy2 = std::min(rect.y2 + 15, mapDims.mapy);

	xsize = ix2 - ix1;
	ysize = iy2 - iy1;

	x1 = (ix1 + 0.5f) / (float)mapDims.mapx;
	x2 = (ix2 + 0.5f) / (float)mapDims.mapx;
	y1 = (iy1 + 0.5f) / (float)mapDims.mapy;
	y2 = (iy2 + 0.5f) / (float)mapDims.mapy;
	tx1 = tx2 = ty1 = ty2 = 0.0f;
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
	// optimize update area (merge overlapping areas etc.)
	heightmapUpdates.Process();

	// limit the to be updated areas
	unsigned int currentPixels = 0;
	unsigned int numCoastRects = 0;

	// select the to be updated areas
	while (!heightmapUpdates.empty()) {
		const SRectangle& cuRect1 = heightmapUpdates.front();

		if ((currentPixels + cuRect1.GetArea() <= 512 * 512) || forceFull) {
			currentPixels += cuRect1.GetArea();
			coastmapAtlasRects.emplace_back(cuRect1);
			heightmapUpdates.pop_front();
			continue;
		}

		break;
	}


	// create a texture atlas for the to-be-updated areas
	CTextureAtlas atlas;
	atlas.SetFreeTexture(false);

	const float* heightMap = (!gs->PreSimFrame()) ? readMap->GetCornerHeightMapUnsynced() : readMap->GetCornerHeightMapSynced();

	for (size_t i = 0; i < coastmapAtlasRects.size(); i++) {
		CoastAtlasRect& caRect = coastmapAtlasRects[i];

		unsigned int a = 0;
		unsigned char* texpixels = (unsigned char*) atlas.AddGetTex(IntToString(i), caRect.xsize, caRect.ysize);

		for (int y = 0; y < caRect.ysize; ++y) {
			const int yindex  = (y + caRect.iy1) * mapDims.mapxp1 + caRect.ix1;
			const int yindex2 = y * caRect.xsize;

			for (int x = 0; x < caRect.xsize; ++x) {
				const int index  = yindex + x;
				const int index2 = (yindex2 + x) << 2;
				const float& height = heightMap[index];

				texpixels[index2    ] = (height > 10.0f)? 255 : 0; // isground
				texpixels[index2 + 1] = (height >  0.0f)? 255 : 0; // coastdist
				texpixels[index2 + 2] = (height <  0.0f)? CReadMap::EncodeHeight(height) : 255; // waterdepth
				texpixels[index2 + 3] = 0;
				a += (height > 0.0f);
			}
		}

		numCoastRects += (caRect.isCoastline = (a != 0 && a != (caRect.ysize * caRect.xsize)));
	}

	// create the texture atlas only if any coastal regions exist
	if (numCoastRects == 0 || !atlas.Finalize()) {
		coastmapAtlasRects.clear();
		return;
	}

	coastUpdateTexture = atlas.GetTexID();

	atlasSizeX = (atlas.GetSize()).x;
	atlasSizeY = (atlas.GetSize()).y;

	// save the area positions in the texture atlas
	for (size_t i = 0; i < coastmapAtlasRects.size(); i++) {
		CoastAtlasRect& r = coastmapAtlasRects[i];
		const AtlasedTexture& tex = atlas.GetTexture(IntToString(i));
		r.tx1 = tex.xstart;
		r.tx2 = tex.xend;
		r.ty1 = tex.ystart;
		r.ty2 = tex.yend;
	}
}


void CBumpWater::UpdateCoastmap(const bool initialize)
{
	coastFBO.Bind();

	blurShader->Enable();
	glAttribStatePtr->PushBits(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->DisableDepthTest();

	glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, coastUpdateTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, coastTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	glAttribStatePtr->ViewPort(0, 0, mapDims.mapx, mapDims.mapy);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	// initial stage; n=0
	coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);
	blurShader->SetUniform2i(2, 0, 0);

	// need this to avoid "Calling glEnd from the current immediate mode state is invalid."
	// (if coastmapAtlasRects contains no actual coastline rects no vertices are submitted
	// between glBegin and glEnd)
	unsigned int numCoastRects = 0;

	GL::RenderDataBufferT4* rdbt = GL::GetRenderBufferT4();

	{
		// NB: 4-channel texcoors
		for (const CoastAtlasRect& r: coastmapAtlasRects) {
			rdbt->SafeAppend({{r.x1, r.y1, 0.0f}, {r.tx1, r.ty1, 0.0f, 0.0f}}); // tl
			rdbt->SafeAppend({{r.x1, r.y2, 0.0f}, {r.tx1, r.ty2, 0.0f, 1.0f}}); // bl
			rdbt->SafeAppend({{r.x2, r.y2, 0.0f}, {r.tx2, r.ty2, 1.0f, 1.0f}}); // br

			rdbt->SafeAppend({{r.x2, r.y2, 0.0f}, {r.tx2, r.ty2, 1.0f, 1.0f}}); // br
			rdbt->SafeAppend({{r.x2, r.y1, 0.0f}, {r.tx2, r.ty1, 1.0f, 0.0f}}); // tr
			rdbt->SafeAppend({{r.x1, r.y1, 0.0f}, {r.tx1, r.ty1, 0.0f, 0.0f}}); // tl
			numCoastRects += r.isCoastline;
		}

		rdbt->Submit(GL_TRIANGLES);
	}

	if (numCoastRects > 0 && atlasSizeX > 0 && atlasSizeY > 0) {
		// (5*2) processing stages; n=1 to n=10 inclusive
		for (int i = 0; i < 5; ++i) {
			coastFBO.AttachTexture(coastUpdateTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);
			glAttribStatePtr->ViewPort(0, 0, atlasSizeX, atlasSizeY);
			blurShader->SetUniform2i(2, 1, i * 2 + 1);

			{
				for (const CoastAtlasRect& r: coastmapAtlasRects) {
					if (!r.isCoastline)
						continue;

					// NB: pass positions as texcoords for scissoring
					rdbt->SafeAppend({{r.tx1, r.ty1, 0.0f}, {r.x1, r.y1, 0.0f, 0.0f}}); // tl
					rdbt->SafeAppend({{r.tx1, r.ty2, 0.0f}, {r.x1, r.y2, 0.0f, 1.0f}}); // bl
					rdbt->SafeAppend({{r.tx2, r.ty2, 0.0f}, {r.x2, r.y2, 1.0f, 1.0f}}); // br

					rdbt->SafeAppend({{r.tx2, r.ty2, 0.0f}, {r.x2, r.y2, 1.0f, 1.0f}}); // br
					rdbt->SafeAppend({{r.tx2, r.ty1, 0.0f}, {r.x2, r.y1, 1.0f, 0.0f}}); // tr
					rdbt->SafeAppend({{r.tx1, r.ty1, 0.0f}, {r.x1, r.y1, 0.0f, 0.0f}}); // tl
				}

				rdbt->Submit(GL_TRIANGLES);
			}


			coastFBO.AttachTexture(coastTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);
			glAttribStatePtr->ViewPort(0, 0, mapDims.mapx, mapDims.mapy);
			blurShader->SetUniform2i(2, 0, i * 2 + 2);

			{
				for (const CoastAtlasRect& r: coastmapAtlasRects) {
					if (!r.isCoastline)
						continue;

					rdbt->SafeAppend({{r.x1, r.y1, 0.0f}, {r.tx1, r.ty1, 0.0f, 0.0f}}); // tl
					rdbt->SafeAppend({{r.x1, r.y2, 0.0f}, {r.tx1, r.ty2, 0.0f, 1.0f}}); // bl
					rdbt->SafeAppend({{r.x2, r.y2, 0.0f}, {r.tx2, r.ty2, 1.0f, 1.0f}}); // br

					rdbt->SafeAppend({{r.x2, r.y2, 0.0f}, {r.tx2, r.ty2, 1.0f, 1.0f}}); // br
					rdbt->SafeAppend({{r.x2, r.y1, 0.0f}, {r.tx2, r.ty1, 1.0f, 0.0f}}); // tr
					rdbt->SafeAppend({{r.x1, r.y1, 0.0f}, {r.tx1, r.ty1, 0.0f, 0.0f}}); // tl
				}

				rdbt->Submit(GL_TRIANGLES);
			}
		}
	}


	coastFBO.Detach(GL_COLOR_ATTACHMENT1);
	blurShader->Disable();
	glAttribStatePtr->PopBits();

	// NB: not needed during init, but no reason to leave bound after ::Update
	coastFBO.Unbind();

	// generate mipmaps
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, coastTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);

	// delete UpdateAtlas
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &coastUpdateTexture);
	coastmapAtlasRects.clear();

	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glActiveTexture(GL_TEXTURE0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  DYNAMIC WAVES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::UpdateDynWaves(const bool initialize)
{
	if (!dynWaves || !dynWavesFBO.IsValid())
		return;

	assert(waterRendering->numTiles <= 16);

	const int modFrameNum = (gs->frameNum + 1) % 60;

	const uint8_t tiles  = waterRendering->numTiles;
	const uint16_t ntiles = tiles * tiles; // uint8 overflows if tiles=16

	const float tilesize = 1.0f / tiles;

	if (modFrameNum == 0) {
		// randomize offsets every two seconds
		for (uint16_t i = 0; i < ntiles; ++i) {
			do {
				tileOffsets[i] = uint8_t(guRNG.NextFloat() * ntiles);
			} while (tileOffsets[i] == i);
		}
	}

	dynWavesFBO.Bind();
	glAttribStatePtr->PushBits(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->BlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	glAttribStatePtr->BlendColor(1.0f, 1.0f, 1.0f, (initialize) ? 1.0f : (modFrameNum + 1) / 600.0f);

	glBindTexture(GL_TEXTURE_2D, normalTexture2);


	GL::RenderDataBuffer2DT* buffer = GL::GetRenderBuffer2DT();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

	glAttribStatePtr->ViewPort(0, 0, normalTextureX, normalTextureY);

		for (uint8_t y = 0; y < tiles; ++y) {
			for (uint8_t x = 0; x < tiles; ++x) {
				const uint8_t offset = tileOffsets[y * tiles + x];
				const uint8_t tx = offset % tiles;
				const uint8_t ty = (offset - tx) / tiles;

				buffer->SafeAppend({(x    ) * tilesize, (y    ) * tilesize,  (tx    ) * tilesize, (ty    ) * tilesize}); // tl
				buffer->SafeAppend({(x    ) * tilesize, (y + 1) * tilesize,  (tx    ) * tilesize, (ty + 1) * tilesize}); // bl
				buffer->SafeAppend({(x + 1) * tilesize, (y + 1) * tilesize,  (tx + 1) * tilesize, (ty + 1) * tilesize}); // br

				buffer->SafeAppend({(x + 1) * tilesize, (y + 1) * tilesize,  (tx + 1) * tilesize, (ty + 1) * tilesize}); // br
				buffer->SafeAppend({(x + 1) * tilesize, (y    ) * tilesize,  (tx + 1) * tilesize, (ty    ) * tilesize}); // tr
				buffer->SafeAppend({(x    ) * tilesize, (y    ) * tilesize,  (tx    ) * tilesize, (ty    ) * tilesize}); // tl
			}
		}

	buffer->Submit(GL_TRIANGLES);
	shader->Disable();

	// redundant?
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->PopBits();

	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	dynWavesFBO.Unbind();

	glBindTexture(GL_TEXTURE_2D, normalTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  DRAW FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CBumpWater::SetUniforms()
{
	glUniform4f( uniforms[ 0], waterRendering->surfaceColor.x * 0.4f, waterRendering->surfaceColor.y * 0.4f, waterRendering->surfaceColor.z * 0.4f, waterRendering->surfaceAlpha);
	glUniform4f( uniforms[ 1], waterRendering->planeColor.x * 0.4f, waterRendering->planeColor.y * 0.4f, waterRendering->planeColor.z * 0.4f, waterRendering->surfaceAlpha);
	glUniform3f( uniforms[ 2], waterRendering->diffuseColor.x, waterRendering->diffuseColor.y, waterRendering->diffuseColor.z);
	glUniform3f( uniforms[ 3], waterRendering->specularColor.x, waterRendering->specularColor.y, waterRendering->specularColor.z);
	glUniform1f( uniforms[ 4], waterRendering->specularPower);
	glUniform1f( uniforms[ 5], waterRendering->specularFactor);
	glUniform1f( uniforms[ 6], waterRendering->ambientFactor);
	glUniform1f( uniforms[ 7], waterRendering->diffuseFactor * 15.0f);
	glUniform3fv(uniforms[ 8], 1, sky->GetLight()->GetLightDir());
	glUniform1f( uniforms[ 9], waterRendering->fresnelMin);
	glUniform1f( uniforms[10], waterRendering->fresnelMax);
	glUniform1f( uniforms[11], waterRendering->fresnelPower);
	glUniform1f( uniforms[12], waterRendering->reflDistortion);
	glUniform2f( uniforms[13], 0.0f, waterRendering->blurBase / globalRendering->viewSizeY);
	glUniform1f( uniforms[14], waterRendering->blurExponent);
	glUniform1f( uniforms[15], waterRendering->perlinStartFreq);
	glUniform1f( uniforms[16], waterRendering->perlinLacunarity);
	glUniform1f( uniforms[17], waterRendering->perlinAmplitude);
	glUniform1f( uniforms[18], waterRendering->windSpeed);
	glUniform1f( uniforms[19], sunLighting->groundShadowDensity);
}


void CBumpWater::Draw()
{
	if (!occlusionQueryResult || (!waterRendering->forceRendering && !readMap->HasVisibleWater()))
		return;

	if (occlusionQuery != 0) {
		glBeginConditionalRender(occlusionQuery, GL_QUERY_BY_REGION_WAIT);
		glBeginQuery(GL_ANY_SAMPLES_PASSED, occlusionQuery);
	}

	if (refraction == 1) {
		// _SCREENCOPY_ REFRACT TEXTURE
		glBindTexture(screenCopyTarget, refractTexture);
		glCopyTexSubImage2D(screenCopyTarget, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	if (depthCopy) {
		// _SCREENCOPY_ DEPTH TEXTURE
		glBindTexture(screenCopyTarget, depthTexture);
		glCopyTexSubImage2D(screenCopyTarget, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	if (refraction < 2)
		glAttribStatePtr->DisableDepthMask();

	if (refraction > 0)
		glAttribStatePtr->DisableBlendMask();


	waterShader->SetFlag("opt_shadows", (shadowHandler.ShadowsLoaded()));
	waterShader->SetFlag("opt_infotex", infoTextureHandler->IsEnabled());

	waterShader->Enable();
	waterShader->SetUniform3fv(0, &camera->GetPos()[0]);
	waterShader->SetUniform1f(1, (gs->frameNum + globalRendering->timeOffset) / 15000.0f);
	waterShader->SetUniform2f(16, windVec.x, windVec.z);
	waterShader->SetUniform1f(19, globalRendering->gammaExponent);
	waterShader->SetUniformMatrix4fv(14, false, camera->GetViewMatrix());
	waterShader->SetUniformMatrix4fv(15, false, camera->GetProjectionMatrix());

	if (shadowHandler.ShadowsLoaded()) {
		waterShader->SetUniformMatrix4fv(13, false, shadowHandler.GetShadowViewMatrixRaw());

		shadowHandler.SetupShadowTexSampler(GL_TEXTURE9);
	}

	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, caustTextures[gs->frameNum % caustTextures.size()]);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, foamTexture);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(screenCopyTarget, refractTexture);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, coastTexture);
	glActiveTexture(GL_TEXTURE7); glBindTexture(screenCopyTarget, depthTexture);
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, waveRandTexture);
	//glActiveTexture(GL_TEXTURE9); see above
	glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, normalTexture);

	if (useUniforms)
		SetUniforms();

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));
	waterPlaneBuffer.Submit(mix(GL_TRIANGLES, GL_TRIANGLE_STRIP, endlessOcean), 0, waterPlaneBuffer.GetNumElems<VA_TYPE_0>());
	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	waterShader->Disable();


	if (shadowHandler.ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE9); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glActiveTexture(GL_TEXTURE0);
	}

	if (refraction < 2)
		glAttribStatePtr->EnableDepthMask();

	if (refraction > 0)
		glAttribStatePtr->EnableBlendMask();


	if (occlusionQuery != 0) {
		glEndQuery(GL_ANY_SAMPLES_PASSED);
		glEndConditionalRender();
	}
}


void CBumpWater::DrawRefraction(CGame* game)
{
	// _RENDER_ REFRACTION TEXTURE
	refractFBO.Bind();

	camera->Update();

	glAttribStatePtr->ViewPort(0, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glAttribStatePtr->ClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 0);
	glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// TODO: disable shader fog or add special underwater settings
	const float3 oldsun = sunLighting->modelDiffuseColor;
	const float3 oldambient = sunLighting->modelAmbientColor;

	sunLighting->modelDiffuseColor *= float3(0.5f, 0.7f, 0.9f);
	sunLighting->modelAmbientColor *= float3(0.6f, 0.8f, 1.0f);

	DrawRefractions(true, true);

	sunLighting->modelDiffuseColor = oldsun;
	sunLighting->modelAmbientColor = oldambient;
}


void CBumpWater::DrawReflection(CGame* game)
{
	reflectFBO.Bind();

	glAttribStatePtr->ClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 0.0f);
	glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_UWREFL);
	CCamera* curCam = CCameraHandler::GetActiveCamera();

	{
		curCam->CopyStateReflect(prvCam);
		curCam->UpdateLoadViewPort(0, 0, reflTexSize, reflTexSize);

		DrawReflections(reflection > 1, true);
	}

	CCameraHandler::SetActiveCamera(prvCam->GetCamType());

	prvCam->Update();
	// done by caller
	// prvCam->LoadViewPort();
}


void CBumpWater::OcclusionQuery()
{
	if (occlusionQuery == 0 || (!waterRendering->forceRendering && !readMap->HasVisibleWater()))
		return;

	glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT_AVAILABLE, &occlusionQueryResult);

	if (occlusionQueryResult || !wasVisibleLastFrame) {
		glGetQueryObjectuiv(occlusionQuery, GL_QUERY_RESULT, &occlusionQueryResult);
		wasVisibleLastFrame = !!occlusionQueryResult;
	} else {
		occlusionQueryResult = true;
		wasVisibleLastFrame = true;
	}

	if (wasVisibleLastFrame)
		return;

	glAttribStatePtr->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glAttribStatePtr->DepthMask(GL_FALSE);
	glAttribStatePtr->EnableDepthTest();

	// default color-only shader
	Shader::IProgramObject* waterPlaneShader = &waterPlaneBuffer.GetShader();

	waterPlaneShader->Enable();
	waterPlaneShader->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix() * CMatrix44f(float3{0.0f, 10.0f, 0.0f}));
	waterPlaneShader->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());

	glBeginQuery(GL_ANY_SAMPLES_PASSED, occlusionQuery);
	waterPlaneBuffer.Submit(mix(GL_TRIANGLES, GL_TRIANGLE_STRIP, endlessOcean), 0, waterPlaneBuffer.GetNumElems<VA_TYPE_0>());
	glEndQuery(GL_ANY_SAMPLES_PASSED);

	waterPlaneShader->Disable();

	glAttribStatePtr->DepthMask(GL_TRUE);
	glAttribStatePtr->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

