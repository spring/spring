/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <vector>

#include "AdvTreeGenerator.h"

#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/myMath.h"

using std::max;
using std::min;

constexpr static unsigned int TEX_SIZE_C = 4;
constexpr static unsigned int TEX_SIZE_X = 256;
constexpr static unsigned int TEX_SIZE_Y = 256;

constexpr static unsigned int NUM_TREE_TYPES =    8; // FIXME: duplicated from ITreeDrawer.h
constexpr static unsigned int MAX_TREE_VERTS = 2048; // number of vertices per randomized subtype

// treeTexMem[y + ypos][x + xpos][i]
// static uint8_t treeTexMem[TEX_SIZE_Y][TEX_SIZE_X * 4 * 2][TEX_SIZE_C];

// treeTexMem[((y + ypos) * (TEX_SIZE_X * 4 * 2) + (x + xpos)) * 4 + i]
static uint8_t treeTexMem[TEX_SIZE_Y * TEX_SIZE_X * 4 * 2 * TEX_SIZE_C];


void CAdvTreeGenerator::Init()
{
	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);

	if (!resourcesParser.Execute())
		LOG_L(L_ERROR, "%s", resourcesParser.GetErrorLog().c_str());

	const LuaTable treesTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("trees");

	CBitmap bm;
	std::string fn;

	FBO fbo;
	fbo.Bind();
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT16, TEX_SIZE_X, TEX_SIZE_Y);
	fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA8, TEX_SIZE_X, TEX_SIZE_Y);

	if (!fbo.CheckStatus("ADVTREE")) {
		fbo.Unbind();
		throw content_error("Could not create FBO!");
	}


	std::memset(&treeTexMem[0], 128, TEX_SIZE_Y * (TEX_SIZE_X * 4 * 2) * TEX_SIZE_C);

	if (!CopyBarkTexMem(fn = "bitmaps/" + treesTable.GetString("bark", "Bark.bmp"))) {
		fbo.Unbind();
		throw content_error("Could not load bark-texture from file " + fn);
	}

	if (!GenBarkTextures(fn = "bitmaps/" + treesTable.GetString("leaf", "bleaf.bmp"))) {
		fbo.Unbind();
		throw content_error("Could not load leaf-texture from file " + fn);
	}

	fbo.Unbind();

	GenVertexBuffers();
}

CAdvTreeGenerator::~CAdvTreeGenerator()
{
	glDeleteTextures(1, &barkTex);

	glDeleteBuffers(1, &treeBuffers[1]);
	glDeleteBuffers(1, &treeBuffers[0]);
}


bool CAdvTreeGenerator::CopyBarkTexMem(const std::string& barkTexFile)
{
	CBitmap bmp;

	if (!bmp.Load(barkTexFile) || bmp.xsize != TEX_SIZE_X || bmp.ysize != TEX_SIZE_Y)
		return false;

	const uint8_t* srcMem = bmp.GetRawMem();

	for (int y = 0; y < TEX_SIZE_Y; y++) {
		for (int x = 0; x < TEX_SIZE_X; x++) {
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x                 ) * 4 + 0] = srcMem[(y * TEX_SIZE_X + x) * 4 + 0];
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x                 ) * 4 + 1] = srcMem[(y * TEX_SIZE_X + x) * 4 + 1];
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x                 ) * 4 + 2] = srcMem[(y * TEX_SIZE_X + x) * 4 + 2];
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x                 ) * 4 + 3] = 255;

			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x + TEX_SIZE_X * 4) * 4 + 0] = uint8_t(srcMem[(y * TEX_SIZE_X + x) * 4 + 0] * 0.6f);
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x + TEX_SIZE_X * 4) * 4 + 1] = uint8_t(srcMem[(y * TEX_SIZE_X + x) * 4 + 1] * 0.6f);
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x + TEX_SIZE_X * 4) * 4 + 2] = uint8_t(srcMem[(y * TEX_SIZE_X + x) * 4 + 2] * 0.6f);
			treeTexMem[(y * (TEX_SIZE_X * 4 * 2) + x + TEX_SIZE_X * 4) * 4 + 3] = 255;
		}
	}

	return true;
}

