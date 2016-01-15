/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "DecalsDrawerGL4.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
//#include "Rendering/GL/TimerQuery.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/LegacyAtlasAlloc.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/Textures/QuadtreeAtlasAlloc.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"


#define LOG_SECTION_DECALS_GL4 "DecalsDrawerGL4"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_DECALS_GL4)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_DECALS_GL4


#if !defined(GL_VERSION_4_0) || HEADLESS
CDecalsDrawerGL4::CDecalsDrawerGL4()
{
	throw opengl_error(LOG_SECTION_DECALS_GL4 ": Compiled without OpenGL4 support!");
}
#else




struct SGLSLDecal {
	float3 pos;
	float rot;
	float4 texOffsets;
	float2 size;
	float alpha;
	float unused;
};

struct SGLSLGroundLighting {
#ifdef _MSC_VER
	__declspec(align(16)) float3 ambientColor;
	__declspec(align(16))float3 diffuseColor;
	__declspec(align(16))float3 specularColor;
	__declspec(align(16))float3 dir;
	__declspec(align(16))float3 fogColor;
#else
	float3 ambientColor __attribute__ ((aligned (16)));
	float3 diffuseColor __attribute__ ((aligned (16)));
	float3 specularColor __attribute__ ((aligned (16)));
	float3 dir __attribute__ ((aligned (16)));
	float3 fogColor __attribute__ ((aligned (16)));
#endif
	float fogEnd;
	float fogScale;
	float3 unused;
};


static GLuint LoadTexture(const std::string& name, size_t* texX, size_t* texY)
{
	std::string fileName = name;

	if (FileSystem::GetExtension(fileName) == "")
		fileName += ".bmp";

	std::string fullName = fileName;
	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("bitmaps/tracks/") + fileName;
	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("unittextures/") + fileName;

	const bool isBitmap = (FileSystem::GetExtension(fullName) == "bmp");

	SCOPED_TIMER("::DECALS");
	CBitmap bm;
	if (!bm.Load(fullName)) {
		throw content_error("Could not load ground decal from file " + fullName);
	}
	if (isBitmap) {
		// bitmaps don't have an alpha channel
		// so use: red := brightness & green := alpha
		for (int y = 0; y < bm.ysize; ++y) {
			for (int x = 0; x < bm.xsize; ++x) {
				const int index = ((y * bm.xsize) + x) * 4;
				const int brightness = bm.mem[index + 0];
				bm.mem[index + 0] = (brightness * 90) / 255;
				bm.mem[index + 1] = (brightness * 60) / 255;
				bm.mem[index + 2] = (brightness * 30) / 255;
				bm.mem[index + 3] = bm.mem[index + 1];
			}
		}
	}

	*texX = bm.xsize;
	*texY = bm.ysize;
	return bm.CreateTexture(true);
}

	struct STex {
		GLuint id;
		size_t sizeX;
		size_t sizeY;
	};

typedef float4 SAtlasTex;
std::map<std::string, SAtlasTex> atlasTexs;


CDecalsDrawerGL4::CDecalsDrawerGL4()
	: CEventClient("[CDecalsDrawerGL4]", 314159, false)
	, maxDecals(0)
	, tbo(0)
	, depthTex(0)
	, atlasTex(0)
{
	lastUpdate = 0;

	eventHandler.AddClient(this);
	CExplosionCreator::AddExplosionListener(this);

	if (!globalRendering->haveGLSL) {
		throw opengl_error(LOG_SECTION_DECALS_GL4 ": missing GLSL");
	}

	//if (!GetDrawDecals()) {
	//	return;
	//}

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &tbo);
	glGenTextures(1, &depthTex);

	LoadShaders();

	//TODO FINISH
	if (!decalShader->IsValid()) {
		LOG_L(L_ERROR, "%s", decalShader->GetLog().c_str());
		throw opengl_error(LOG_SECTION_DECALS_GL4 ": cannot compile shader");
	}

	if (!GLEW_ARB_depth_clamp) //GL_DEPTH_CLAMP
		throw opengl_error(LOG_SECTION_DECALS_GL4 ": missing GL_ARB_depth_clamp");

	if (!dynamic_cast<CSMFReadMap*>(readMap))
		throw unsupported_error(LOG_SECTION_DECALS_GL4 ": only SMF supported");

	if (dynamic_cast<CSMFReadMap*>(readMap)->GetNormalsTexture() <= 0)
		throw unsupported_error(LOG_SECTION_DECALS_GL4 ": advanced map shading must be enabled");

	CreateBoundingBoxVBOs();
	CreateStructureVBOs();

	ViewResize();

	GenerateAtlasTexture();
}


