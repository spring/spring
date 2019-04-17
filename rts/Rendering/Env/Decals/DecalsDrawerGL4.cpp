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
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/QuadtreeAtlasAlloc.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/UnorderedMap.hpp"

#include <numeric>


CONFIG(bool, GroundDecalsParallaxMapping).defaultValue(true);


//#define DEBUG_SAVE_ATLAS


#define LOG_SECTION_DECALS_GL4 "DecalsDrawerGL4"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_DECALS_GL4)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_DECALS_GL4


#if !defined(GL_VERSION_4_3) || HEADLESS
CDecalsDrawerGL4::CDecalsDrawerGL4()
{
	throw opengl_error(LOG_SECTION_DECALS_GL4 ": Compiled without OpenGL4 support!");
}
#else


static CDecalsDrawerGL4* decalDrawer = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SGLSLDecal {
	alignas(16) float3 pos;
		    float  alpha;
	alignas(16) float2 size;
		    float2 rotMatElements;
	alignas(16) float4 texOffsets;
	alignas(16) float4 texNormalOffsets;
};

struct SGLSLGroundLighting {
	alignas(16) float3 ambientColor;
	alignas(16) float3 diffuseColor;
	alignas(16) float3 specularColor;
	alignas(16) float3 dir;
	alignas(16) float3 fogColor;
	float fogEnd;
	float fogScale;
	float3 unused;
};


struct STex {
	GLuint id;
	int2 size;
};

typedef float4 SAtlasTex;
static spring::unordered_map<std::string, SAtlasTex> atlasTexs;


static std::string GetExtraTextureName(const std::string& s)
{
	return s + "_normal"; //FIXME
}


bool CDecalsDrawerGL4::Decal::IsValid() const
{
	return (size.x > 0);
}


bool CDecalsDrawerGL4::Decal::InView() const
{
	return camera->InView(pos, std::max(size.x, size.y) * math::SQRT2);
}


float CDecalsDrawerGL4::Decal::GetRating(bool inview_test) const
{
	float r = 0.f;
	r += 10.f * alpha;
	r += 100.f * (inview_test && InView());
	r += 1000.f * Clamp((size.x * size.y) * 0.0001f, 0.0f, 1.0f);
	r += 10000.f * int(type);
	return r;
}


void CDecalsDrawerGL4::Decal::SetOwner(const void* o)
{
	if (!IsValid())
		return;

	owner = o;
	if (o == nullptr) {
		decalDrawer->alphaDecayingDecals.push_back(GetIdx());
	}
}


int CDecalsDrawerGL4::Decal::GetIdx() const
{
	return (this - &decalDrawer->decals[0]) / sizeof(CDecalsDrawerGL4::Decal);
}


void CDecalsDrawerGL4::Decal::Free() const
{
	decalDrawer->FreeDecal(GetIdx());
}


void CDecalsDrawerGL4::Decal::Invalidate() const
{
	const int idx = GetIdx();
	decalDrawer->decalsToUpdate.push_back(idx);

}


bool CDecalsDrawerGL4::Decal::InvalidateExtents() const
{
	Invalidate();
	const int idx = GetIdx();
	decalDrawer->waitingDecalsForOverlapTest.push_back(idx);
	decalDrawer->RemoveFromGroup(idx);
	return decalDrawer->FindAndAddToGroup(idx);
}


void CDecalsDrawerGL4::Decal::SetTexture(const std::string& name)
{
	texOffsets       = atlasTexs["%FALLBACK_TEXTURE%"];
	texNormalOffsets = atlasTexs["%FALLBACK_TEXTURE_NORMAL%"];

	const auto it = atlasTexs.find(name);
	if (it != atlasTexs.end()) {
		texOffsets = it->second;
	}

	const auto it2 = atlasTexs.find(GetExtraTextureName(name));
	if (it2 != atlasTexs.end()) {
		texNormalOffsets = it2->second;
	}
}