bool CAdvTreeGenerator::GenBarkTextures(const std::string& leafTexFile)
{
	CBitmap bmp;

	if (!bmp.Load(leafTexFile))
		return false;

	bmp.CreateAlpha(0, 0, 0);
	// bmp.Renormalize(float3(0.22f, 0.43f, 0.18f));

	GLuint leafTex = 0;
	glGenTextures(1, &leafTex);
	glBindTexture(GL_TEXTURE_2D, leafTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bmp.xsize, bmp.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bmp.GetRawMem());
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, leafTex);

	CreateLeafTex(&treeTexMem[0], TEX_SIZE_X * 1, 0, TEX_SIZE_X * 4 * 2);
	CreateLeafTex(&treeTexMem[0], TEX_SIZE_X * 2, 0, TEX_SIZE_X * 4 * 2);
	CreateLeafTex(&treeTexMem[0], TEX_SIZE_X * 3, 0, TEX_SIZE_X * 4 * 2);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &leafTex);

	CreateGranTex(&treeTexMem[0], TEX_SIZE_X * 4 + TEX_SIZE_X * 3, 0, TEX_SIZE_X * 4 * 2);
	CreateGranTex(&treeTexMem[0], TEX_SIZE_X * 5,                  0, TEX_SIZE_X * 4 * 2);
	CreateGranTex(&treeTexMem[0], TEX_SIZE_X * 6,                  0, TEX_SIZE_X * 4 * 2);

	glGenTextures(1, &barkTex);
	glBindTexture(GL_TEXTURE_2D, barkTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, TEX_SIZE_X * 4 * 2, TEX_SIZE_Y, GL_RGBA, GL_UNSIGNED_BYTE, &treeTexMem[0]);

	return true;
}

void CAdvTreeGenerator::GenVertexBuffers()
{
	std::array<VA_TYPE_TN, MAX_TREE_VERTS * NUM_TREE_TYPES> bushVerts;
	std::array<VA_TYPE_TN, MAX_TREE_VERTS * NUM_TREE_TYPES> pineVerts;

	bushVerts.fill({ZeroVector, 0.0f, 0.0f, ZeroVector});
	pineVerts.fill({ZeroVector, 0.0f, 0.0f, ZeroVector});

	// randomized bush-tree subtypes, packed stem- and leaf-data
	for (int a = 0; a < NUM_TREE_TYPES; ++a) {
		prvVertPtr = &bushVerts[0] + a * MAX_TREE_VERTS;
		curVertPtr = &bushVerts[0] + a * MAX_TREE_VERTS;

		{
			const float numBranches = 10.0f;
			const float treeScale = 0.65f + 0.2f * guRNG.NextFloat();

			GenBushTree(int(numBranches), treeScale * MAX_TREE_HEIGHT, treeScale * 0.05f * MAX_TREE_HEIGHT);
		}

		numTreeVerts[a] = curVertPtr - prvVertPtr;
	}

	// randomized pine-tree subtypes, ditto
	for (int a = 0; a < NUM_TREE_TYPES; ++a) {
		prvVertPtr = &pineVerts[0] + a * MAX_TREE_VERTS;
		curVertPtr = &pineVerts[0] + a * MAX_TREE_VERTS;

		{
			const float numBranches = 20.0f + 10.0f * guRNG.NextFloat();
			const float treeScale = 0.7f + 0.2f * guRNG.NextFloat();

			GenPineTree(int(numBranches), MAX_TREE_HEIGHT * treeScale);
		}

		numTreeVerts[NUM_TREE_TYPES + a] = curVertPtr - prvVertPtr;
	}

	glGenBuffers(1, &treeBuffers[0]);
	glBindBuffer(GL_ARRAY_BUFFER, treeBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, bushVerts.size() * sizeof(VA_TYPE_TN), bushVerts.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &treeBuffers[1]);
	glBindBuffer(GL_ARRAY_BUFFER, treeBuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, pineVerts.size() * sizeof(VA_TYPE_TN), pineVerts.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}



void CAdvTreeGenerator::BindTreeBuffer(unsigned int treeType) const {
	assert(treeType >= 0);
	// all bush subtypes share the same buffer [0], so do all pine subtypes [1]
	glBindBuffer(GL_ARRAY_BUFFER, treeBuffers[treeType >= NUM_TREE_TYPES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VA_TYPE_TN), VA_TYPE_OFFSET(float, 0));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VA_TYPE_TN), VA_TYPE_OFFSET(float, 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VA_TYPE_TN), VA_TYPE_OFFSET(float, 5));
}