CDecalsDrawerGL4::~CDecalsDrawerGL4()
{
	eventHandler.RemoveClient(this);

	glDeleteTextures(1, &tbo);
	glDeleteTextures(1, &depthTex);

	shaderHandler->ReleaseProgramObjects("[DecalsDrawerGL4]");
	decalShader = NULL;
}


void CDecalsDrawerGL4::LoadShaders()
{
	decalShader = shaderHandler->CreateProgramObject("[DecalsDrawerGL4]", "DecalShader", false);

	decalShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/DecalsVertGL4.glsl", "", GL_VERTEX_SHADER));
	decalShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/DecalsFragGL4.glsl", "", GL_FRAGMENT_SHADER));
	decalShader->Link();

	if (!decalShader->IsValid()) {
		const char* fmt = "decal-shader validation error:\n%s";
		const char* log = (decalShader->GetLog()).c_str();
		LOG_L(L_ERROR, fmt, log);
	}

	decalShader->Enable();
		decalShader->SetUniform("invMapSizePO2", 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
		decalShader->SetUniform("invMapSize",    1.0f / (mapDims.mapx * SQUARE_SIZE),     1.0f / (mapDims.mapy * SQUARE_SIZE));
		decalShader->SetUniform("invScreenSize", 1.0f / globalRendering->viewSizeX,   1.0f / globalRendering->viewSizeY);

	decalShader->Disable();
	decalShader->Validate();
}


void CDecalsDrawerGL4::GenerateAtlasTexture()
{
	std::map<std::string, STex> textures;
	for (UnitDef& unitDef: unitDefHandler->unitDefs) {
		SolidObjectDecalDef& decalDef = unitDef.decalDef;

		if (!decalDef.useGroundDecal)
			continue;
		if (textures.find(decalDef.groundDecalTypeName) != textures.end())
			continue;

		try {
			STex tex;
			tex.id = LoadTexture(decalDef.groundDecalTypeName, &tex.sizeX, &tex.sizeY);
			textures[decalDef.groundDecalTypeName] = tex;
		} catch(const content_error& err) {
			LOG_L(L_ERROR, "%s", err.what());
			STex tex; tex.id = 0;
			textures[decalDef.groundDecalTypeName] = tex;
		}
	}

	{
		LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
		if (!resourcesParser.Execute()) {
			LOG_L(L_ERROR, "Failed to load resources: %s", resourcesParser.GetErrorLog().c_str());
		}

		const LuaTable scarsTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("scars");
		for (int i = 1; i <= 4; ++i)
		{
				STex tex;
				tex.id = LoadTexture("bitmaps/" + scarsTable.GetString(i, IntToString(i, "scars/scar%i.bmp")), &tex.sizeX, &tex.sizeY);
				textures[IntToString(i)] = tex;
		}
	}

	CQuadtreeAtlasAlloc atlas;
	//CLegacyAtlasAlloc atlas;
	atlas.SetNonPowerOfTwo(globalRendering->supportNPOTs);
	atlas.SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);
	//atlas.SetMaxSize(512,512);

	for (std::map<std::string, STex>::const_iterator it = textures.begin(); it != textures.end(); ++it) {
		if (it->second.id == 0)
			continue;

		const float maxSize = 512;
		size_t sizeX = it->second.sizeX;
		size_t sizeY = it->second.sizeY;
		if (sizeX > maxSize) {
			sizeY = sizeY * (maxSize / sizeX);
			sizeX = maxSize;
		}
		if (sizeY > maxSize) {
			sizeX = sizeX * (maxSize / sizeY);
			sizeY = maxSize;
		}

		atlas.AddEntry(it->first, int2(sizeX, sizeY));
	}
	/*bool success =*/ atlas.Allocate();



	glGenTextures(1, &atlasTex);
	glBindTexture(GL_TEXTURE_2D, atlasTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//#ifdef OPENGL4 //FIXME
//		glTexStorage2D(GL_TEXTURE_2D, atlas.GetMaxMipMaps(), GL_RGBA8, atlas.GetAtlasSize().x, atlas.GetAtlasSize().y);
//#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, atlas.GetMaxMipMaps());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas.GetAtlasSize().x, atlas.GetAtlasSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
//#endif

	FBO fb;
	if (!fb.IsValid()) {
		LOG_L(L_ERROR, "[%s] framebuffer not valid", __FUNCTION__);
		return;
	}
	fb.Bind();
	fb.AttachTexture(atlasTex);
	bool status = fb.CheckStatus(LOG_SECTION_DECALS_GL4);
	if (!status) {
		LOG_L(L_ERROR, "[%s] Couldn't render to FBO!", __FUNCTION__);
		return;
	}

	fb.Bind();
		glViewport(0, 0, atlas.GetAtlasSize().x, atlas.GetAtlasSize().y);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f); //red
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0,1,0,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(1.0f/atlas.GetAtlasSize().x,1.0f/atlas.GetAtlasSize().y,1.0f);

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ZERO);
		glDisable(GL_DEPTH_TEST);

	for (std::map<std::string, STex>::const_iterator it = textures.begin(); it != textures.end(); ++it) {
		if (it->second.id == 0)
			continue;

		float4 texCoords = atlas.GetTexCoords(it->first);
		float4 absCoords = atlas.GetEntry(it->first);

		//FIXME
		//if (!node) {
		//	LOG_L(L_ERROR, "DecalTextureAtlas full: failed to add %s", it->first.c_str());
		//	continue;
		//}

		SAtlasTex aTex(texCoords);
		atlasTexs[it->first.c_str()] = aTex;

		CVertexArray va;
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, it->second.id);
		//FIXME why +1?
		va.AddVertex2dT(absCoords.x,   absCoords.y, 0.0f, 0.0f);
		va.AddVertex2dT(absCoords.z+1, absCoords.y, 1.0f, 0.0f);
		va.AddVertex2dT(absCoords.x,   absCoords.w+1, 0.0f, 1.0f);
		va.AddVertex2dT(absCoords.z+1, absCoords.w+1, 1.0f, 1.0f);
		va.DrawArray2dT(GL_TRIANGLE_STRIP);
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef DEBUG
	const size_t datasize = 4 * atlas.GetAtlasSize().x * atlas.GetAtlasSize().y;
	unsigned char* img = new unsigned char[datasize];
	memset(img, 0, datasize);
	glReadPixels(0, 0, atlas.GetAtlasSize().x, atlas.GetAtlasSize().y, GL_RGBA, GL_UNSIGNED_BYTE, img);
	CBitmap bitmap(img, atlas.GetAtlasSize().x, atlas.GetAtlasSize().y);
	bitmap.Save("x_decal_atlas.png", false);
	delete[] img;