std::string CDecalsDrawerGL4::Decal::GetTexture() const
{
	for (auto& p: atlasTexs) {
		if (p.second != texOffsets)
			continue;

		return p.first;
	}

	return "not_found";
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////


CDecalsDrawerGL4::CDecalsDrawerGL4()
	: CEventClient("[CDecalsDrawerGL4]", 314159, false)
	, curWorstDecalIdx(-1)
	, curWorstDecalRating(1e6)
	, maxDecals(0)
	, maxDecalGroups(0)
	, useSSBO(false)
	, laggedFrames(0)
	, overlapStage(0)
	, depthTex(0)
	, atlasTex(0)
{
	//if (!GetDrawDecals()) {
	//	return;
	//}

	if (dynamic_cast<CSMFReadMap*>(readMap) == nullptr)
		throw unsupported_error(LOG_SECTION_DECALS_GL4 ": only SMF supported");

	if (static_cast<CSMFReadMap*>(readMap)->GetNormalsTexture() <= 0)
		throw unsupported_error(LOG_SECTION_DECALS_GL4 ": advanced map shading must be enabled");

	DetectMaxDecals();
	LoadShaders();
	if (!decalShader->IsValid())
		throw opengl_error(LOG_SECTION_DECALS_GL4 ": cannot compile shader");

	glGenTextures(1, &depthTex);
	bboxArray.Generate();

	CreateBoundingBoxVBOs();
	CreateStructureVBOs();
	SunChanged();

	ViewResize();
	GenerateAtlasTexture();

	fboOverlap.Bind();
	//fboOverlap.CreateRenderBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA8, OVERLAP_TEST_TEXTURE_SIZE, OVERLAP_TEST_TEXTURE_SIZE);
	fboOverlap.CreateRenderBuffer(GL_STENCIL_ATTACHMENT, GL_STENCIL_INDEX8, OVERLAP_TEST_TEXTURE_SIZE, OVERLAP_TEST_TEXTURE_SIZE);
	fboOverlap.CheckStatus(LOG_SECTION_DECALS_GL4);
	fboOverlap.Unbind();

	eventHandler.AddClient(this);
	CExplosionCreator::AddExplosionListener(this);

	decalDrawer = this;
}


CDecalsDrawerGL4::~CDecalsDrawerGL4()
{
	eventHandler.RemoveClient(this);

	glDeleteTextures(1, &depthTex);
	glDeleteTextures(1, &atlasTex);
	bboxArray.Delete();

	shaderHandler->ReleaseProgramObjects("[DecalsDrawerGL4]");

	decalShader = nullptr;
	decalDrawer = nullptr;
}


void CDecalsDrawerGL4::OnDecalLevelChanged()
{
	//note: AddClient() is safe against double adding
	if (GetDrawDecals()) {
		eventHandler.AddClient(this);
	} else {
		eventHandler.RemoveClient(this);
	}
}










//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDecalsDrawerGL4::DetectMaxDecals()
{
	// detect hw limits
	const int maxDecalsUBO  = globalRendering->glslMaxUniformBufferSize / sizeof(SGLSLDecal);
	const int maxDecalsSSBO = 0xFFFFFF; // SSBO min supported size is 16MB
	const int maxDecalGroupsUBO  = globalRendering->glslMaxUniformBufferSize / sizeof(SDecalGroup);
	const int maxDecalGroupsSSBO = maxDecalGroupsUBO; // groups are always saved in a UBO

	const int userWanted = decalLevel * 512;

	// detect if SSBO is needed and available
	useSSBO = (userWanted > maxDecalsUBO && GLEW_ARB_shader_storage_buffer_object);

	// now calc the actual max usable
	maxDecals      = (useSSBO) ? maxDecalsUBO : maxDecalsSSBO;
	maxDecalGroups = (useSSBO) ? maxDecalGroupsUBO : maxDecalGroupsSSBO;
	maxDecals      = std::min({userWanted, maxDecals, maxDecalGroups});
	maxDecalGroups = std::min(maxDecalGroups, maxDecals);

	if (maxDecals == 0 || maxDecalGroups == 0)
		throw unsupported_error(LOG_SECTION_DECALS_GL4 ": no UBO/SSBO");

	//FIXME
	LOG_L(L_ERROR, "MaxDecals=%i[UBO=%i SSBO=%i] MaxDecalGroups=%i[UBO=%i SSBO=%i] useSSBO=%i",
		maxDecals, maxDecalsUBO, maxDecalsSSBO,
		maxDecalGroups, maxDecalGroupsUBO, maxDecalGroupsSSBO,
		int(useSSBO));

	decals.resize(maxDecals);
	freeIds.resize(maxDecals - 1); // idx = 0 is invalid, so -1
	std::iota(freeIds.begin(), freeIds.end(), 1); // start with 1, 0 is illegal
	std::random_shuffle(freeIds.begin(), freeIds.end(), guRNG);
	groups.reserve(maxDecalGroups);
}


void CDecalsDrawerGL4::LoadShaders()
{
	decalShader = shaderHandler->CreateProgramObject("[DecalsDrawerGL4]", "DecalShader");

	decalShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/DecalsVertGL4.glsl", "", GL_VERTEX_SHADER));
	decalShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/DecalsFragGL4.glsl", "", GL_FRAGMENT_SHADER));
	decalShader->SetFlag("MAX_DECALS_PER_GROUP", MAX_DECALS_PER_GROUP);
	decalShader->SetFlag("MAX_DECALS_GROUPS", maxDecalGroups);
	decalShader->SetFlag("MAX_DECALS", maxDecals);
	decalShader->SetFlag("USE_SSBO", useSSBO);
	decalShader->SetFlag("USE_PARALLAX", configHandler->GetBool("GroundDecalsParallaxMapping"));
	decalShader->Link();

	decalShader->Enable();
		decalShader->SetUniform("invMapSizePO2", 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
		decalShader->SetUniform("invMapSize",    1.0f / (mapDims.mapx * SQUARE_SIZE),     1.0f / (mapDims.mapy * SQUARE_SIZE));
		decalShader->SetUniform("invScreenSize", 1.0f / globalRendering->viewSizeX,   1.0f / globalRendering->viewSizeY);
	decalShader->Disable();
	decalShader->Validate();
}


static STex LoadTexture(const std::string& name)
{
	std::string fileName = name;

	if (FileSystem::GetExtension(fileName).empty())
		fileName += ".bmp";

	std::string fullName = fileName;
	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("bitmaps/") + fileName;
	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("bitmaps/tracks/") + fileName;
	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("unittextures/") + fileName;

	CBitmap bm;
	if (!bm.Load(fullName))
		throw content_error("Could not load ground decal \"" + fileName + "\"");

	if (FileSystem::GetExtension(fullName) == "bmp") {
		// bitmaps don't have an alpha channel
		// so use: red := brightness & green := alpha
		const unsigned char* rmem = bm.GetRawMem();
		      unsigned char* wmem = bm.GetRawMem();

		for (int y = 0; y < bm.ysize; ++y) {
			for (int x = 0; x < bm.xsize; ++x) {
				const int index = ((y * bm.xsize) + x) * 4;

				const auto brightness = rmem[index + 0];
				const auto alpha      = rmem[index + 1];

				wmem[index + 0] = (brightness * 90) / 255;
				wmem[index + 1] = (brightness * 60) / 255;
				wmem[index + 2] = (brightness * 30) / 255;
				wmem[index + 3] = alpha;
			}
		}
	}

	return { bm.CreateTexture(), int2(bm.xsize, bm.ysize) };
}


static inline void GetBuildingDecals(spring::unordered_map<std::string, STex>& textures)
{
	for (const UnitDef& unitDef: unitDefHandler->GetUnitDefsVec()) {
		const SolidObjectDecalDef& decalDef = unitDef.decalDef;

		if (!decalDef.useGroundDecal)
			continue;
		if (textures.find(decalDef.groundDecalTypeName) != textures.end())
			continue;

		try {
			textures[                    decalDef.groundDecalTypeName ] = LoadTexture(decalDef.groundDecalTypeName);
			textures[GetExtraTextureName(decalDef.groundDecalTypeName)] = LoadTexture(decalDef.groundDecalTypeName + ".dds"); //FIXME
		} catch(const content_error& err) {
			LOG_L(L_ERROR, "%s", err.what());
		}
	}
}


static inline void GetGroundScars(spring::unordered_map<std::string, STex>& textures)
{
	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		LOG_L(L_ERROR, "Failed to load resources: %s", resourcesParser.GetErrorLog().c_str());
	}

	const LuaTable scarsTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("scars");

	for (int i = 1; i <= scarsTable.GetLength(); ++i) {
		std::string texName  = scarsTable.GetString(i, IntToString(i, "scars/scar%i.bmp"));
		std::string texName2 = texName + ".dds"; //FIXME
		auto name = IntToString(i);

		try {
			textures[                    name ] = LoadTexture(texName);
			textures[GetExtraTextureName(name)] = LoadTexture(texName2);
		} catch(const content_error& err) {
			LOG_L(L_ERROR, "%s", err.what());
		}
	}
}


static inline void GetFallbacks(spring::unordered_map<std::string, STex>& textures)
{
	auto CREATE_SINGLE_COLOR = [](SColor c) -> STex {
		CBitmap bm;
		bm.AllocDummy(c);
		bm = bm.CreateRescaled(32, 32);
		return { bm.CreateTexture(), int2(bm.xsize, bm.ysize) };
	};

	textures["%FALLBACK_TEXTURE%"] = CREATE_SINGLE_COLOR(SColor(255, 0, 0, 255));
	textures["%FALLBACK_TEXTURE_NORMAL%"] = CREATE_SINGLE_COLOR(SColor(0, 255, 0, 255));
}


void CDecalsDrawerGL4::GenerateAtlasTexture()
{
	spring::unordered_map<std::string, STex> textures;

	GetBuildingDecals(textures);
	GetGroundScars(textures);
	GetFallbacks(textures);

	CQuadtreeAtlasAlloc atlas;
	atlas.SetNonPowerOfTwo(true);
	atlas.SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);
	for (const auto& texture: textures) {
		if (texture.second.id == 0)
			continue;

		const float maxSize = 1024; //512;
		int2 size = texture.second.size;
		if (size.x > maxSize) {
			size.y = size.y * (maxSize / size.x);
			size.x = maxSize;
		}
		if (size.y > maxSize) {
			size.x = size.x * (maxSize / size.y);
			size.y = maxSize;
		}

		atlas.AddEntry(texture.first, size);
	}
	/*bool success =*/ atlas.Allocate();



	glGenTextures(1, &atlasTex);
	glBindTexture(GL_TEXTURE_2D, atlasTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
	glSpringTexStorage2D(GL_TEXTURE_2D, atlas.GetMaxMipMaps(), GL_RGBA8, atlas.GetAtlasSize().x, atlas.GetAtlasSize().y);

	FBO fb;
	if (!fb.IsValid()) {
		LOG_L(L_ERROR, "[%s] framebuffer not valid", __FUNCTION__);
		return;
	}
	fb.Bind();
	fb.AttachTexture(atlasTex);
	if (!fb.CheckStatus(LOG_SECTION_DECALS_GL4)) {
		LOG_L(L_ERROR, "[%s] Couldn't render to FBO!", __FUNCTION__);
		return;
	}

	const int2 atlasSize = atlas.GetAtlasSize();

	glAttribStatePtr->ViewPort(0, 0, atlasSize.x, atlasSize.y);

	glAttribStatePtr->ClearColor(0.0f, 0.0f, 0.0f, 0.0f); // transparent black
	glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_ONE, GL_ZERO);
	glAttribStatePtr->DisableDepthTest();


	{
		GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
		Shader::IProgramObject* shader = buffer->GetShader();

		assert(!decalShader->IsBound());
		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj(0.0f, atlasSize.x, 0.0f, atlasSize.y, -1.0f, 1.0f, globalRendering->supportClipSpaceControl * 1.0f));

		for (auto& p: textures) {
			if (p.second.id == 0)
				continue;

			const float4 texCoords = atlas.GetTexCoords(p.first);
			const float4 absCoords = atlas.GetEntry(p.first);
			atlasTexs[p.first] = SAtlasTex(texCoords);

			glBindTexture(GL_TEXTURE_2D, p.second.id);

			// NB: TC, but z=0 to emulate 2DTC
			buffer->SafeAppend({{absCoords.x       , absCoords.y       , 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}});
			buffer->SafeAppend({{absCoords.z + 1.0f, absCoords.y       , 0.0f}, 1.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // FIXME: why +1?
			buffer->SafeAppend({{absCoords.x       , absCoords.w + 1.0f, 0.0f}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}});
			buffer->SafeAppend({{absCoords.z + 1.0f, absCoords.w + 1.0f, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}); // FIXME: why +1?
			buffer->Submit(GL_TRIANGLE_STRIP);

			glDeleteTextures(1, &p.second.id);
		}

		shader->Disable();
	}


	fb.Unbind();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, atlasTex);
	glGenerateMipmap(GL_TEXTURE_2D);

#ifdef DEBUG_SAVE_ATLAS
	glSaveTexture(atlasTex, "x_decal_atlas.png");
#endif
}