void CAdvTreeGenerator::DrawTreeBuffer(unsigned int treeType) const {
	constexpr unsigned int primTypes[] = {GL_QUADS, GL_TRIANGLES};

	assert(treeType >= 0);

	const unsigned int pineType = (treeType >= NUM_TREE_TYPES);
	const unsigned int baseType = treeType - (NUM_TREE_TYPES * pineType);

	glDrawArrays(primTypes[pineType], baseType * MAX_TREE_VERTS, numTreeVerts[treeType]);
}



void CAdvTreeGenerator::DrawTrunk(const float3& start, const float3& end, const float3& orto1, const float3& orto2, float size)
{
	const float3 flatSun = sky->GetLight()->GetLightDir() * XZVector;

	// cylinder faces
	const int numSides = max(3.0f, size * 10.0f);

	for (int a = 0; a < numSides; a++) {
		const float curAngle = (a    ) / (float)numSides * math::TWOPI;
		const float nxtAngle = (a + 1) / (float)numSides * math::TWOPI;
		const float curColor = 0.4f + (((orto1 * std::sin(curAngle) + orto2 * std::cos(curAngle)).dot(flatSun))) * 0.3f;
		const float nxtColor = 0.4f + (((orto1 * std::sin(nxtAngle) + orto2 * std::cos(nxtAngle)).dot(flatSun))) * 0.3f;

		const float3 v0 = start + orto1 * std::sin(curAngle) * size        + orto2 * std::cos(curAngle) * size       ;
		const float3 v1 =   end + orto1 * std::sin(curAngle) * size * 0.2f + orto2 * std::cos(curAngle) * size * 0.2f;
		const float3 v2 = start + orto1 * std::sin(nxtAngle) * size        + orto2 * std::cos(nxtAngle) * size       ;
		const float3 v3 =   end + orto1 * std::sin(nxtAngle) * size * 0.2f + orto2 * std::cos(nxtAngle) * size * 0.2f;

		assert((curVertPtr - prvVertPtr) < (MAX_TREE_VERTS - 4 + 1));

		// originally triangles ({v0,v1,v2}, {v3,v2,v1}), but quads allow merging with leaf-data
		*(curVertPtr++) = {v0, curAngle / math::PI * 0.125f * 0.5f, 0, FwdVector * curColor};
		*(curVertPtr++) = {v1, curAngle / math::PI * 0.125f * 0.5f, 3, FwdVector * curColor};
		*(curVertPtr++) = {v3, nxtAngle / math::PI * 0.125f * 0.5f, 3, FwdVector * nxtColor};
		*(curVertPtr++) = {v2, nxtAngle / math::PI * 0.125f * 0.5f, 0, FwdVector * nxtColor};
	}
}

void CAdvTreeGenerator::GenBushTree(int numBranch, float height, float width)
{
	const float3 orto1 = RgtVector;
	const float3 orto2 = FwdVector;
	const float baseAngle = math::TWOPI * guRNG.NextFloat();

	DrawTrunk(ZeroVector, UpVector * height, orto1, orto2, width);

	for (int a = 0; a < numBranch; ++a) {
		const float angle = baseAngle + (a * 3.88f) + 0.5f * guRNG.NextFloat();
		const float length = (height * (0.4f + 0.1f * guRNG.NextFloat())) * std::sqrt(float(numBranch - a) / numBranch);

		float3 start = UpVector * ((a + 5) * height / (numBranch + 5));
		float3 dir = (orto1 * std::sin(angle) + orto2 * std::cos(angle)) * XZVector + (UpVector * (0.3f + 0.4f * guRNG.NextFloat()));

		TrunkIterator(start, dir.ANormalize(), length, length * 0.05f, 1);
	}

	for (int a = 0; a < 3; ++a) {
		const float angle = (a * 3.88f) + 0.5f * guRNG.NextFloat();
		const float length = MAX_TREE_HEIGHT * 0.1f;

		float3 start = UpVector * (height - 0.3f);
		float3 dir = (orto1 * std::sin(angle) + orto2 * std::cos(angle)) * XZVector + (UpVector * 0.8f);

		TrunkIterator(start, dir.ANormalize(), length, length * 0.05f, 0);
	}
}

