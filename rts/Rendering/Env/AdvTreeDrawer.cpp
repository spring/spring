/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AdvTreeDrawer.h"
#include "AdvTreeGenerator.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "System/Matrix44f.h"

static const float TEX_LEAF_START_Y1 = 0.001f;
static const float TEX_LEAF_END_Y1   = 0.124f;
static const float TEX_LEAF_START_Y2 = 0.126f;
static const float TEX_LEAF_END_Y2   = 0.249f;
static const float TEX_LEAF_START_Y3 = 0.251f;
static const float TEX_LEAF_END_Y3   = 0.374f;
static const float TEX_LEAF_START_Y4 = 0.376f;
static const float TEX_LEAF_END_Y4   = 0.499f;

static const float TEX_LEAF_START_X1 = 0.0f;
static const float TEX_LEAF_END_X1   = 0.125f;
static const float TEX_LEAF_START_X2 = 0.0f;
static const float TEX_LEAF_END_X2   = 0.125f;
static const float TEX_LEAF_START_X3 = 0.0f;
static const float TEX_LEAF_END_X3   = 0.125f;

static const float PART_MAX_TREE_HEIGHT   = MAX_TREE_HEIGHT * 0.4f;
static const float HALF_MAX_TREE_HEIGHT   = MAX_TREE_HEIGHT * 0.5f;


CAdvTreeDrawer::CAdvTreeDrawer(): ITreeDrawer()
{
	if (!GLEW_ARB_vertex_program || !FBO::IsSupported())
		throw content_error("ADVTREE: missing OpenGL features!");

	LoadTreeShaders();

	treeGen = new CAdvTreeGenerator();
	treeGen->CreateFarTex(treeShaders[TREE_PROGRAM_NEAR_BASIC]);

	oldTreeDistance = 4;
	lastListClean = 0;

	treeSquares.resize(nTrees);
}

CAdvTreeDrawer::~CAdvTreeDrawer()
{
	for (TreeSquareStruct& tss: treeSquares) {
		if (tss.dispList) {
			glDeleteLists(tss.dispList, 1);
		}
		if (tss.farDispList) {
			glDeleteLists(tss.farDispList, 1);
		}
	}

	delete treeGen;

	shaderHandler->ReleaseProgramObjects("[TreeDrawer]");
	treeShaders.clear();
}