#endif

	for (std::map<std::string, STex>::const_iterator it = textures.begin(); it != textures.end(); ++it) {
		if (it->second.id == 0)
			continue;
		glDeleteTextures(1, &it->second.id);
	}

	fb.Unbind();
	glBindTexture(GL_TEXTURE_2D, atlasTex);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void CDecalsDrawerGL4::CreateBoundingBoxVBOs()
{
	float3 boxverts[] = {
		float3(-1.0f, -1.0f, -1.0f),
		float3(-1.0f,  1.0f, -1.0f),
		float3( 1.0f, -1.0f, -1.0f),
		float3( 1.0f,  1.0f, -1.0f),

		float3( 1.0f, -1.0f,  1.0f),
		float3( 1.0f,  1.0f,  1.0f),
		float3(-1.0f, -1.0f,  1.0f),
		float3(-1.0f,  1.0f,  1.0f),
	};
	GLubyte indices[] = {
		//back
		0, 1, 2,
		3, 2, 1,

		//right
		2, 3, 4,
		5, 4, 3,

		//front
		4, 5, 6,
		7, 6, 5,

		//left
		6, 7, 0,
		1, 0, 7,

		//btm
		0, 4, 6,
		0, 2, 4,

		//top
		1, 5, 3,
		1, 7, 5,
	};

	vboVertices.Bind(GL_ARRAY_BUFFER);
	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);

	vboVertices.New(sizeof(boxverts) * sizeof(float3), GL_STATIC_DRAW, &boxverts[0]);
	vboIndices.New(sizeof(indices) * sizeof(GLubyte), GL_STATIC_DRAW, &indices[0]);

	vboVertices.Unbind();
	vboIndices.Unbind();
}