void CAdvTreeGenerator::TrunkIterator(const float3& start, const float3& dir, float length, float size, int depth)
{
	float3 orto1;
	if (dir.dot(UpVector) < 0.9f)
		orto1 = dir.cross(UpVector);
	else
		orto1 = dir.cross(RgtVector);
	orto1.ANormalize();

	float3 orto2 = dir.cross(orto1);
	orto2.ANormalize();

	DrawTrunk(start, start + dir * length, orto1, orto2, size);

	if (depth <= 1)
		CreateLeaves(start, dir, length, orto1, orto2);

	if (depth == 0)
		return;

	const float dirDif = 0.8f * guRNG.NextFloat() + 1.0f;
	const int numTrunks = (int) length * 5 / MAX_TREE_HEIGHT;

	for (int a = 0; a < numTrunks; a++) {
		const float angle = math::PI + float(a) * math::PI + 0.3f * guRNG.NextFloat();
		const float newLength = length * (float(numTrunks - a) / (numTrunks + 1));

		float3 newBase = start + dir * length * (float(a + 1) / (numTrunks + 1));
		float3 newDir = dir + orto1 * std::cos(angle) * dirDif + orto2 * std::sin(angle) * dirDif;

		TrunkIterator(newBase, newDir.ANormalize(), newLength, newLength * 0.05f, depth - 1);
	}
}

void CAdvTreeGenerator::CreateLeaves(const float3& start, const float3& dir, float length, float3& orto1, float3& orto2)
{
	const float baseRot = math::TWOPI * guRNG.NextFloat();
	const int numLeaves = (int) length * 10 / MAX_TREE_HEIGHT;

	const float3 flatSun = sky->GetLight()->GetLightDir() * XZVector;

	auto ADD_LEAF = [&](const float3& pos) {
		const float3 npos = (pos * XZVector).ANormalize();

		const float baseTex = (guRNG.NextInt(3)) * 0.125f;
		const float flipTex = (guRNG.NextInt(2)) * 0.123f;
		const float col = 0.5f + (npos.dot(flatSun) * 0.3f) + 0.1f * guRNG.NextFloat();

		assert((curVertPtr - prvVertPtr) < (MAX_TREE_VERTS - 4 + 1));

		*(curVertPtr++) = {pos, 0.126f + baseTex + flipTex, 0.98f, float3( 0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col)};
		*(curVertPtr++) = {pos, 0.249f + baseTex - flipTex, 0.98f, float3(-0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col)};
		*(curVertPtr++) = {pos, 0.249f + baseTex - flipTex, 0.02f, float3(-0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col)};
		*(curVertPtr++) = {pos, 0.126f + baseTex + flipTex, 0.02f, float3( 0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col)};
	};

	for (int a = 0; a < numLeaves + 1; a++) {
		const float angle = baseRot + a * 0.618f * math::TWOPI;
		const float dist = mix(guRNG.NextFloat(), std::sqrt(a * 1.0f + 1.0f), 0.6f);

		float3 pos = start + dir * length * (0.7f + 0.3f * guRNG.NextFloat());
		float3 ofs = (orto1 * std::sin(angle) + orto2 * std::cos(angle)) * dist * 0.1f * MAX_TREE_HEIGHT;

		pos += ofs;
		pos.y = std::max(pos.y, 0.2f * MAX_TREE_HEIGHT);

		ADD_LEAF(pos);
	}

	ADD_LEAF(start + dir * length * 1.03f);
}