void CDecalsDrawerGL4::CreateBoundingBoxVBOs()
{
	constexpr float vertices[] = {
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,

		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
	};
	constexpr GLubyte indices[] = {
		0, 1, 2,  3, 2, 1, // back
		2, 3, 4,  5, 4, 3, // right
		4, 5, 6,  7, 6, 5, // front
		6, 7, 0,  1, 0, 7, // left
		0, 4, 6,  0, 2, 4, // btm
		1, 5, 3,  1, 7, 5, // top
	};

	// upload
	bboxArray.Bind();
	bboxVerts.Bind(GL_ARRAY_BUFFER);
	bboxIndcs.Bind(GL_ELEMENT_ARRAY_BUFFER);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float3), VA_TYPE_OFFSET(float, 0));

	bboxVerts.New(sizeof(vertices), GL_STATIC_DRAW, &vertices[0]);
	bboxIndcs.New(sizeof(indices), GL_STATIC_DRAW, &indices[0]);

	bboxArray.Unbind();
	bboxVerts.Unbind();
	bboxIndcs.Unbind();

	glDisableVertexAttribArray(0);
}


void CDecalsDrawerGL4::CreateStructureVBOs()
{
	{
		// Decal Groups UBO
		GLsizei uniformBlockSize = 0;
		GLuint uniformBlockIndex = glGetUniformBlockIndex(decalShader->GetObjID(), "decalGroupsUBO");
		glGetActiveUniformBlockiv(decalShader->GetObjID(), uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);

		uboDecalGroups.Bind(GL_UNIFORM_BUFFER);
		uboDecalGroups.New(uniformBlockSize, GL_DYNAMIC_DRAW);
		uboDecalGroups.Unbind();

		glUniformBlockBinding(decalShader->GetObjID(), uniformBlockIndex, 4);
		glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboDecalGroups.GetId());
	}

	{
		// Decals UBO
		GLint uniformBlockSize = 0;
		GLuint uniformBlockIndex = 0;
		if (useSSBO) {
			uniformBlockIndex = glGetProgramResourceIndex(decalShader->GetObjID(), GL_SHADER_STORAGE_BLOCK, "decalsUBO");
			constexpr GLenum props[] = {GL_BUFFER_DATA_SIZE};
			glGetProgramResourceiv(decalShader->GetObjID(),
				GL_SHADER_STORAGE_BLOCK, uniformBlockIndex,
				1, props,
				1, nullptr, &uniformBlockSize);
		} else {
			uniformBlockIndex = glGetUniformBlockIndex(decalShader->GetObjID(), "decalsUBO");
			glGetActiveUniformBlockiv(decalShader->GetObjID(), uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);
		}

		uboDecalsStructures.Bind(GL_UNIFORM_BUFFER);
		uboDecalsStructures.New(uniformBlockSize, GL_DYNAMIC_DRAW);
		uboDecalsStructures.Unbind();

		if (useSSBO) {
			glShaderStorageBlockBinding(decalShader->GetObjID(), uniformBlockIndex, 3);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, uboDecalsStructures.GetId());
		} else {
			glUniformBlockBinding(decalShader->GetObjID(), uniformBlockIndex, 3);
			glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboDecalsStructures.GetId());
		}
	}
}