void CDecalsDrawerGL4::CreateStructureVBOs()
{
	{
	GLuint uniformBlockIndex = glGetUniformBlockIndex(decalShader->GetObjID(), "SGroundLighting");
	assert(uniformBlockIndex != GL_INVALID_INDEX);

	GLsizei uniformBlockSize = 0;
	glGetActiveUniformBlockiv(decalShader->GetObjID(), uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);

	if (uniformBlockSize == 0)
		LOG("uniformBlockSize %u", uniformBlockSize);
	assert(uniformBlockSize > 0);

	if (uniformBlockSize % sizeof(SGLSLGroundLighting) != 0)
		LOG("uniformBlockSize sizeof(SGLSLGroundLighting) %u " _STPF_, uniformBlockSize, sizeof(SGLSLGroundLighting));
	assert(uniformBlockSize % sizeof(SGLSLGroundLighting) == 0);


	uboGroundLighting.Bind(GL_UNIFORM_BUFFER);
	uboGroundLighting.New(uniformBlockSize, GL_STATIC_DRAW);
		SGLSLGroundLighting* uboGroundLightingData = (SGLSLGroundLighting*)uboGroundLighting.MapBuffer(0, sizeof(SGLSLGroundLighting));
		uboGroundLightingData->ambientColor  = sunLighting->groundAmbientColor  * CGlobalRendering::SMF_INTENSITY_MULT;
		uboGroundLightingData->diffuseColor  = sunLighting->groundSunColor      * CGlobalRendering::SMF_INTENSITY_MULT;
		uboGroundLightingData->specularColor = sunLighting->groundSpecularColor * CGlobalRendering::SMF_INTENSITY_MULT;
		uboGroundLightingData->dir           = mapInfo->light.sunDir;
		uboGroundLightingData->fogColor      = mapInfo->atmosphere.fogColor;
		uboGroundLightingData->fogEnd        = globalRendering->viewRange * mapInfo->atmosphere.fogEnd;
		uboGroundLightingData->fogScale      = 1.0f / (globalRendering->viewRange * (mapInfo->atmosphere.fogEnd - mapInfo->atmosphere.fogStart));
		uboGroundLighting.UnmapBuffer();
	glUniformBlockBinding(decalShader->GetObjID(), uniformBlockIndex, 5);
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, uboGroundLighting.GetId());
	uboGroundLighting.Unbind();
	}

	{
	uboDecalsStructures.Bind(GL_UNIFORM_BUFFER);

	// Uniform Array Solution
	/*GLuint uniformBlockIndex = glGetUniformBlockIndex(decalShader->GetObjID(), "decals");

	GLsizei uniformBlockSize = 0;
	glGetActiveUniformBlockiv(decalShader->GetObjID(), uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);

	if (uniformBlockSize % sizeof(SGLSLDecal) != 0)
		LOG("uniformBlockSize sizeof(SGLSLDecal) %u " _STPF_, uniformBlockSize, sizeof(SGLSLDecal));
	assert(uniformBlockSize % sizeof(SGLSLDecal) == 0);

	maxDecals = uniformBlockSize / sizeof(SGLSLDecal);

	uniformBlockSize = maxDecals * sizeof(SGLSLDecal);
	uboDecalsStructures.New(uniformBlockSize, GL_DYNAMIC_DRAW);

	glUniformBlockBinding(decalShader->GetObjID(), uniformBlockIndex, 3);
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboDecalsStructures.GetId());*/

	// TBO solution
	GLint maxTexBufSize;
	glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTexBufSize);

	maxDecals = (maxTexBufSize / sizeof(SGLSLDecal)) / 2;

	GLsizei uniformBlockSize = maxDecals * sizeof(SGLSLDecal);
	uboDecalsStructures.New(uniformBlockSize, GL_DYNAMIC_DRAW);

	uboDecalsStructures.Unbind();
	}

	glBindTexture(GL_TEXTURE_BUFFER, tbo);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, uboDecalsStructures.GetId());

	/*{
	vboVisibilityFeeback.Bind(GL_UNIFORM_BUFFER);
	vboVisibilityFeeback.New(maxDecals, GL_STATIC_DRAW);
	vboVisibilityFeeback.Unbind();
	}*/
}