void CAdvTreeGenerator::CreateGranTex(uint8_t* data, int xpos, int ypos, int xsize)
{
	glViewport(0, 0, TEX_SIZE_X, TEX_SIZE_Y);

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);

	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CreateGranTexBranch(ZeroVector, float3(0.93f, 0.93f, 0.0f));

	uint8_t buf[TEX_SIZE_X * TEX_SIZE_Y * TEX_SIZE_C];
	glReadPixels(0, 0, TEX_SIZE_X, TEX_SIZE_Y,  GL_RGBA, GL_UNSIGNED_BYTE,  &buf[0]);

	for (int y = 0; y < TEX_SIZE_Y; ++y) {
		for (int x = 0; x < TEX_SIZE_X; ++x) {
			if (buf[(y * TEX_SIZE_X + x) * 4] == 0 && buf[(y * TEX_SIZE_X + x) * 4 + 1] == 0 && buf[(y * TEX_SIZE_X + x) * 4 + 2] == 0) {
				data[((y + ypos) * xsize + x + xpos) * 4 + 0] = 60;
				data[((y + ypos) * xsize + x + xpos) * 4 + 1] = 90;
				data[((y + ypos) * xsize + x + xpos) * 4 + 2] = 40;
				data[((y + ypos) * xsize + x + xpos) * 4 + 3] = 0;
			} else {
				data[((y + ypos) * xsize + x + xpos) * 4 + 0] = buf[(y * TEX_SIZE_X + x) * 4 + 0];
				data[((y + ypos) * xsize + x + xpos) * 4 + 1] = buf[(y * TEX_SIZE_X + x) * 4 + 1];
				data[((y + ypos) * xsize + x + xpos) * 4 + 2] = buf[(y * TEX_SIZE_X + x) * 4 + 2];
				data[((y + ypos) * xsize + x + xpos) * 4 + 3] = 255;
			}
		}
	}

	glPopAttrib();
}

void CAdvTreeGenerator::CreateGranTexBranch(const float3& start, const float3& end)
{
	const float3 dir = (end - start).ANormalize();
	const float3 orto = dir.cross(FwdVector);

	const float length = dir.dot(end - start);

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	// root-level call, setup
	if (start == ZeroVector) {
		shader->Enable();
		shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::OrthoProj(0.0f, 1.0f, 0.0f, 1.0f, -4.0f, 4.0f));
	}

	{
		float3 c = float3(guRNG.NextFloat(), guRNG.NextFloat(), guRNG.NextFloat());

		c *= float3(0.02f, 0.05f, 0.01f);
		c += float3(0.05f, 0.21f, 0.04f);

		buffer->SafeAppend({start + dir * 0.006f + orto * 0.007f, SColor(c.x, c.y, c.z, 1.0f)});
		buffer->SafeAppend({start + dir * 0.006f - orto * 0.007f, SColor(c.x, c.y, c.z, 1.0f)});
		buffer->SafeAppend({  end                - orto * 0.007f, SColor(c.x, c.y, c.z, 1.0f)});
		buffer->SafeAppend({  end                + orto * 0.007f, SColor(c.x, c.y, c.z, 1.0f)});

		c = float3(0.18f, 0.18f, 0.07f);

		buffer->SafeAppend({start + orto * length * 0.01f , SColor(c.x, c.y, c.z, 1.0f)});
		buffer->SafeAppend({start - orto * length * 0.01f , SColor(c.x, c.y, c.z, 1.0f)});
		buffer->SafeAppend({  end - orto          * 0.001f, SColor(c.x, c.y, c.z, 1.0f)});
		buffer->SafeAppend({  end + orto          * 0.001f, SColor(c.x, c.y, c.z, 1.0f)});
	}

	float tipDist = 0.025f;
	float delta = 0.013f;
	int side = (guRNG.NextInt() & 1) * 2 - 1;

	while (tipDist < length * (0.15f * guRNG.NextFloat() + 0.83f)) {
		float3 bstart = start + dir * (length - tipDist);
		float3 bdir = dir + orto * side * (1.0f + 0.7f * guRNG.NextFloat());
		float3 bend = bstart + bdir.ANormalize() * tipDist * ((6.0f - tipDist) * (0.10f + 0.05f * guRNG.NextFloat()));

		CreateGranTexBranch(bstart, bend);

		side *= -1;

		tipDist += delta;
		delta += 0.005f;
	}

	if (start == ZeroVector) {
		buffer->Submit(GL_QUADS);
		shader->Disable();
	}
}