void CDecalsDrawerGL4::ViewResize()
{
	decalShader->Enable();
	decalShader->SetUniform("invScreenSize", 1.0f / globalRendering->viewSizeX, 1.0f / globalRendering->viewSizeY);
	decalShader->Disable();

	glBindTexture(GL_TEXTURE_2D, depthTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
}


void CDecalsDrawerGL4::SunChanged()
{
	// Ground Lighting UBO
	GLuint uniformBlockIndex = glGetUniformBlockIndex(decalShader->GetObjID(), "SGroundLighting");
	assert(uniformBlockIndex != GL_INVALID_INDEX);

	GLsizei uniformBlockSize = 0;
	glGetActiveUniformBlockiv(decalShader->GetObjID(), uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);

	//FIXME
	if (uniformBlockSize != sizeof(SGLSLGroundLighting))
		LOG("uniformBlockSize sizeof(SGLSLGroundLighting) %u " _STPF_, uniformBlockSize, sizeof(SGLSLGroundLighting));

	assert(uniformBlockSize == sizeof(SGLSLGroundLighting));

	uboGroundLighting.Bind(GL_UNIFORM_BUFFER);
	uboGroundLighting.New(uniformBlockSize, GL_STATIC_DRAW);
		SGLSLGroundLighting* uboGroundLightingData = (SGLSLGroundLighting*) uboGroundLighting.MapBuffer(0, sizeof(SGLSLGroundLighting));
		uboGroundLightingData->ambientColor  = sunLighting->groundAmbientColor  * CGlobalRendering::SMF_INTENSITY_MULT;
		uboGroundLightingData->diffuseColor  = sunLighting->groundDiffuseColor  * CGlobalRendering::SMF_INTENSITY_MULT;
		uboGroundLightingData->specularColor = sunLighting->groundSpecularColor * CGlobalRendering::SMF_INTENSITY_MULT;
		uboGroundLightingData->dir           = sky->GetLight()->GetLightDir();
		uboGroundLightingData->fogColor      = sky->fogColor;
		uboGroundLightingData->fogEnd        = camera->GetFarPlaneDist() * sky->fogEnd;
		uboGroundLightingData->fogScale      = 1.0f / (camera->GetFarPlaneDist() * (sky->fogEnd - sky->fogStart));
		uboGroundLighting.UnmapBuffer();
	glUniformBlockBinding(decalShader->GetObjID(), uniformBlockIndex, 5);
	glBindBufferBase(GL_UNIFORM_BUFFER, 5, uboGroundLighting.GetId());
	uboGroundLighting.Unbind();
}










//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CDecalsDrawerGL4::AnyDecalsInView() const
{
	for (const Decal& d: decals) {
		if (d.InView())
			return true;
	}
	return false;
}


void CDecalsDrawerGL4::Draw()
{
	trackHandler.Draw(nullptr);

	if (!GetDrawDecals())
		return;

	if (!AnyDecalsInView())
		return;

	// disable parallax, when we are lagging for a while
	/*static bool wi = true;
	const bool setTimerQuery = drawTimerQuery.Available() || wi;
	if (setTimerQuery) {
		wi = false;
		const float lastDrawTime = drawTimerQuery.QueryMilliSeconds();
		static float avgfoo[2] = {0,0};
		float& f = avgfoo[decalShader->GetFlagBool("USE_PARALLAX")];
		f = mix(f, lastDrawTime, 0.2f);
		LOG_L(L_ERROR, "CDecalsDrawerGL4::Update %fms %fms", avgfoo[0], avgfoo[1]);
		if (lastDrawTime > 5.f) {
			++laggedFrames;
			if (laggedFrames > 30) {
				LOG_L(L_ERROR, "Disabling Parallax decals");
				decalShader->SetFlag("USE_PARALLAX", false);
			}
		} else {
			laggedFrames = 0;
		}
		drawTimerQuery.Start();
	}*/

	// we render the decals in screen space
	// for that we need the depth, to be able to reconstruct the worldpos
#ifdef glCopyTextureSubImage2D
	if (GLEW_EXT_direct_state_access) {
		glCopyTextureSubImage2D(depthTex, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	} else
#endif
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTex);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_ONE, GL_SRC_ALPHA);
	glAttribStatePtr->EnableCullFace();
	glAttribStatePtr->CullFace(GL_FRONT);
	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->EnableDepthClamp();
	// glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->DisableDepthTest();


	const CMatrix44f& viewProjMat    = camera->GetViewProjectionMatrix();
	      CMatrix44f  viewProjMatInv = camera->GetViewProjectionMatrixInverse();

	const std::array<GLuint, 5> textures = {
		atlasTex,
		static_cast<CSMFReadMap*>(readMap)->GetNormalsTexture(),
		shadowHandler.GetShadowTextureID(),
		infoTextureHandler->GetCurrentInfoTexture(),
		depthTex
	};

	decalShader->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	decalShader->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());
	decalShader->Enable();
		decalShader->SetUniform3v("camPos", &camera->GetPos()[0]);
		decalShader->SetUniform3v("camDir", &camera->GetDir()[0]);

		viewProjMatInv.Translate(-OnesVector);
		viewProjMatInv.Scale(OnesVector * 2.0f);

		decalShader->SetUniformMatrix4x4("viewProjMatrix", false, viewProjMat.m);
		decalShader->SetUniformMatrix4x4("viewProjMatrixInv", false, viewProjMatInv.m);

	glSpringBindTextures(0, textures.size(), &textures[0]);

	if (shadowHandler.ShadowsLoaded()) {
		decalShader->SetUniformMatrix4x4("shadowMatrix", false, shadowHandler.GetShadowViewMatrixRaw());
		decalShader->SetUniform("shadowDensity", sunLighting->groundShadowDensity);
	}

	// Draw
	DrawDecals();

	glAttribStatePtr->DisableDepthClamp();
	glAttribStatePtr->EnableDepthTest();
	glAttribStatePtr->DisableCullFace();
	glAttribStatePtr->CullFace(GL_BACK);
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAttribStatePtr->DisableBlendMask();

	decalShader->Disable();
	//if (setTimerQuery) drawTimerQuery.Stop();
}


void CDecalsDrawerGL4::DrawDecals()
{
	bboxArray.Bind();
	glDrawElementsInstanced(GL_TRIANGLES, bboxIndcs.GetSize(), GL_UNSIGNED_BYTE, bboxIndcs.GetPtr(), groups.size());
	bboxArray.Unbind();
}