void CDecalsDrawerGL4::ViewResize()
{
	decalShader->Enable();
	decalShader->SetUniform("invScreenSize", 1.f / globalRendering->viewSizeX, 1.f / globalRendering->viewSizeY);

	decalShader->Disable();

	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
}


void CDecalsDrawerGL4::Draw()
{
	if (!GetDrawDecals())
		return;

	if (decals.empty())
		return;

	UpdateDecalsVBO();
	//UpdateVisibilityVBO();

	glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);

	/*static GLTimerQuery glTimer;
	static float gpuDrawFrameTimeInMS = 0;
	if (glTimer.Available()) {
		gpuDrawFrameTimeInMS = mix(gpuDrawFrameTimeInMS, glTimer.Query(), 0.05f);
		//LOG("CDecalsDrawerGL4::Draw %.4f ms", gpuDrawFrameTimeInMS);
	}
	glTimer.Start();*/

	const CSMFReadMap* smfrm = dynamic_cast<CSMFReadMap*>(readMap);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	//glEnable(GL_DEPTH_TEST);

	decalShader->SetFlag("HAVE_SHADOWS", ShadowHandler->shadowsLoaded());
	decalShader->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());

	decalShader->Enable();
		decalShader->SetUniform3v("camPos", &camera->GetPos()[0]);
		//decalShader->SetUniform("invMapSizePO2", 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
		//decalShader->SetUniform("invMapSize",    1.0f / (mapDims.mapx * SQUARE_SIZE),     1.0f / (mapDims.mapy * SQUARE_SIZE));
		//decalShader->SetUniform("invScreenSize", 1.0f / globalRendering->viewSizeX,   1.0f / globalRendering->viewSizeY);

	glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, smfrm->GetNormalsTexture());

	if (shadowHandler->ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

		decalShader->SetUniformMatrix4x4("shadowMatrix", false, shadowHandler->GetShadowMatrixRaw());
		decalShader->SetUniform("shadowDensity", sky->GetLight()->GetGroundShadowDensity()); //FIXME only do on shadow change?
	}

	glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());

	glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, depthTex);

	glActiveTexture(GL_TEXTURE5);
		CNamedTextures::Bind("scar2_normal.tga");

	glActiveTexture(GL_TEXTURE6);
		CNamedTextures::Bind("CopperVerdigris-NormalMap.png");

	glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_BUFFER, tbo);

	glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlasTex);

	DrawDecals();

	glDisable(GL_DEPTH_CLAMP);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	decalShader->Disable();

	//glTimer.Stop();
}


void CDecalsDrawerGL4::UpdateVisibilityVBO()
{
	/*glEnable(GL_RASTERIZER_DISCARD);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboVisibilityFeeback.GetId());
	glBeginTransformFeedback(GL_POINTS);

		vboVertices.Bind(GL_ARRAY_BUFFER);
		vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, vboVertices.GetPtr());

				// note, we only render one vertex!
				glDrawElementsInstanced(GL_POINTS, 1, GL_UNSIGNED_BYTE, vboIndices.GetPtr(), scars.size());

			glDisableClientState(GL_VERTEX_ARRAY);

		vboVertices.Unbind();
		vboIndices.Unbind();

	glEndTransformFeedback();
	glDisable(GL_RASTERIZER_DISCARD);*/
}


void CDecalsDrawerGL4::UpdateDecalsVBO()
{
	if (lastUpdate >= decals.size())
		return;

	if (decals.size() > maxDecals)
		return;

	uboDecalsStructures.Bind(GL_UNIFORM_BUFFER);
		GLubyte* uboData = uboDecalsStructures.MapBuffer(sizeof(SGLSLDecal) * lastUpdate, sizeof(SGLSLDecal) * (std::min(decals.size(), maxDecals) - lastUpdate));
		SGLSLDecal* uboScarsData = (SGLSLDecal*)uboData;
		for (size_t i=0; i < (std::min(decals.size(), maxDecals) - lastUpdate); ++i) {
			uboScarsData[i].pos        = decals[i + lastUpdate]->pos;
			uboScarsData[i].rot        = decals[i + lastUpdate]->rot;
			uboScarsData[i].size       = decals[i + lastUpdate]->size;
			uboScarsData[i].alpha      = decals[i + lastUpdate]->alpha;
			uboScarsData[i].texOffsets = decals[i + lastUpdate]->texOffsets;
		}
		uboDecalsStructures.UnmapBuffer();
	uboDecalsStructures.Unbind();

	// UBO: Update binding
	glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboDecalsStructures.GetId());

	// TBO: seems it is not needed to rebound TBOs
	//glBindTexture(GL_TEXTURE_BUFFER, tbo);
	//glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, uboDecalsStructures.GetId());

	lastUpdate = decals.size();
}