void CAdvTreeDrawer::LoadTreeShaders() {
	treeShaders.resize(TREE_PROGRAM_LAST, NULL);

	const static std::string shaderNames[TREE_PROGRAM_LAST] = {
		"treeNearDefShader", // no-shadow default shader
		"treeNearAdvShader",
		"treeDistAdvShader",
	};
	const static std::string shaderDefines[TREE_PROGRAM_LAST] = {
		"#define TREE_NEAR_BASIC\n",
		"#define TREE_NEAR_SHADOW\n",
		"#define TREE_DIST_SHADOW\n"
	};

	const static int numUniformNamesNDNA = 6;
	const static std::string uniformNamesNDNA[numUniformNamesNDNA] = {
		"cameraDirX",          // VP
		"cameraDirY",          // VP
		"treeOffset",          // VP
		"groundAmbientColor",  // VP + FP
		"groundDiffuseColor",  // VP
		"alphaModifiers",      // VP
	};
	const static int numUniformNamesNADA = 5;
	const std::string uniformNamesNADA[numUniformNamesNADA] = {
		"shadowMatrix",        // VP
		"shadowParams",        // VP
		"groundShadowDensity", // FP
		"shadowTex",           // FP
		"diffuseTex",          // FP
	};

	if (globalRendering->haveGLSL) {
		treeShaders[TREE_PROGRAM_NEAR_BASIC] =
			shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_NEAR_BASIC] + "GLSL", false);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->AttachShaderObject(
			shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_NEAR_BASIC], GL_VERTEX_SHADER)
		);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Link();

		treeShaders[TREE_PROGRAM_NEAR_SHADOW] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_NEAR_SHADOW] + "GLSL", false);
		treeShaders[TREE_PROGRAM_DIST_SHADOW] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_DIST_SHADOW] + "GLSL", false);

		if (CShadowHandler::ShadowsSupported()) {
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->AttachShaderObject(
				shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_NEAR_SHADOW], GL_VERTEX_SHADER)
			);
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->AttachShaderObject(
				shaderHandler->CreateShaderObject("GLSL/TreeFragProg.glsl", shaderDefines[TREE_PROGRAM_NEAR_SHADOW], GL_FRAGMENT_SHADER)
			);

			treeShaders[TREE_PROGRAM_DIST_SHADOW]->AttachShaderObject(
				shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_DIST_SHADOW], GL_VERTEX_SHADER)
			);
			treeShaders[TREE_PROGRAM_DIST_SHADOW]->AttachShaderObject(
				shaderHandler->CreateShaderObject("GLSL/TreeFragProg.glsl", shaderDefines[TREE_PROGRAM_DIST_SHADOW], GL_FRAGMENT_SHADER)
			);
		}

		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Link();
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->Link();

		// ND, NA: indices [0, numUniformNamesNDNA - 1]
		for (int i = 0; i < numUniformNamesNDNA; i++) {
			treeShaders[TREE_PROGRAM_NEAR_BASIC ]->SetUniformLocation(uniformNamesNDNA[i]);
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniformLocation(uniformNamesNDNA[i]);
			treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniformLocation((i != 3)? "$UNUSED$": uniformNamesNDNA[i]);
		}

		// ND: index <numUniformNamesNDNA>
		treeShaders[TREE_PROGRAM_NEAR_BASIC ]->SetUniformLocation("invMapSizePO2");
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniformLocation("$UNUSED$");
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniformLocation("$UNUSED$");

		// NA, DA: indices [numUniformNamesNDNA + 1, numUniformNamesNDNA + numUniformNamesNADA]
		for (int i = 0; i < numUniformNamesNADA; i++) {
			treeShaders[TREE_PROGRAM_NEAR_BASIC ]->SetUniformLocation("$UNUSED$");
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniformLocation(uniformNamesNADA[i]);
			treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniformLocation(uniformNamesNADA[i]);
		}

		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Enable();
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->SetUniform3fv(4, &sunLighting->groundDiffuseColor[0]);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->SetUniform4f(6, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Disable();
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Validate();

		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Enable();
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform3fv(4, &sunLighting->groundDiffuseColor[0]);
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform1f(9, 1.0f - (sky->GetLight()->GetGroundShadowDensity() * 0.5f));
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform1i(10, 0);
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform1i(11, 1);
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Disable();
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Validate();

		treeShaders[TREE_PROGRAM_DIST_SHADOW]->Enable();
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform1f(9, 1.0f - (sky->GetLight()->GetGroundShadowDensity() * 0.5f));
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform1i(10, 0);
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform1i(11, 1);
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->Disable();
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->Validate();
	} else {
		treeShaders[TREE_PROGRAM_NEAR_BASIC] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_NEAR_BASIC] + "ARB", true);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->AttachShaderObject(shaderHandler->CreateShaderObject("ARB/treeNS.vp", "", GL_VERTEX_PROGRAM_ARB));
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Link();

		if (CShadowHandler::ShadowsSupported()) {
			treeShaders[TREE_PROGRAM_NEAR_SHADOW] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_NEAR_SHADOW] + "ARB", true);
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->AttachShaderObject(shaderHandler->CreateShaderObject("ARB/tree.vp", "", GL_VERTEX_PROGRAM_ARB));
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->AttachShaderObject(shaderHandler->CreateShaderObject("ARB/treeFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Link();
			treeShaders[TREE_PROGRAM_DIST_SHADOW] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_DIST_SHADOW] + "ARB", true);
			treeShaders[TREE_PROGRAM_DIST_SHADOW]->AttachShaderObject(shaderHandler->CreateShaderObject("ARB/treeFar.vp", "", GL_VERTEX_PROGRAM_ARB));
			treeShaders[TREE_PROGRAM_DIST_SHADOW]->AttachShaderObject(shaderHandler->CreateShaderObject("ARB/treeFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			treeShaders[TREE_PROGRAM_DIST_SHADOW]->Link();
		}
	}
}



void CAdvTreeDrawer::Update()
{
	for (unsigned int n = 0; n < fallingTrees.size(); /*no-op*/) {
		FallingTree* fti = &fallingTrees[n];

		fti->fallPos += (fti->speed * 0.1f);
		fti->speed += (std::sin(fti->fallPos) * 0.04f);

		if (fti->fallPos > 1.0f) {
			// remove the tree
			fallingTrees[n] = fallingTrees.back();
			fallingTrees.pop_back();
			continue;
		}

		n += 1;
	}
}



static inline void SetArrayQ(CVertexArray* va, float t1, float t2, const float3& v)
{
	va->AddVertexQT(v, t1, t2);
}

void CAdvTreeDrawer::DrawTreeVertexA(CVertexArray* va, float3& ftpos, float dx, float dy) {
	SetArrayQ(va, TEX_LEAF_START_X1 + dx, TEX_LEAF_START_Y1 + dy, ftpos); ftpos.y += MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X1 + dx, TEX_LEAF_END_Y1   + dy, ftpos); ftpos.x -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X1   + dx, TEX_LEAF_END_Y1   + dy, ftpos); ftpos.y -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X1   + dx, TEX_LEAF_START_Y1 + dy, ftpos); ftpos.x += HALF_MAX_TREE_HEIGHT;

	ftpos.z += HALF_MAX_TREE_HEIGHT;

	SetArrayQ(va, TEX_LEAF_START_X2 + dx, TEX_LEAF_START_Y2 + dy, ftpos); ftpos.y += MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X2 + dx, TEX_LEAF_END_Y2   + dy, ftpos); ftpos.z -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X2   + dx, TEX_LEAF_END_Y2   + dy, ftpos); ftpos.y -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X2   + dx, TEX_LEAF_START_Y2 + dy, ftpos);

	ftpos.x += HALF_MAX_TREE_HEIGHT;
	ftpos.y += PART_MAX_TREE_HEIGHT;
}