void CDecalsDrawerGL4::UpdateDecalsVBO()
{
	if (decalsToUpdate.empty())
		return;


	uboDecalGroups.Bind(GL_UNIFORM_BUFFER);
		GLubyte* uboData = uboDecalGroups.MapBuffer(0, sizeof(SDecalGroup) * groups.size());
		SDecalGroup* uboScarsData = (SDecalGroup*) uboData;
		memcpy(uboScarsData, &groups[0], sizeof(SDecalGroup) * groups.size());
		uboDecalGroups.UnmapBuffer();
	uboDecalGroups.Unbind();
	glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboDecalGroups.GetId());


	uboDecalsStructures.Bind(GL_UNIFORM_BUFFER);
		for (int idx: decalsToUpdate) {
			const Decal& d = decals[idx];

			GLubyte* uboData = uboDecalsStructures.MapBuffer(sizeof(SGLSLDecal) * idx, sizeof(SGLSLDecal) * 1);
			SGLSLDecal* uboScarsData = (SGLSLDecal*) uboData;

			uboScarsData[0].pos        = d.pos;
			uboScarsData[0].alpha      = d.alpha;
			uboScarsData[0].size       = float2(1.0f / d.size.x, 1.0f / d.size.y);
			uboScarsData[0].rotMatElements = float2(std::cos(d.rot), std::sin(d.rot));
			uboScarsData[0].texOffsets = d.texOffsets;
			uboScarsData[0].texNormalOffsets = d.texNormalOffsets;
			uboDecalsStructures.UnmapBuffer();
		}
		decalsToUpdate.clear();
	uboDecalsStructures.Unbind();

	if (useSSBO) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, uboDecalsStructures.GetId());
	} else {
		glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboDecalsStructures.GetId());
	}
}


void CDecalsDrawerGL4::Update()
{
	SCOPED_TIMER("Update::DecalsDrawerGL4");
	UpdateOverlap();
	OptimizeGroups();
	UpdateDecalsVBO();
}