void CDecalsDrawerGL4::DrawDecals()
{
	vboVertices.Bind(GL_ARRAY_BUFFER);
	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vboVertices.GetPtr());

			glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboDecalsStructures.GetId());
			glBindBufferBase(GL_UNIFORM_BUFFER, 5, uboGroundLighting.GetId());
			glDrawElementsInstanced(GL_TRIANGLES, vboIndices.GetSize(), GL_UNSIGNED_BYTE, vboIndices.GetPtr(), std::min(decals.size(), maxDecals));

		glDisableClientState(GL_VERTEX_ARRAY);

	vboVertices.Unbind();
	vboIndices.Unbind();
}


void CDecalsDrawerGL4::ExplosionOccurred(const CExplosionEvent& event) {
	AddExplosion(event.GetPos(), event.GetDamage(), event.GetRadius(), ((event.GetWeaponDef() != NULL) && event.GetWeaponDef()->visuals.explosionScar));
}


void CDecalsDrawerGL4::AddExplosion(float3 pos, float damage, float radius, bool addScar)
{
	if (!addScar)
		return;

	//FIXME just delink from eventhandler!
	//if (!GetDrawDecals())
	//	return;

	//FIXME decalLevel is private! const float lifeTime = decalLevel * damage * 3.0f;
	const float altitude = pos.y - CGround::GetHeightReal(pos.x, pos.z, false);

	// no decals for below-ground & in-air explosions
	if (std::abs(altitude) > radius) { return; }

	pos.y -= altitude;
	radius -= altitude;

	damage = std::min(damage, radius * 30.0f);
	damage *= (radius) / (radius + altitude);
	radius = std::min(radius, damage * 0.25f);

	if (radius <= 0.0f) { return; }

	const float MAGIC_NUMBER2 = 1.4f;

	Decal* s  = new Decal;
	s->pos    = pos;
	s->rot    = gu->RandFloat() * fastmath::PI2;
	s->size.x = radius * MAGIC_NUMBER2;
	s->size.y = s->size.x;
	s->alpha  = Clamp(damage / 255.0f, 0.2f, 1.0f);

	// pick one of 4 scar textures
	int r = (gu->RandInt() & 3) + 1;
	char buf[2];
	SNPRINTF(buf, sizeof(buf), "%i", r);
	s->texOffsets = atlasTexs[std::string(buf)];

	if (decals.size() < maxDecals) {
		//FIXME use mt-safe container
		decals.push_back(s);
	} else {
		//FIXME
		delete s;
	}
}


void CDecalsDrawerGL4::UnitCreated(const CUnit* unit, const CUnit* builder)
{
	//if (!GetDrawDecals())
	//	return;
	if (!unit->unitDef->decalDef.useGroundDecal)
		return;

	const int sizex = unit->unitDef->decalDef.groundDecalSizeX;
	const int sizey = unit->unitDef->decalDef.groundDecalSizeY;

	Decal* s  = new Decal;
	s->pos    = unit->pos;
	s->rot    = GetRadFromXY(unit->frontdir.x, unit->frontdir.z) - fastmath::HALFPI;
	s->size.x = sizex * SQUARE_SIZE;
	s->size.y = sizey * SQUARE_SIZE;
	s->alpha  = 1.0f;

	s->texOffsets = atlasTexs[unit->unitDef->decalDef.groundDecalTypeName];

	if (decals.size() < maxDecals) {
		//FIXME use mt-safe container
		decals.push_back(s);
	} else {
		//FIXME
		delete s;
	}
}


void CDecalsDrawerGL4::UnitDestroyed(const CUnit* unit, const CUnit* attacker, bool preEvent)
{
	if (!preEvent)
		return;
	if (!unit->unitDef->decalDef.useGroundDecal)
		return;

	//TODO FINISH
	//decal->owner = NULL;
	//decal->gbOwner = gb;
}

#endif // !defined(GL_VERSION_4_0) || HEADLESS