void CAdvTreeGenerator::GenPineTree(int numBranch, float height)
{
	DrawPineTrunk(ZeroVector, UpVector * height, height * 0.025f);
	float3 orto1 = RgtVector;
	float3 orto2 = FwdVector;

	const float baseAngle = math::TWOPI * guRNG.NextFloat();

	for (int a = 0; a < numBranch; ++a) {
		const float sh = 0.2f + 0.2f * guRNG.NextFloat();
		const float h  = height * std::pow(sh + a * 1.0f / numBranch * (1.0f - sh), 0.7f);
		const float angle = baseAngle + (a * 0.618f + 0.1f * guRNG.NextFloat()) * math::TWOPI;

		float3 dir = orto1 * std::sin(angle) + orto2 * std::cos(angle);

		dir *= XZVector;
		dir += (UpVector * ((a - numBranch) * 0.01f - (0.2f + 0.2f * guRNG.NextFloat())));

		DrawPineBranch(UpVector * h, dir.ANormalize(), std::sqrt(numBranch * 1.0f - a + 5.0f) * 0.08f * MAX_TREE_HEIGHT);
	}

	// create the top
	const float col = 0.55f + 0.2f * guRNG.NextFloat();

	assert((curVertPtr - prvVertPtr) < (MAX_TREE_VERTS - 6 + 1));

	*(curVertPtr++) = {UpVector * (height - 0.09f * MAX_TREE_HEIGHT), 0.126f + 0.5f, 0.02f, float3( 0.0f                   , 0.0f, col)};
	*(curVertPtr++) = {UpVector * (height - 0.03f * MAX_TREE_HEIGHT), 0.249f + 0.5f, 0.02f, float3( 0.05f * MAX_TREE_HEIGHT, 0.0f, col)};
	*(curVertPtr++) = {UpVector * (height - 0.03f * MAX_TREE_HEIGHT), 0.126f + 0.5f, 0.98f, float3(-0.05f * MAX_TREE_HEIGHT, 0.0f, col)};

	*(curVertPtr++) = {UpVector * (height - 0.03f * MAX_TREE_HEIGHT), 0.249f + 0.5f, 0.02f, float3( 0.05f * MAX_TREE_HEIGHT, 0.0f, col)};
	*(curVertPtr++) = {UpVector * (height - 0.03f * MAX_TREE_HEIGHT), 0.126f + 0.5f, 0.98f, float3(-0.05f * MAX_TREE_HEIGHT, 0.0f, col)};
	*(curVertPtr++) = {UpVector * (height + 0.03f * MAX_TREE_HEIGHT), 0.249f + 0.5f, 0.98f, float3( 0.0f                   , 0.0f, col)};
}