void CDecalsDrawerGL4::GameFrame(int n)
{
	SCOPED_TIMER("Sim::GameFrame::DecalsDrawerGL4");

	for (int idx: alphaDecayingDecals) {
		Decal& d = decals[idx];
		assert((d.owner == nullptr) && (d.type == Decal::BUILDING));

		if ((d.alpha -= d.alphaFalloff) > 0.0f) {
			decalsToUpdate.push_back(idx);
		} else {
			FreeDecal(idx);
		}
	}
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline std::array<float2, 4> GetEdgePoinsOfDecal(const CDecalsDrawerGL4::Decal& d)
{
	CMatrix44f m;
	m.RotateY(d.rot);
	auto f3tof2 = [](float3 f3) { return float2(f3.x, f3.z); };

	return {
		f3tof2(d.pos + m * float3(-d.size.x, 0.f, -d.size.y)),
		f3tof2(d.pos + m * float3(-d.size.x, 0.f,  d.size.y)),
		f3tof2(d.pos + m * float3( d.size.x, 0.f,  d.size.y)),
		f3tof2(d.pos + m * float3( d.size.x, 0.f, -d.size.y))
	};
}


static inline bool Overlap(const CDecalsDrawerGL4::SDecalGroup& g1, const CDecalsDrawerGL4::SDecalGroup& g2)
{
	const float3 hsize1 = (g1.boundAABB[1] - g1.boundAABB[0]) * 0.5f;
	const float3 mid1   = g1.boundAABB[0] + hsize1;
	const float3 hsize2 = (g2.boundAABB[1] - g2.boundAABB[0]) * 0.5f;
	const float3 mid2   = g2.boundAABB[0] + hsize2;
	const float3 diff   = mid1 - mid2;

	if (std::abs(diff.x) > (hsize1.x + hsize2.x))
		return false;
	if (std::abs(diff.z) > (hsize1.z + hsize2.z))
		return false;
	return true;
}


static inline bool Overlap(const CDecalsDrawerGL4::Decal& d1, const CDecalsDrawerGL4::Decal& d2)
{
	const float hsize1 = std::max(d1.size.x, d1.size.y) * math::SQRT2;
	const float hsize2 = std::max(d2.size.x, d2.size.y) * math::SQRT2;
	const float3 diff  = d1.pos - d2.pos;

	if (std::abs(diff.x) > (hsize1 + hsize2))
		return false;
	if (std::abs(diff.z) > (hsize1 + hsize2))
		return false;
	return true;
}

static inline bool Overlap(const CDecalsDrawerGL4::SDecalGroup& g, const CDecalsDrawerGL4::Decal& d)
{
	const float3 hsize1 = (g.boundAABB[1] - g.boundAABB[0]) * 0.5f;
	const float3 mid1   = g.boundAABB[0] + hsize1;
	const float hsize2  = std::max(d.size.x, d.size.y) * math::SQRT2;
	const float3 diff   = mid1 - d.pos;

	if (std::abs(diff.x) > (hsize1.x + hsize2))
		return false;
	if (std::abs(diff.z) > (hsize1.z + hsize2))
		return false;
	return true;
}


void CDecalsDrawerGL4::UpdateBoundingBox(SDecalGroup& g)
{
	if (g.ids.front() == 0) {
		g.boundAABB.fill(float4());
		return;
	}

	// compute the AABB
	float3 mins(1e9,1e9,1e9), maxs(-1e9,-1e9,-1e9);
	for (int idx: g.ids) {
		if (idx == 0)
			break;

		Decal& d = decals[idx];
		const auto e = GetEdgePoinsOfDecal(d);
		for (const float2& p: e) {
			mins = float3::min(mins, float3(p.x, d.pos.y - d.size.x, p.y));
			maxs = float3::max(maxs, float3(p.x, d.pos.y + d.size.x, p.y));
		}
	}
	mins.y = std::max(mins.y, readMap->GetCurrMinHeight());
	maxs.y = std::min(maxs.y, readMap->GetCurrMaxHeight());
	g.boundAABB = {{ float4(mins, 0.f), float4(maxs, 0.f) }};
}


bool CDecalsDrawerGL4::TryToCombineDecalGroups(SDecalGroup& g1, SDecalGroup& g2)
{
	// g1 is full -> we cannot move anything from g2 to it
	if (g1.ids.back() != 0)
		return false;

	if (!Overlap(g1, g2))
		return false;


	if ((g1.size() + g2.size()) <= CDecalsDrawerGL4::MAX_DECALS_PER_GROUP) {
		// move all g2 decals to g1 (and remove g2 later)
		for (int n = g1.size(), k = 0; n < g1.ids.size(); ++n,++k) {
			g1.ids[n] = g2.ids[k];
		}
		g2.ids.fill(0);

		UpdateBoundingBox(g1);
		//UpdateBoundingBox(g2); gets removed, no reason to update
		return true;
	}


	// move n decals from g2 to g1
	const int s = g1.size();
	const int n = g1.ids.size() - s;
	memcpy(&g1.ids[s], &g2.ids[0], sizeof(int) * n);
	for (int i = 0; i<g2.ids.size(); ++i) {
		g2.ids[i] = ((n + i) < g2.ids.size()) ? g2.ids[n + i] : 0;
	}
	UpdateBoundingBox(g1);
	UpdateBoundingBox(g2);
	return false;
}


bool CDecalsDrawerGL4::AddDecalToGroup(SDecalGroup& g, const Decal& d, const int decalIdx)
{
	if (g.ids.back() != 0)
		return false;

	if (!Overlap(g, d))
		return false;

	auto it = spring::find(g.ids, 0);
	*it = decalIdx;
	UpdateBoundingBox(g);
	return true;
}


bool CDecalsDrawerGL4::FindAndAddToGroup(int decalIdx)
{
	// first check if there is an existing fitting group
	Decal& d = decals[decalIdx];
	for (SDecalGroup& g: groups) {
		if (AddDecalToGroup(g, d, decalIdx)) {
			return true;
		}
	}

	// no fitting one found, add new group
	if (groups.size() >= maxDecalGroups)
		return false;
	groups.emplace_back();
	SDecalGroup& g = groups.back();
	g.ids.fill(0);
	g.ids[0] = decalIdx;
	UpdateBoundingBox(g);
	return true;
}


void CDecalsDrawerGL4::RemoveFromGroup(int idx)
{
	for (auto it = groups.begin(); it != groups.end(); ++it) {
		SDecalGroup& g = *it;
		if (std::remove(g.ids.begin(), g.ids.end(), idx) != g.ids.end()) {
			g.ids.back() = 0;
			UpdateBoundingBox(g);
			if (g.ids.front() == 0) {
				groups.erase(it);
			}
			return;
		}

	}
}


void CDecalsDrawerGL4::OptimizeGroups()
{
	static int updates = 0;
	updates += decalsToUpdate.size();
	if (updates <= (MAX_DECALS_PER_GROUP * 4))
		return;
	if (overlapStage != 0)
		return;
	updates = 0;

	for (auto it = groups.begin(); it != groups.end(); ++it) {
		SDecalGroup& g1 = *it;

		for (auto jt = it + 1; jt != groups.end(); ) {
			auto& g2 = *jt;
			if (TryToCombineDecalGroups(g1, g2)) {
				jt = groups.erase(jt);
			} else {
				++jt;
			}
		}
	}
}


static void DRAW_DECAL(const CDecalsDrawerGL4::Decal* d, GL::RenderDataBufferTC* rdb)
{
	CMatrix44f m;
	m.Translate(d->pos.x, d->pos.z, 0.0f);
	m.RotateZ(d->rot * math::DEG_TO_RAD);

	float2 dsize = d->size;
	// make sure it is at least 1x1 pixels!
	dsize.x = std::max(dsize.x, std::ceil( float(mapDims.mapx * SQUARE_SIZE) / CDecalsDrawerGL4::OVERLAP_TEST_TEXTURE_SIZE ));
	dsize.y = std::max(dsize.y, std::ceil( float(mapDims.mapy * SQUARE_SIZE) / CDecalsDrawerGL4::OVERLAP_TEST_TEXTURE_SIZE ));

	const float3 ds1 = float3(dsize.x,  dsize.y, 0.0f);
	const float3 ds2 = float3(dsize.x, -dsize.y, 0.0f);
	const float4 tc  = d->texOffsets;
	const SColor  c  = {1.0f, 1.0f, 1.0f, 1.0f};

	rdb->SafeAppend({m * -ds1, tc[0], tc[1], c}); // bl
	rdb->SafeAppend({m *  ds2, tc[2], tc[1], c}); // br
	rdb->SafeAppend({m *  ds1, tc[2], tc[3], c}); // tr

	rdb->SafeAppend({m *  ds1, tc[2], tc[3], c}); // tr
	rdb->SafeAppend({m * -ds2, tc[0], tc[3], c}); // tl
	rdb->SafeAppend({m * -ds1, tc[0], tc[1], c}); // bl
}


void CDecalsDrawerGL4::UpdateOverlap()
{
	//FIXME generation

	//FIXME comment!
	if (!fboOverlap.IsValid())
		return;

	std::vector<int> candidatesForOverlap = UpdateOverlap_PreCheck();
	if ((overlapStage == 0) && candidatesForOverlap.empty())
		return; // early-exit before GL stuff is done

	fboOverlap.Bind();
	glAttribStatePtr->PushBits(GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glAttribStatePtr->ViewPort(0, 0, OVERLAP_TEST_TEXTURE_SIZE, OVERLAP_TEST_TEXTURE_SIZE);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTex);

	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->DisableDepthTest();
	glAttribStatePtr->EnableStencilTest();
	glAttribStatePtr->StencilMask(0xFF);


	{
		GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
		Shader::IProgramObject* shader = buffer->GetShader();

		assert(!decalShader->IsBound());
		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj(0.0f, mapDims.mapx * SQUARE_SIZE, 0.0f, mapDims.mapy * SQUARE_SIZE, -1.0f, 1.0f, globalRendering->supportClipSpaceControl * 1.0f));
		shader->SetUniform("u_alpha_test_ctrl", 0.0f, 1.0f, 0.0f, 0.0f); // test > 0.0


		// either initialize or check queries
		switch (overlapStage) {
			case 0: {
				UpdateOverlap_Initialize(buffer);
				overlapStage = 1;
			} break;
			case 1: {
				candidatesForOverlap = UpdateOverlap_CheckQueries(buffer);
			} break;
			default: overlapStage = 0;
		}

		// generate queries for next frame/call
		if (candidatesForOverlap.empty()) {
			assert(waitingOverlapGlQueries.empty());
			overlapStage = 0;
		} else {
			UpdateOverlap_GenerateQueries(candidatesForOverlap, buffer);
		}

		shader->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
		shader->Disable();
	}

	glAttribStatePtr->PopBits();
	//fboOverlap.Unbind();
}


std::vector<int> CDecalsDrawerGL4::UpdateOverlap_PreCheck()
{
	std::vector<int> candidatesForOverlap;

	// we are in a test already, check if one of the tested decals changed, if so we need to restart the test
	if ((overlapStage == 1) && !waitingDecalsForOverlapTest.empty()) {
		for (auto& p: waitingOverlapGlQueries) {
			auto it = spring::find(waitingDecalsForOverlapTest, p.first);
			if (it == waitingDecalsForOverlapTest.end())
				continue;

			// one of the decals changed, we need to restart the overlap test
			overlapStage = 0;
			for (auto& p: waitingOverlapGlQueries) {
				glDeleteQueries(1, &p.second);
				waitingDecalsForOverlapTest.push_back(p.first);
			}
			waitingOverlapGlQueries.clear();
			break;
		}
	}

	// not enough changed decals -> wait further
	if ((overlapStage == 0) && (waitingDecalsForOverlapTest.size() <= (MAX_OVERLAP * 4)))
		return candidatesForOverlap;

	// only test those decals which overlap with others and add them to the vector
	if (overlapStage == 0) {
		candidatesForOverlap = CandidatesForOverlap();
		waitingDecalsForOverlapTest.clear();
	}

	return candidatesForOverlap;
}