void CAdvTreeDrawer::DrawTreeVertex(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge) {
	if (enlarge)
		va->EnlargeArrays(12, 0, VA_SIZE_T);

	float3 ftpos = pos;
	ftpos.x += HALF_MAX_TREE_HEIGHT;

	DrawTreeVertexA(va, ftpos, dx, dy);

	ftpos.z += MAX_TREE_HEIGHT;

	SetArrayQ(va, TEX_LEAF_START_X3 + dx, TEX_LEAF_START_Y3 + dy, ftpos); ftpos.z -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X3 + dx, TEX_LEAF_END_Y3   + dy, ftpos); ftpos.x -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X3   + dx, TEX_LEAF_END_Y3   + dy, ftpos); ftpos.z += MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X3   + dx, TEX_LEAF_START_Y3 + dy, ftpos);
}

void CAdvTreeDrawer::DrawTreeVertexMid(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge) {
	if (enlarge)
		va->EnlargeArrays(12, 0, VA_SIZE_T);

	float3 ftpos = pos;
	ftpos.x += HALF_MAX_TREE_HEIGHT;

	DrawTreeVertexA(va, ftpos, dx, dy);

	ftpos.z += HALF_MAX_TREE_HEIGHT;

	SetArrayQ(va, TEX_LEAF_START_X3 + dx, TEX_LEAF_START_Y3 + dy, ftpos);
		ftpos.x -= HALF_MAX_TREE_HEIGHT;
		ftpos.z -= HALF_MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X3 + dx, TEX_LEAF_END_Y3 + dy,   ftpos);
		ftpos.x -= HALF_MAX_TREE_HEIGHT;
		ftpos.z += HALF_MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X3 + dx,   TEX_LEAF_END_Y3 + dy,   ftpos);
		ftpos.x += HALF_MAX_TREE_HEIGHT;
		ftpos.z += HALF_MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X3 + dx,   TEX_LEAF_START_Y3 + dy, ftpos);
}

void CAdvTreeDrawer::DrawTreeVertexFar(CVertexArray* va, const float3& pos, const float3& swd, float dx, float dy, bool enlarge) {
	if (enlarge)
		va->EnlargeArrays(4, 0, VA_SIZE_T);

	float3 base = pos + swd;

	SetArrayQ(va, TEX_LEAF_START_X1 + dx, TEX_LEAF_START_Y4 + dy, base); base.y += MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X1 + dx, TEX_LEAF_END_Y4   + dy, base); base   -= (swd * 2.0f);
	SetArrayQ(va, TEX_LEAF_END_X1   + dx, TEX_LEAF_END_Y4   + dy, base); base.y -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X1   + dx, TEX_LEAF_START_Y4 + dy, base);
}




struct CAdvTreeSquareDrawer : public CReadMap::IQuadDrawer
{
	CAdvTreeSquareDrawer(CAdvTreeDrawer* td, int cx, int cy, float treeDistance, bool drawDetailed)
		: td(td)
		, cx(cx)
		, cy(cy)
		, treeDistance(treeDistance)
		, drawDetailed(drawDetailed)
		, blendEnabled(false)
	{
		glDisable(GL_BLEND);
	}

	void ResetState() {
		td = nullptr;

		cx = 0;
		cy = 0;

		treeDistance = 0.0f;
		drawDetailed = false;
		blendEnabled = false;
		glDisable(GL_BLEND);
	}

	void DrawQuad(int x, int y);

	CAdvTreeDrawer* td;
	int cx, cy;
	float treeDistance;
	bool drawDetailed;
	bool blendEnabled;
};