void CAdvTreeGenerator::DrawPineTrunk(const float3 &start, const float3 &end, float size)
{
	constexpr float3 orto1 = RgtVector;
	constexpr float3 orto2 = FwdVector;

	const float3 flatSun = sky->GetLight()->GetLightDir() * XZVector;

	constexpr int numSides = 8;
	for (int a = 0; a < numSides; a++) {
		const float curAngle = (a    ) / (float)numSides * math::TWOPI;
		const float nxtAngle = (a + 1) / (float)numSides * math::TWOPI;

		const float curColor = 0.45f + (((orto1 * std::sin(curAngle) + orto2 * std::cos(curAngle)).dot(flatSun))) * 0.3f;
		const float nxtColor = 0.45f + (((orto1 * std::sin(nxtAngle) + orto2 * std::cos(nxtAngle)).dot(flatSun))) * 0.3f;

		assert((curVertPtr - prvVertPtr) < (MAX_TREE_VERTS - 6 + 1));

		*(curVertPtr++) = {start + orto1 * std::sin(curAngle) * size        + orto2 * std::cos(curAngle) * size       , curAngle / math::PI * 0.125f * 0.5f + 0.5f, 0, FwdVector * curColor};
		*(curVertPtr++) = {  end + orto1 * std::sin(curAngle) * size * 0.1f + orto2 * std::cos(curAngle) * size * 0.1f, curAngle / math::PI * 0.125f * 0.5f + 0.5f, 3, FwdVector * curColor};
		*(curVertPtr++) = {start + orto1 * std::sin(nxtAngle) * size        + orto2 * std::cos(nxtAngle) * size       , nxtAngle / math::PI * 0.125f * 0.5f + 0.5f, 0, FwdVector * nxtColor};

		*(curVertPtr++) = {start + orto1 * std::sin(nxtAngle) * size        + orto2 * std::cos(nxtAngle) * size       , nxtAngle / math::PI * 0.125f * 0.5f + 0.5f, 0, FwdVector * nxtColor};
		*(curVertPtr++) = {  end + orto1 * std::sin(curAngle) * size * 0.1f + orto2 * std::cos(curAngle) * size * 0.1f, curAngle / math::PI * 0.125f * 0.5f + 0.5f, 3, FwdVector * curColor};
		*(curVertPtr++) = {  end + orto1 * std::sin(nxtAngle) * size * 0.1f + orto2 * std::cos(nxtAngle) * size * 0.1f, nxtAngle / math::PI * 0.125f * 0.5f + 0.5f, 3, FwdVector * nxtColor};
	}
}

void CAdvTreeGenerator::DrawPineBranch(const float3& start, const float3& dir, float size)
{
	const float3 flatSun = sky->GetLight()->GetLightDir() * XZVector;

	float3 orto1 = dir.cross(UpVector);
	float3 orto2 = dir.cross(orto1.ANormalize());

	const float3   dirSize =   dir * size;
	const float3 orto1Size = orto1 * size;
	const float3 orto2Size = orto2 * size;

	const float baseTex = 0.0f; // int(guRNG.NextFloat() * 3.0f) * 0.125f;
	const float baseCol = 0.4f + 0.3f * dir.dot(flatSun) + 0.1f * guRNG.NextFloat();

	const float tx0 = baseTex + 0.5f + 0.126f;
	const float tx1 = baseTex + 0.5f + 0.249f;
	const float tx2 = baseTex + 0.5f + 0.1875f;

	const float col1 = baseCol + 0.2f * guRNG.NextFloat();
	const float col2 = baseCol + 0.2f * guRNG.NextFloat();
	const float col3 = baseCol + 0.2f * guRNG.NextFloat();
	const float col4 = baseCol + 0.2f * guRNG.NextFloat();
	const float col5 = baseCol + 0.2f * guRNG.NextFloat();

	assert((curVertPtr - prvVertPtr) < (MAX_TREE_VERTS - 12 + 1));

	*(curVertPtr++) = {start                                                       , tx0, 0.02f, FwdVector * col1};
	*(curVertPtr++) = {start + dirSize * 0.5f + orto1Size * 0.5f + orto2Size * 0.2f, tx1, 0.02f, FwdVector * col2};
	*(curVertPtr++) = {start + dirSize * 0.5f                                      , tx2, 0.50f, FwdVector * col4};

	*(curVertPtr++) = {start                                                       , tx0, 0.02f, FwdVector * col1};
	*(curVertPtr++) = {start + dirSize * 0.5f - orto1Size * 0.5f + orto2Size * 0.2f, tx0, 0.98f, FwdVector * col3};
	*(curVertPtr++) = {start + dirSize * 0.5f                                      , tx2, 0.50f, FwdVector * col4};

	*(curVertPtr++) = {start + dirSize * 0.5f                                      , tx2, 0.50f, FwdVector * col4};
	*(curVertPtr++) = {start + dirSize * 0.5f + orto1Size * 0.5f + orto2Size * 0.2f, tx1, 0.02f, FwdVector * col2};
	*(curVertPtr++) = {start + dirSize        + orto2Size * 0.1f                   , tx1, 0.98f, FwdVector * col5};

	*(curVertPtr++) = {start + dirSize * 0.5f                                      , tx2, 0.50f, FwdVector * col4};
	*(curVertPtr++) = {start + dirSize * 0.5f - orto1Size * 0.5f + orto2Size * 0.2f, tx0, 0.98f, FwdVector * col3};
	*(curVertPtr++) = {start + dirSize                           + orto2Size * 0.1f, tx1, 0.98f, FwdVector * col5};
}