std::vector<int> CDecalsDrawerGL4::CandidatesForOverlap() const
{
	std::vector<int> candidatesForOverlap;
	std::vector<bool> decalsInserted(decals.size(), false);

	for (int i: waitingDecalsForOverlapTest) {
		const Decal& d1 = decals[i];

		if (!d1.IsValid())
			continue;

		for (int j = 0; j < decals.size(); ++j) {
			const Decal& d2 = decals[j];

			if (!d2.IsValid())
				continue;

			if (decalsInserted[i] && decalsInserted[j])
				continue;

			if (i == j)
				continue;

			if (!Overlap(d1, d2))
				continue;

			if (!decalsInserted[i]) {
				decalsInserted[i] = true;
				candidatesForOverlap.push_back(i);
			}
			if (!decalsInserted[j]) {
				decalsInserted[j] = true;
				candidatesForOverlap.push_back(j);
			}
		}
	}

	return candidatesForOverlap;
}


void CDecalsDrawerGL4::UpdateOverlap_Initialize(GL::RenderDataBufferTC* rdb)
{
	glAttribStatePtr->ClearStencil(0);
	glAttribStatePtr->Clear(GL_STENCIL_BUFFER_BIT);
	// glAttribStatePtr->ClearColor(0.0f, 0.5f, 0.0f, 1.0f);
	// glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);

	glAttribStatePtr->StencilFunc(GL_ALWAYS, 0, 0xFF);
	glAttribStatePtr->StencilOper(GL_INCR_WRAP, GL_INCR_WRAP, GL_INCR_WRAP);

	{
		for (const Decal& d: decals) {
			DRAW_DECAL(&d, rdb);
		}

		rdb->Submit(GL_TRIANGLES);
	}

	#if 0
	{
		CBitmap bitmap;
		bitmap.Alloc(OVERLAP_TEST_TEXTURE_SIZE, OVERLAP_TEST_TEXTURE_SIZE, 4);
		glReadPixels(0, 0, OVERLAP_TEST_TEXTURE_SIZE, OVERLAP_TEST_TEXTURE_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.GetRawMem());
		bitmap.Save("decals.png", true);
	}
	#endif
}


void CDecalsDrawerGL4::UpdateOverlap_GenerateQueries(const std::vector<int>& candidatesForOverlap, GL::RenderDataBufferTC* rdb)
{
	glAttribStatePtr->StencilFunc(GL_GREATER, MAX_OVERLAP, 0xFF);
	glAttribStatePtr->StencilOper(GL_KEEP, GL_KEEP, GL_KEEP);


	for (int i: candidatesForOverlap) {
		GLuint q;
		glGenQueries(1, &q);
		glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, q);

		DRAW_DECAL(&decals[i], rdb);
		rdb->Submit(GL_TRIANGLES);

		glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
		waitingOverlapGlQueries.emplace_back(i, q);
	}
}