void CAdvTreeSquareDrawer::DrawQuad(int x, int y)
{
	const int treesX = td->treesX;
	ITreeDrawer::TreeSquareStruct* tss = &td->treeSquares[(y * treesX) + x];

	if ((abs(cy - y) <= 2) && (abs(cx - x) <= 2) && drawDetailed) {
		// skip the closest squares
		return;
	}

	float3 dif;
		dif.x = camera->GetPos().x - ((x * SQUARE_SIZE * TREE_SQUARE_SIZE) + (SQUARE_SIZE * TREE_SQUARE_SIZE / 2));
		dif.y = 0.0f;
		dif.z = camera->GetPos().z - ((y * SQUARE_SIZE * TREE_SQUARE_SIZE) + (SQUARE_SIZE * TREE_SQUARE_SIZE / 2));
	const float dist = dif.Length();
	const float distFactor = dist / treeDistance;
	dif.Normalize();
	const float3 side = UpVector.cross(dif);

	if (distFactor < MID_TREE_DIST_FACTOR) {
		// midle-distance trees
		tss->lastSeen = gs->frameNum;

		if (tss->dispList == 0) {
			tss->dispList = glGenLists(1);

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

			for (auto ti = tss->trees.cbegin(); ti != tss->trees.cend(); ++ti) {
				const ITreeDrawer::TreeStruct* ts = &(*ti);
				const CFeature* f = featureHandler->GetFeature(ts->id);

				if (f == NULL)
					continue;
				if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;

				if (ts->type < 8) {
					CAdvTreeDrawer::DrawTreeVertexMid(va, ts->pos, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					CAdvTreeDrawer::DrawTreeVertexMid(va, ts->pos, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->dispList, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}

		if (blendEnabled) {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glDisable(GL_BLEND);
			glAlphaFunc(GL_GREATER, 0.5f);
			blendEnabled = false;
		}
		glCallList(tss->dispList);
		return;
	}

	if (distFactor < FAR_TREE_DIST_FACTOR) {
		// far-distance trees
		tss->lastSeenFar = gs->frameNum;

		if ((tss->farDispList == 0) || (dif.dot(tss->viewVector) < 0.97f)) {
			if (tss->farDispList == 0)
				tss->farDispList = glGenLists(1);

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(4 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector = dif;

			for (auto ti = tss->trees.cbegin(); ti != tss->trees.cend(); ++ti) {
				const ITreeDrawer::TreeStruct* ts = &(*ti);
				const CFeature* f = featureHandler->GetFeature(ts->id);

				if (f == NULL)
					continue;
				// note: will cause some trees to be invisible if list is not refreshed
				if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;

				if (ts->type < 8) {
					CAdvTreeDrawer::DrawTreeVertexFar(va, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					CAdvTreeDrawer::DrawTreeVertexFar(va, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->farDispList, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}

		if (distFactor > FADE_TREE_DIST_FACTOR) {
			// faded far trees
			if (!blendEnabled) {
				const float alpha = 1.0f - ((distFactor - FADE_TREE_DIST_FACTOR) / (FAR_TREE_DIST_FACTOR - FADE_TREE_DIST_FACTOR));
				glEnable(GL_BLEND);
				glColor4f(1.0f, 1.0f, 1.0f, alpha);
				glAlphaFunc(GL_GREATER, alpha * 0.5f);
				blendEnabled = true;
			}
		} else {
			if (blendEnabled) {
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glDisable(GL_BLEND);
				glAlphaFunc(GL_GREATER, 0.5f);
				blendEnabled = false;
			}
		}

		glCallList(tss->farDispList);
	}
}




void CAdvTreeDrawer::Draw(float treeDistance, bool drawReflection)
{
	// trees are never drawn in any special (non-opaque) pass
	CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_PLAYER);
	Shader::IProgramObject* treeShader = NULL;

	const int activeFarTex = treeGen->farTex[cam->GetDir().z >= 0.0f];
	const bool drawDetailed = ((treeDistance >= 4.0f) || drawReflection);

	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(GL_TRUE);

	sky->SetupFog();

	if (shadowHandler->ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, activeFarTex);

		shadowHandler->SetupShadowTexSampler(GL_TEXTURE0);

		treeShader = treeShaders[TREE_PROGRAM_DIST_SHADOW];
		treeShader->Enable();

		if (globalRendering->haveGLSL) {
			treeShader->SetUniformMatrix4fv(7, false, shadowHandler->GetShadowMatrixRaw());
			treeShader->SetUniform4fv(8, &(shadowHandler->GetShadowParams().x));
		} else {
			treeShader->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
			treeShader->SetUniform4f(10, sunLighting->groundAmbientColor.x, sunLighting->groundAmbientColor.y, sunLighting->groundAmbientColor.z, 1.0f);
			treeShader->SetUniform4f(11, 0.0f, 0.0f, 0.0f, 1.0f - (sky->GetLight()->GetGroundShadowDensity() * 0.5f));
			treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);

			glMatrixMode(GL_MATRIX0_ARB);
			glLoadMatrixf(shadowHandler->GetShadowMatrixRaw());
			glMatrixMode(GL_MODELVIEW);
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
	}

	const int cx = int(cam->GetPos().x / (SQUARE_SIZE * TREE_SQUARE_SIZE));
	const int cy = int(cam->GetPos().z / (SQUARE_SIZE * TREE_SQUARE_SIZE));

	CAdvTreeSquareDrawer drawer(this, cx, cy, treeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE, drawDetailed);

	oldTreeDistance = treeDistance;

	// draw far-trees using map-dependent grid-visibility (FIXME: ignores LOS)
	readMap->GridVisibility(nullptr, &drawer, drawer.treeDistance * 2.0f, TREE_SQUARE_SIZE);


	if (drawDetailed) {
		// draw near-trees
		const int xstart = Clamp(cx - 2, 0, mapDims.mapx / TREE_SQUARE_SIZE - 1);
		const int xend   = Clamp(cx + 2, 0, mapDims.mapx / TREE_SQUARE_SIZE - 1);
		const int ystart = Clamp(cy - 2, 0, mapDims.mapy / TREE_SQUARE_SIZE - 1);
		const int yend   = Clamp(cy + 2, 0, mapDims.mapy / TREE_SQUARE_SIZE - 1);

		if (shadowHandler->ShadowsLoaded()) {
			treeShader->Disable();
			treeShader = treeShaders[TREE_PROGRAM_NEAR_SHADOW];
			treeShader->Enable();

			if (globalRendering->haveGLSL) {
				treeShader->SetUniformMatrix4fv(7, false, shadowHandler->GetShadowMatrixRaw());
				treeShader->SetUniform4fv(8, &(shadowHandler->GetShadowParams().x));
			}

			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
			glActiveTexture(GL_TEXTURE0);
		} else {
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);

			treeShader = treeShaders[TREE_PROGRAM_NEAR_BASIC];
			treeShader->Enable();

			if (!globalRendering->haveGLSL) {
				const int mx = mapDims.pwr2mapx * SQUARE_SIZE;
				const int my = mapDims.pwr2mapy * SQUARE_SIZE;
				treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				treeShader->SetUniform4f(15, 1.0f / mx, 1.0f / my, 1.0f / mx, 1.0f);
			}
		}


		if (globalRendering->haveGLSL) {
			treeShader->SetUniform3fv(0, &cam->GetRight()[0]);
			treeShader->SetUniform3fv(1, &cam->GetUp()[0]);
			treeShader->SetUniform2f(5, 0.20f * (1.0f / MAX_TREE_HEIGHT), 0.85f);
		} else {
			treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			treeShader->SetUniform3f(13, cam->GetRight().x, cam->GetRight().y, cam->GetRight().z);
			treeShader->SetUniform3f( 9, cam->GetUp().x,    cam->GetUp().y,    cam->GetUp().z   );
			treeShader->SetUniform4f(11, sunLighting->groundDiffuseColor.x, sunLighting->groundDiffuseColor.y, sunLighting->groundDiffuseColor.z, 0.85f);
			treeShader->SetUniform4f(14, sunLighting->groundAmbientColor.x, sunLighting->groundAmbientColor.y, sunLighting->groundAmbientColor.z, 0.85f);
			treeShader->SetUniform4f(12, 0.0f, 0.0f, 0.0f, 0.20f * (1.0f / MAX_TREE_HEIGHT)); // w = alpha/height modifier
		}


		glAlphaFunc(GL_GREATER, 0.5f);
		glDisable(GL_BLEND);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		CVertexArray* va = GetVertexArray();
		va->Initialize();


		static std::vector<FadeTree> fadeTrees;

		fadeTrees.clear();
		fadeTrees.reserve(3000);

		for (TreeSquareStruct* pTSS = &treeSquares[ystart * treesX]; pTSS <= &treeSquares[yend * treesX]; pTSS += treesX) {
			for (TreeSquareStruct* tss = pTSS + xstart; tss <= (pTSS + xend); ++tss) {
				tss->lastSeen = gs->frameNum;
				va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

				for (auto ti = tss->trees.cbegin(); ti != tss->trees.cend(); ++ti) {
					const TreeStruct* ts = &(*ti);
					const CFeature* f = featureHandler->GetFeature(ts->id);

					if (f == NULL)
						continue;
					if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
						continue;
					if (!cam->InView(ts->pos + (UpVector * (MAX_TREE_HEIGHT / 2.0f)), MAX_TREE_HEIGHT / 2.0f))
						continue;

					const float camDist = (ts->pos - cam->GetPos()).SqLength();
					int type = ts->type;
					float dy = 0.0f;
					unsigned int dispList;

					if (type < 8) {
						dy = 0.5f;
						dispList = treeGen->pineDL + type;
					} else {
						type -= 8;
						dy = 0.0f;
						dispList = treeGen->leafDL + type;
					}

					if (camDist < (SQUARE_SIZE * SQUARE_SIZE * 110 * 110)) {
						// draw detailed near-distance tree (same as mid-distance trees without alpha)
						treeShader->SetUniform3f(((globalRendering->haveGLSL)? 2: 10), ts->pos.x, ts->pos.y, ts->pos.z);
						glCallList(dispList);
					} else if (camDist < (SQUARE_SIZE * SQUARE_SIZE * 125 * 125)) {
						// draw mid-distance tree
						const float relDist = (ts->pos.distance(cam->GetPos()) - SQUARE_SIZE * 110) / (SQUARE_SIZE * 15);

						treeShader->SetUniform3f(((globalRendering->haveGLSL)? 2: 10), ts->pos.x, ts->pos.y, ts->pos.z);

						glAlphaFunc(GL_GREATER, 0.8f + relDist * 0.2f);
						glCallList(dispList);
						glAlphaFunc(GL_GREATER, 0.5f);

						// save for second pass
						fadeTrees.emplace_back();
						FadeTree& ft = fadeTrees.back();

						ft.id = f->id;
						ft.type = type;

						ft.pos = ts->pos;

						ft.relDist = relDist;
						ft.deltaY = dy;
					} else {
						// draw far-distance tree
						CAdvTreeDrawer::DrawTreeVertex(va, ts->pos, type * 0.125f, dy, false);
					}
				}
			}
		}


		// reset the world-offset
		treeShader->SetUniform3f(((globalRendering->haveGLSL)? 2: 10), 0.0f, 0.0f, 0.0f);

		// draw trees that have been marked as falling
		for (auto fti = fallingTrees.cbegin(); fti != fallingTrees.cend(); ++fti) {
			// const CFeature* f = featureHandler->GetFeature(fti->id);
			const float3 pos = fti->pos - UpVector * (fti->fallPos * 20);

			// featureID is invalid for falling trees
			// if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
			//   continue;
			if (!losHandler->InLos(pos, gu->myAllyTeam))
				continue;
			if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT / 2.0f)), MAX_TREE_HEIGHT / 2.0f))
				continue;

			const float ang = fti->fallPos * PI;

			const float3 yvec(fti->dir.x * std::sin(ang), std::cos(ang), fti->dir.z * std::sin(ang));
			const float3 zvec((yvec.cross(-RgtVector)).ANormalize());
			const float3 xvec(yvec.cross(zvec));

			CMatrix44f transMatrix(pos, xvec, yvec, zvec);

			glPushMatrix();
			glMultMatrixf(&transMatrix[0]);

			int type = fti->type;
			int dispList = 0;

			if (type < 8) {
				dispList = treeGen->pineDL + type;
			} else {
				type -= 8;
				dispList = treeGen->leafDL + type;
			}

			glCallList(dispList);
			glPopMatrix();
		}


		if (shadowHandler->ShadowsLoaded()) {
			treeShader->Disable();
			treeShader = treeShaders[TREE_PROGRAM_DIST_SHADOW];
			treeShader->Enable();

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
			glActiveTexture(GL_TEXTURE0);
		} else {
			treeShader->Disable();
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		}


		// draw far-distance trees
		va->DrawArrayT(GL_QUADS);

		// draw faded mid-distance trees
		for (const FadeTree& fTree: fadeTrees) {
			const CFeature* f = featureHandler->GetFeature(fTree.id);

			if (f == NULL)
				continue;
			if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
				continue;
			if (!cam->InView(fTree.pos, MAX_TREE_HEIGHT / 2.0f))
				continue;

			va = GetVertexArray();
			va->Initialize();
			va->CheckInitSize(12 * VA_SIZE_T);

			CAdvTreeDrawer::DrawTreeVertex(va, fTree.pos, fTree.type * 0.125f, fTree.deltaY, false);

			glAlphaFunc(GL_GREATER, 1.0f - (fTree.relDist * 0.5f));
			va->DrawArrayT(GL_QUADS);
		}
	}

	if (shadowHandler->ShadowsLoaded()) {
		treeShader->Disable();

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	}

	glPopAttrib();



	// clean out squares from memory that are no longer visible
	// can be -1, do not want to let start or end become negative
	const int frameNum = std::max(gs->frameNum, 0);

	const int startClean = (lastListClean * 20) % nTrees;
	const int   endClean = (     frameNum * 20) % nTrees;

	lastListClean = frameNum;

	if (startClean > endClean) {
		for (int i = startClean; i < nTrees; i++) {
			TreeSquareStruct* pTSS = &treeSquares[i];

			if ((pTSS->lastSeen < frameNum - 50) && pTSS->dispList) {
				glDeleteLists(pTSS->dispList, 1);
				pTSS->dispList = 0;
			}
			if ((pTSS->lastSeenFar < (frameNum - 50)) && pTSS->farDispList) {
				glDeleteLists(pTSS->farDispList, 1);
				pTSS->farDispList = 0;
			}
		}
		for (int i = 0; i < endClean; i++) {
			TreeSquareStruct* pTSS = &treeSquares[i];

			if ((pTSS->lastSeen < (frameNum - 50)) && pTSS->dispList) {
				glDeleteLists(pTSS->dispList, 1);
				pTSS->dispList = 0;
			}
			if ((pTSS->lastSeenFar < (frameNum - 50)) && pTSS->farDispList) {
				glDeleteLists(pTSS->farDispList, 1);
				pTSS->farDispList = 0;
			}
		}
	} else {
		for (int i = startClean; i < endClean; i++) {
			TreeSquareStruct* pTSS = &treeSquares[i];

			if ((pTSS->lastSeen < (frameNum - 50)) && pTSS->dispList) {
				glDeleteLists(pTSS->dispList, 1);
				pTSS->dispList = 0;
			}
			if ((pTSS->lastSeenFar < (frameNum - 50)) && pTSS->farDispList) {
				glDeleteLists(pTSS->farDispList, 1);
				pTSS->farDispList = 0;
			}
		}
	}
}



struct CAdvTreeSquareShadowPassDrawer: public CReadMap::IQuadDrawer
{
	void ResetState() {
		td = nullptr;

		cx = 0;
		cy = 0;

		treeDistance = 0.0f;
		drawDetailed = false;
	}

	void DrawQuad(int x, int y);

	CAdvTreeDrawer* td;
	int cx, cy;
	bool drawDetailed;
	float treeDistance;
};

void CAdvTreeSquareShadowPassDrawer::DrawQuad(int x, int y)
{
	const int treesX = td->treesX;
	ITreeDrawer::TreeSquareStruct* tss = &td->treeSquares[(y * treesX) + x];

	if ((abs(cy - y) <= 2) && (abs(cx - x) <= 2) && drawDetailed) {
		// skip the closest squares
		return;
	}

	float3 dif;
		dif.x = camera->GetPos().x - ((x * SQUARE_SIZE * TREE_SQUARE_SIZE) + (SQUARE_SIZE * TREE_SQUARE_SIZE / 2));
		dif.y = 0.0f;
		dif.z = camera->GetPos().z - ((y * SQUARE_SIZE * TREE_SQUARE_SIZE) + (SQUARE_SIZE * TREE_SQUARE_SIZE / 2));
	const float dist = dif.Length();
	const float distFactor = dist / treeDistance;
	dif.Normalize();
	const float3 side = UpVector.cross(dif);

	if (distFactor < MID_TREE_DIST_FACTOR) {
		// midle distance trees
		tss->lastSeen = gs->frameNum;

		if (tss->dispList == 0) {
			tss->dispList = glGenLists(1);

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

			for (auto ti = tss->trees.cbegin(); ti != tss->trees.cend(); ++ti) {
				const ITreeDrawer::TreeStruct* ts = &(*ti);
				const CFeature* f = featureHandler->GetFeature(ts->id);

				if (f == NULL)
					continue;
				// note: will cause some trees to be invisible if list is not refreshed
				if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;

				if (ts->type < 8) {
					CAdvTreeDrawer::DrawTreeVertexMid(va, ts->pos, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					CAdvTreeDrawer::DrawTreeVertexMid(va, ts->pos, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->dispList, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glAlphaFunc(GL_GREATER, 0.5f);
		glCallList(tss->dispList);
		return;
	}

	if (distFactor < FAR_TREE_DIST_FACTOR) {
		// far trees
		tss->lastSeenFar = gs->frameNum;

		if ((tss->farDispList == 0) || (dif.dot(tss->viewVector) < 0.97f)) {
			if (tss->farDispList == 0)
				tss->farDispList = glGenLists(1);

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(4 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector = dif;

			for (auto ti = tss->trees.cbegin(); ti != tss->trees.cend(); ++ti) {
				const ITreeDrawer::TreeStruct* ts = &(*ti);
				const CFeature* f = featureHandler->GetFeature(ts->id);

				if (f == NULL)
					continue;
				if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;

				if (ts->type < 8) {
					CAdvTreeDrawer::DrawTreeVertexFar(va, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					CAdvTreeDrawer::DrawTreeVertexFar(va, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->farDispList, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}

		if (distFactor > FADE_TREE_DIST_FACTOR) {
			// faded far trees
			const float alpha = 1.0f - (distFactor - FADE_TREE_DIST_FACTOR) / (FAR_TREE_DIST_FACTOR - FADE_TREE_DIST_FACTOR);
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glAlphaFunc(GL_GREATER, alpha * 0.5f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glAlphaFunc(GL_GREATER, 0.5f);
		}

		glCallList(tss->farDispList);
	}
}



void CAdvTreeDrawer::DrawShadowPass()
{
	CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_SHADOW);
	Shader::IProgramObject* po = NULL;

	const float treeDistance = oldTreeDistance;
	const int activeFarTex = (cam->GetDir().z < 0.0f)? treeGen->farTex[0] : treeGen->farTex[1];
	const bool drawDetailed = (treeDistance >= 4.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, activeFarTex);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	CAdvTreeSquareShadowPassDrawer drawer;
	const int cx = drawer.cx = (int)(cam->GetPos().x / (SQUARE_SIZE * TREE_SQUARE_SIZE));
	const int cy = drawer.cy = (int)(cam->GetPos().z / (SQUARE_SIZE * TREE_SQUARE_SIZE));

	drawer.drawDetailed = drawDetailed;
	drawer.td = this;
	drawer.treeDistance = treeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE;

	// draw with extraSize=1
	readMap->GridVisibility(nullptr, &drawer, drawer.treeDistance * 2.0f, TREE_SQUARE_SIZE, 1);

	if (drawDetailed) {
		const int xstart = Clamp(cx - 2, 0, mapDims.mapx / TREE_SQUARE_SIZE - 1);
		const int xend   = Clamp(cx + 2, 0, mapDims.mapx / TREE_SQUARE_SIZE - 1);
		const int ystart = Clamp(cy - 2, 0, mapDims.mapy / TREE_SQUARE_SIZE - 1);
		const int yend   = Clamp(cy + 2, 0, mapDims.mapy / TREE_SQUARE_SIZE - 1);

		glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
		glEnable(GL_TEXTURE_2D);

		po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE_NEAR);
		po->Enable();

		if (globalRendering->haveGLSL) {
			po->SetUniform3fv(1, &cam->GetRight()[0]);
			po->SetUniform3fv(2, &cam->GetUp()[0]);
		} else {
			po->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			po->SetUniform4f(13, cam->GetRight().x, cam->GetRight().y, cam->GetRight().z, 0.0f);
			po->SetUniform4f(9,  cam->GetUp().x,    cam->GetUp().y,    cam->GetUp().z,    0.0f);
			po->SetUniform4f(11, 1.0f, 1.0f, 1.0f, 0.85f                           );
			po->SetUniform4f(12, 0.0f, 0.0f, 0.0f, 0.20f * (1.0f / MAX_TREE_HEIGHT));   // w = alpha/height modifier
		}

		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		CVertexArray* va = GetVertexArray();
		va->Initialize();


		static std::vector<FadeTree> fadeTrees;

		fadeTrees.clear();
		fadeTrees.reserve(3000);

		for (TreeSquareStruct* pTSS = &treeSquares[ystart * treesX]; pTSS <= &treeSquares[yend * treesX]; pTSS += treesX) {
			for (TreeSquareStruct* tss = pTSS + xstart; tss <= pTSS + xend; ++tss) {
				tss->lastSeen = gs->frameNum;
				va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

				for (auto ti = tss->trees.cbegin(); ti != tss->trees.cend(); ++ti) {
					const TreeStruct* ts = &(*ti);
					const CFeature* f = featureHandler->GetFeature(ts->id);

					if (f == NULL)
						continue;
					if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
						continue;
					if (!cam->InView(ts->pos + float3(0, MAX_TREE_HEIGHT / 2, 0), MAX_TREE_HEIGHT / 2 + 150))
						continue;

					const float camDist = (ts->pos - cam->GetPos()).SqLength();
					int type = ts->type;
					float dy = 0.0f;
					unsigned int dispList;

					if (type < 8) {
						dy = 0.5f;
						dispList = treeGen->pineDL + type;
					} else {
						type -= 8;
						dy = 0;
						dispList = treeGen->leafDL + type;
					}

					if (camDist < SQUARE_SIZE * SQUARE_SIZE * 110 * 110) {
						po->SetUniform3f((globalRendering->haveGLSL? 3: 10), ts->pos.x, ts->pos.y, ts->pos.z);
						glCallList(dispList);
					} else if (camDist < SQUARE_SIZE * SQUARE_SIZE * 125 * 125) {
						const float relDist = (ts->pos.distance(cam->GetPos()) - SQUARE_SIZE * 110) / (SQUARE_SIZE * 15);

						glAlphaFunc(GL_GREATER, 0.8f + relDist * 0.2f);
						po->SetUniform3f((globalRendering->haveGLSL? 3: 10), ts->pos.x, ts->pos.y, ts->pos.z);
						glCallList(dispList);
						glAlphaFunc(GL_GREATER, 0.5f);

						// save for second pass
						fadeTrees.emplace_back();
						FadeTree& ft = fadeTrees.back();

						ft.id = f->id;
						ft.type = type;

						ft.pos = ts->pos;

						ft.relDist = relDist;
						ft.deltaY = dy;
					} else {
						CAdvTreeDrawer::DrawTreeVertex(va, ts->pos, type * 0.125f, dy, false);
					}
				}
			}
		}


		po->SetUniform3f((globalRendering->haveGLSL? 3: 10), 0.0f, 0.0f, 0.0f);

		for (auto fti = fallingTrees.cbegin(); fti != fallingTrees.cend(); ++fti) {
			// const CFeature* f = featureHandler->GetFeature(fti->id);
			const float3 pos = fti->pos - UpVector * (fti->fallPos * 20);

			// featureID is invalid for falling trees
			// if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
			//   continue;
			if (!losHandler->InLos(pos, gu->myAllyTeam))
				continue;
			if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT / 2.0f)), MAX_TREE_HEIGHT / 2.0f))
				continue;

			const float ang = fti->fallPos * PI;

			const float3 yvec(fti->dir.x * std::sin(ang), std::cos(ang), fti->dir.z * std::sin(ang));
			const float3 zvec((yvec.cross(RgtVector)).ANormalize());
			const float3 xvec(zvec.cross(yvec));

			CMatrix44f transMatrix(pos, xvec, yvec, zvec);

			glPushMatrix();
			glMultMatrixf(&transMatrix[0]);

			int type = fti->type;
			int dispList;

			if (type < 8) {
				dispList = treeGen->pineDL + type;
			} else {
				type -= 8;
				dispList = treeGen->leafDL + type;
			}

			glCallList(dispList);
			glPopMatrix();
		}

		po->Disable();
		po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE_FAR);
		po->Enable();

		// draw far-distance trees
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
		va->DrawArrayT(GL_QUADS);

		// draw faded mid-distance trees
		for (const FadeTree& fTree: fadeTrees) {
			const CFeature* f = featureHandler->GetFeature(fTree.id);

			if (f == NULL)
				continue;
			if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
				continue;
			if (!cam->InView(fTree.pos, MAX_TREE_HEIGHT / 2.0f))
				continue;

			va = GetVertexArray();
			va->Initialize();
			va->CheckInitSize(12 * VA_SIZE_T);

			CAdvTreeDrawer::DrawTreeVertex(va, fTree.pos, fTree.type * 0.125f, fTree.deltaY, false);

			glAlphaFunc(GL_GREATER, 1.0f - (fTree.relDist * 0.5f));
			va->DrawArrayT(GL_QUADS);
		}

		po->Disable();
	}

	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
}


void CAdvTreeDrawer::AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir)
{
	const float len = dir.Length();

	if (len > 500.0f)
		return;

	fallingTrees.emplace_back();
	FallingTree& ft = fallingTrees.back();

	ft.id = treeID;
	ft.type = treeType;
	ft.pos = pos;
	ft.dir = dir / len;
	ft.speed = std::max(0.01f, len * 0.0004f);
	ft.fallPos = 0.0f;
}