void CAdvTreeGenerator::CreateLeafTex(uint8_t* data, int xpos, int ypos, int xsize)
{
	glViewport(0, 0, TEX_SIZE_X, TEX_SIZE_Y);

	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	CMatrix44f leafMat;

	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::OrthoProj(-1.0f, 1.0f, -1.0f, 1.0f, -5.0f, 5.0f));


	const float baseCol = 0.8f + 0.2f * guRNG.NextFloat();

	for (int a = 0; a < 84; ++a) {
		const float xp = 0.9f - 1.8f * guRNG.NextFloat();
		const float yp = 0.9f - 1.8f * guRNG.NextFloat();

		const float rCol = baseCol * (0.7f + 0.3f * guRNG.NextFloat());
		const float gCol = baseCol * (0.7f + 0.3f * guRNG.NextFloat());
		const float bCol = baseCol * (0.7f + 0.3f * guRNG.NextFloat());

		leafMat.LoadIdentity();
		leafMat.RotateZ(-(360.0f * guRNG.NextFloat()) * math::DEG_TO_RAD);
		leafMat.RotateX(-(360.0f * guRNG.NextFloat()) * math::DEG_TO_RAD);
		leafMat.RotateY(-(360.0f * guRNG.NextFloat()) * math::DEG_TO_RAD);
		leafMat.Translate(xp, yp, 0.0f);

		shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, leafMat);
		buffer->SafeAppend({{-0.1f, -0.2f, 0.0f}, 0.0f, 0.0f, SColor(rCol, gCol, bCol, 1.0f)});
		buffer->SafeAppend({{-0.1f,  0.2f, 0.0f}, 0.0f, 1.0f, SColor(rCol, gCol, bCol, 1.0f)});
		buffer->SafeAppend({{ 0.1f,  0.2f, 0.0f}, 1.0f, 1.0f, SColor(rCol, gCol, bCol, 1.0f)});
		buffer->SafeAppend({{ 0.1f, -0.2f, 0.0f}, 1.0f, 0.0f, SColor(rCol, gCol, bCol, 1.0f)});
		buffer->Submit(GL_QUADS);
	}

	shader->Disable();


	uint8_t leafTexBuf[TEX_SIZE_X * TEX_SIZE_Y * TEX_SIZE_C];
	glReadPixels(0, 0, 256, 256,  GL_RGBA, GL_UNSIGNED_BYTE,  &leafTexBuf[0]);

	#if 0
	CBitmap bm(leafTexBuf, TEX_SIZE_X, TEX_SIZE_Y);
	bm.Save("leaf.bmp");
	#endif

	for (int y = 0; y < TEX_SIZE_Y; ++y) {
		for (int x = 0; x < TEX_SIZE_X; ++x) {
			if (leafTexBuf[(y * TEX_SIZE_X + x) * 4 + 1] != 0) {
				data[((y + ypos) * xsize + x + xpos) * 4 + 0] = leafTexBuf[(y * TEX_SIZE_X + x) * 4 + 0];
				data[((y + ypos) * xsize + x + xpos) * 4 + 1] = leafTexBuf[(y * TEX_SIZE_X + x) * 4 + 1];
				data[((y + ypos) * xsize + x + xpos) * 4 + 2] = leafTexBuf[(y * TEX_SIZE_X + x) * 4 + 2];
				data[((y + ypos) * xsize + x + xpos) * 4 + 3] = 255;
			} else {
				data[((y + ypos) * xsize + x + xpos) * 4 + 0] = (uint8_t)(0.24f * 1.2f * 255);
				data[((y + ypos) * xsize + x + xpos) * 4 + 1] = (uint8_t)(0.40f * 1.2f * 255);
				data[((y + ypos) * xsize + x + xpos) * 4 + 2] = (uint8_t)(0.23f * 1.2f * 255);
				data[((y + ypos) * xsize + x + xpos) * 4 + 3] = 0;
			}
		}
	}

	glViewport(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);
}