std::vector<int> CDecalsDrawerGL4::UpdateOverlap_CheckQueries(GL::RenderDataBufferTC* rdb)
{
	// query the results of the last issued test (if any pixels were rendered for the decal)
	std::vector<int> candidatesForRemoval;
	candidatesForRemoval.reserve(waitingOverlapGlQueries.size());
	for (auto& p: waitingOverlapGlQueries) {
		GLint rendered = GL_FALSE;
		glGetQueryObjectiv(p.second, GL_QUERY_RESULT, &rendered);
		glDeleteQueries(1, &p.second);

		if (rendered != GL_FALSE)
			continue;

		candidatesForRemoval.push_back(p.first);
	}
	waitingOverlapGlQueries.clear();

	// no candidates left, restart with stage 0
	if (candidatesForRemoval.empty()) {
		overlapStage = 0;
		return candidatesForRemoval;
	}


	//FIXME comment
	auto sortBeginIt = candidatesForRemoval.begin();
	auto sortEndIt = candidatesForRemoval.end();

	while (sortBeginIt != sortEndIt) {
		std::sort(sortBeginIt, sortEndIt, [&](const int& idx1, const int& idx2) {
			return decals[idx1].generation < decals[idx2].generation;
		});
		sortEndIt = std::partition(sortBeginIt+1, sortEndIt, [&](const int& idx) {
			return !Overlap(decals[*sortBeginIt], decals[idx]);
		});
		++sortBeginIt;
	}

	auto eraseIt = sortEndIt;


	// free one decal (+ remove it from the overlap texture)
	int numFreedDecals = 0;

	glAttribStatePtr->StencilFunc(GL_ALWAYS, 0, 0xFF);
	glAttribStatePtr->StencilOper(GL_DECR_WRAP, GL_DECR_WRAP, GL_DECR_WRAP);


	for (auto it = candidatesForRemoval.begin(); it != eraseIt; ++it) {
		const int curIndex = *it;

		DRAW_DECAL(&decals[curIndex], rdb);
		FreeDecal(curIndex);

		numFreedDecals++;
	}

	rdb->Submit(GL_TRIANGLES);


	candidatesForRemoval.erase(candidatesForRemoval.begin(), eraseIt);

	//FIXME
	LOG_L(L_ERROR, "Query freed: %i of %i (decals:%i groups:%i)", numFreedDecals, int(candidatesForRemoval.size()), int(decals.size() - freeIds.size()), int(groups.size()));

	// overlap texture changed, so need to retest the others
	return candidatesForRemoval;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDecalsDrawerGL4::GetWorstRatedDecal(int* idx, float* rating, const bool inview_test) const
{
	int   i = 0;
	float r = 1e9;
	for (int idx2 = 1; idx2 < decals.size(); ++idx2) {
		if (!decals[idx2].IsValid())
			continue;
		const float r2 = decals[idx2].GetRating(inview_test);
		if (r2 < r) {
			r = r2;
			i = idx2;
		}
	}
	*idx = i;
	*rating = r;
}


int CDecalsDrawerGL4::CreateLuaDecal()
{
	//SCOPED_TIMER("DecalsDrawerGL4::Update");

	if (freeIds.empty()) { // try to make space for new one
		// current worst decal is inview, try to find a better one (at best outside of view)
		if ((curWorstDecalIdx == 0) || decals[curWorstDecalIdx].InView()) {
			GetWorstRatedDecal(&curWorstDecalIdx, &curWorstDecalRating, true);
		}

		if (curWorstDecalIdx == 0)
			return 0;

		FreeDecal(curWorstDecalIdx);
	}

	int idx = freeIds.back();
	freeIds.pop_back();
	decals[idx] = Decal();
	decals[idx].type = Decal::LUA;
	return idx;
}


int CDecalsDrawerGL4::NewDecal(const Decal& d)
{
	//SCOPED_TIMER("DecalsDrawerGL4::Update");

	if (freeIds.empty()) {
		// early-exit: all decals are `better` than the new one -> don't add
		const float r = d.GetRating(true);
		if (r < curWorstDecalRating) {
			return 0;
		}

		// try to make space for new one
		if ((curWorstDecalIdx == 0) || decals[curWorstDecalIdx].InView()) {
			// current worst decal is inview, try to find a better one (at best outside of view)
			GetWorstRatedDecal(&curWorstDecalIdx, &curWorstDecalRating, true);
		}

		if (r >= curWorstDecalRating) {
			FreeDecal(curWorstDecalIdx);
		}

		if (freeIds.empty()) {
			return 0;
		}
	}

	int idx = freeIds.back();
	freeIds.pop_back();
	decals[idx] = d;
	if (!FindAndAddToGroup(idx)) {
		FreeDecal(idx);
		return 0;
	}
	decalsToUpdate.push_back(idx);
	waitingDecalsForOverlapTest.push_back(idx);

	const float r = d.GetRating(false);
	if (r < curWorstDecalRating) {
		curWorstDecalRating = r;
		curWorstDecalIdx = idx;
	}

	return idx;
}


void CDecalsDrawerGL4::FreeDecal(int idx)
{
	if (idx == 0)
		return;

	//SCOPED_TIMER("DecalsDrawerGL4::Update");

	Decal& d = decals[idx];
	if ((d.owner == nullptr) && (d.type == Decal::BUILDING)) {
		auto it = spring::find(alphaDecayingDecals, idx);
		assert(it != alphaDecayingDecals.end());
		alphaDecayingDecals.erase(it);
	}

	d = Decal();
	decalsToUpdate.push_back(idx);
	freeIds.push_back(idx);



	// we were the worst rated decal? -> find new one
	if (idx == curWorstDecalIdx) {
		GetWorstRatedDecal(&curWorstDecalIdx, &curWorstDecalRating, false);
	}

	RemoveFromGroup(idx);
}


CDecalsDrawerGL4::Decal& CDecalsDrawerGL4::GetDecalOwnedBy(const void* owner)
{
	for (auto& d: decals) {
		if (d.owner == owner)
			return d;
	}
	return decals[0]; // [0] is an invalid idx!
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline bool HasGroundDecalDef(const CSolidObject* object)
{
	return object->GetDef()->decalDef.useGroundDecal;
}


static inline bool ExplosionInAirLos(const CExplosionParams& event)
{
	const auto proj = projectileHandler.GetProjectileBySyncedID(event.projectileID);
	if (proj != nullptr) {
		if (teamHandler.ValidAllyTeam(proj->GetAllyteamID()) && teamHandler.AlliedAllyTeams(gu->myAllyTeam, proj->GetAllyteamID()))
			return true;
	}

	if (losHandler->InAirLos(event.pos, gu->myAllyTeam))
		return true;

	if (event.craterAreaOfEffect > 50) {
		// check multiple (random) points for larger explosions
		const auto p = event.pos;
		const auto r = event.craterAreaOfEffect;
		for (int i = r * 0.3f; i >= 0; --i) {
			if (losHandler->InAirLos(p + (guRNG.NextVector2D() * r), gu->myAllyTeam))
				return true;
		}
	}
	return false;
}


void CDecalsDrawerGL4::AddExplosion(float3 pos, float damage, float radius)
{
	const float altitude = pos.y - CGround::GetHeightReal(pos.x, pos.z, false);

	// no decals for below-ground & in-air explosions
	if (std::abs(altitude) > radius) { return; }

	pos.y  -= altitude;
	radius -= altitude;

	damage = std::min(damage, radius * 30.0f);
	damage *= (radius) / (radius + altitude);
	radius = std::min(radius, damage * 0.25f);

	if (radius <= 0.0f) { return; }

	Decal d;
	d.pos    = pos;
	d.rot    = guRNG.NextFloat() * math::TWOPI;
	d.size.x = radius * math::SQRT2;
	d.size.y = d.size.x;
	d.alpha  = Clamp(damage / 255.0f, 0.75f, 1.0f);
	d.SetTexture(IntToString((guRNG.NextInt() & 3) + 1)); // pick one of 4 scar textures
	d.type  = Decal::EXPLOSION;
	d.owner = nullptr;
	d.generation = gs->frameNum;

	NewDecal(d);
}


void CDecalsDrawerGL4::CreateBuildingDecal(const CSolidObject* object)
{
	const SolidObjectDecalDef& decalDef = object->GetDef()->decalDef;
	if (!decalDef.useGroundDecal)
		return;

	const int sizex = decalDef.groundDecalSizeX;
	const int sizey = decalDef.groundDecalSizeY;

	Decal d;
	d.pos   = object->pos;
	d.rot   = GetRadFromXY(object->frontdir.x, object->frontdir.z) - math::HALFPI;
	d.size  = float2(sizex * SQUARE_SIZE, sizey * SQUARE_SIZE);
	d.alpha = 1.0f;
	d.alphaFalloff = decalDef.groundDecalDecaySpeed;
	d.SetTexture(decalDef.groundDecalTypeName);
	d.type  = Decal::BUILDING;
	d.owner = object;
	d.generation = gs->frameNum;

	NewDecal(d);
}


void CDecalsDrawerGL4::DeownBuildingDecal(const CSolidObject* object)
{
	if (!HasGroundDecalDef(object)) return;
	GetDecalOwnedBy(object).SetOwner(nullptr);
}


void CDecalsDrawerGL4::ForceRemoveSolidObject(CSolidObject* object)
{
	FreeDecal(GetDecalOwnedBy(object).GetIdx());
}



void CDecalsDrawerGL4::ExplosionOccurred(const CExplosionParams& event) {
	if ((event.weaponDef != nullptr) && !event.weaponDef->visuals.explosionScar)
		return;

	if (!ExplosionInAirLos(event))
		return;

	AddExplosion(event.pos, event.damages.GetDefault(), event.craterAreaOfEffect);
}

void CDecalsDrawerGL4::RenderUnitCreated(const CUnit* unit, int cloaked) { CreateBuildingDecal(unit); }
void CDecalsDrawerGL4::RenderFeatureCreated(const CFeature* feature)     { CreateBuildingDecal(feature); }
void CDecalsDrawerGL4::RenderUnitDestroyed(const CUnit* unit)          { DeownBuildingDecal(unit); }
void CDecalsDrawerGL4::RenderFeatureDestroyed(const CFeature* feature) { DeownBuildingDecal(feature); }


void CDecalsDrawerGL4::GhostCreated(CSolidObject* object, GhostSolidObject* gb)
{
	if (!HasGroundDecalDef(object)) return;
	GetDecalOwnedBy(object).SetOwner(gb);
}

void CDecalsDrawerGL4::GhostDestroyed(GhostSolidObject* gb)
{
	GetDecalOwnedBy(gb).SetOwner(nullptr);
}





#endif // !defined(GL_VERSION_4_0) || HEADLESS
