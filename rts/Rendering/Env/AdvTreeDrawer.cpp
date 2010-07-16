/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "AdvTreeDrawer.h"
#include "AdvTreeGenerator.h"
#include "GrassDrawer.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.hpp"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/ShadowHandler.h"
#include "System/LogOutput.h"
#include "System/Matrix44f.h"
#include "System/GlobalUnsynced.h"

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
static const float DOUBLE_MAX_TREE_HEIGHT = MAX_TREE_HEIGHT * 2.0f;


CAdvTreeDrawer::CAdvTreeDrawer(): CBaseTreeDrawer()
{
	LoadTreeShaders();

	treeGen = new CAdvTreeGenerator();
	treeGen->CreateFarTex(treeShaders[TREE_PROGRAM_NEAR_BASIC]);
	grassDrawer = new CGrassDrawer();

	oldTreeDistance = 4;
	lastListClean = 0;
	treesX = gs->mapx / TREE_SQUARE_SIZE;
	treesY = gs->mapy / TREE_SQUARE_SIZE;
	nTrees = treesX * treesY;
	trees = new TreeSquareStruct[nTrees];

	for (TreeSquareStruct* pTSS = trees; pTSS < trees + nTrees; ++pTSS) {
		pTSS->lastSeen    = 0;
		pTSS->lastSeenFar = 0;
		pTSS->viewVector  = UpVector;
		pTSS->displist    = 0;
		pTSS->farDisplist = 0;
	}
}

CAdvTreeDrawer::~CAdvTreeDrawer()
{
	for (TreeSquareStruct* pTSS = trees; pTSS < trees + nTrees; ++pTSS) {
		if (pTSS->displist) { glDeleteLists(pTSS->displist, 1); }
		if (pTSS->farDisplist) { glDeleteLists(pTSS->farDisplist, 1); }
	}

	delete[] trees;
	delete treeGen;
	delete grassDrawer;

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

		if (shadowHandler->canUseShadows) {
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
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->SetUniform3fv(3, const_cast<float*>(&mapInfo->light.groundAmbientColor[0]));
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->SetUniform3fv(4, const_cast<float*>(&mapInfo->light.groundSunColor[0]));
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->SetUniform4f(6, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Disable();

		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Enable();
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform3fv(3, const_cast<float*>(&mapInfo->light.groundAmbientColor[0]));
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform3fv(4, const_cast<float*>(&mapInfo->light.groundSunColor[0]));
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform1f(9, 1.0f - (mapInfo->light.groundShadowDensity * 0.5f));
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform1i(10, 0);
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->SetUniform1i(11, 1);
		treeShaders[TREE_PROGRAM_NEAR_SHADOW]->Disable();

		treeShaders[TREE_PROGRAM_DIST_SHADOW]->Enable();
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform3fv(3, const_cast<float*>(&mapInfo->light.groundAmbientColor[0]));
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform1f(9, 1.0f - (mapInfo->light.groundShadowDensity * 0.5f));
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform1i(10, 0);
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->SetUniform1i(11, 1);
		treeShaders[TREE_PROGRAM_DIST_SHADOW]->Disable();
	} else {
		treeShaders[TREE_PROGRAM_NEAR_BASIC] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_NEAR_BASIC] + "ARB", true);
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->AttachShaderObject(shaderHandler->CreateShaderObject("ARB/treeNS.vp", "", GL_VERTEX_PROGRAM_ARB));
		treeShaders[TREE_PROGRAM_NEAR_BASIC]->Link();

		if (shadowHandler->canUseShadows) {
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
	GML_STDMUTEX_LOCK(tree); // Update

	for (std::list<FallingTree>::iterator fti = fallingTrees.begin(); fti != fallingTrees.end(); ) {
		fti->fallPos += (fti->speed * 0.1f);

		if (fti->fallPos > 1.0f) {
			// remove the tree
			std::list<FallingTree>::iterator prev = fti++;
			fallingTrees.erase(prev);
		} else {
			fti->speed += (sin(fti->fallPos) * 0.04f);
			++fti;
		}
	}
}



static CVertexArray* varr = NULL;

static void inline SetArrayQ(CVertexArray* va, float t1, float t2, const float3& v)
{
	va->AddVertexQT(v, t1, t2);
}

inline void DrawTreeVertexA(CVertexArray* va, float3& ftpos, float dx, float dy) {
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

inline void DrawTreeVertex(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true) {
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

inline void DrawTreeVertexMid(CVertexArray* va, const float3& pos, float dx, float dy, bool enlarge = true) {
	if (enlarge)
		va->EnlargeArrays(12, 0, VA_SIZE_T);

	float3 ftpos = pos;
	ftpos.x += HALF_MAX_TREE_HEIGHT;

	DrawTreeVertexA(va, ftpos, dx, dy);

	ftpos.z += HALF_MAX_TREE_HEIGHT;

	SetArrayQ(va, TEX_LEAF_START_X3 + dx, TEX_LEAF_START_Y3 + dy,ftpos);
		ftpos.x -= HALF_MAX_TREE_HEIGHT;
		ftpos.z -= HALF_MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X3 + dx,TEX_LEAF_END_Y3 + dy,ftpos);
		ftpos.x -= HALF_MAX_TREE_HEIGHT;
		ftpos.z += HALF_MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X3 + dx, TEX_LEAF_END_Y3 + dy,ftpos);
		ftpos.x += HALF_MAX_TREE_HEIGHT;
		ftpos.z += HALF_MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X3 + dx, TEX_LEAF_START_Y3 + dy, ftpos);
}

inline void DrawTreeVertexFar(CVertexArray* va, const float3& pos, const float3& swd, float dx, float dy, bool enlarge = true) {
	if (enlarge)
		va->EnlargeArrays(4, 0, VA_SIZE_T);

	float3 base = pos + swd;

	SetArrayQ(va, TEX_LEAF_START_X1 + dx, TEX_LEAF_START_Y4 + dy, base); base.y += MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_START_X1 + dx, TEX_LEAF_END_Y4   + dy, base); base   -= (swd * 2.0f);
	SetArrayQ(va, TEX_LEAF_END_X1   + dx, TEX_LEAF_END_Y4   + dy, base); base.y -= MAX_TREE_HEIGHT;
	SetArrayQ(va, TEX_LEAF_END_X1   + dx, TEX_LEAF_START_Y4 + dy, base);
}




struct CAdvTreeSquareDrawer: CReadMap::IQuadDrawer
{
	CAdvTreeSquareDrawer(): td(NULL) {
	}

	void DrawQuad(int x, int y);

	CAdvTreeDrawer* td;
	int cx, cy;
	float treeDistance;
	bool drawDetailed;
};

void CAdvTreeSquareDrawer::DrawQuad(int x, int y)
{
	const int treesX = td->treesX;
	CAdvTreeDrawer::TreeSquareStruct* tss = &td->trees[y * treesX + x];

	if (abs(cy - y) <= 2 && abs(cx - x) <= 2 && drawDetailed) {
		// skip the closest squares
		return;
	}

	float3 dif;
		dif.x = camera->pos.x - (x * SQUARE_SIZE * TREE_SQUARE_SIZE + SQUARE_SIZE * TREE_SQUARE_SIZE / 2);
		dif.y = 0.0f;
		dif.z = camera->pos.z - (y * SQUARE_SIZE * TREE_SQUARE_SIZE + SQUARE_SIZE * TREE_SQUARE_SIZE / 2);

	const float dist = dif.Length();
	const float distfactor = dist / treeDistance;

	dif /= dist;

	if (distfactor < MID_TREE_DIST_FACTOR) {
		// midle-distance trees
		tss->lastSeen = gs->frameNum;

		if (!tss->displist) {
			varr = GetVertexArray();
			varr->Initialize();
			varr->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

			tss->displist = glGenLists(1);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexMid(varr, ts->pos, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexMid(varr, ts->pos, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->displist, GL_COMPILE);
			varr->DrawArrayT(GL_QUADS);
			glEndList();
		}

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_BLEND);
		glAlphaFunc(GL_GREATER, 0.5f);
		glCallList(tss->displist);
	}
	else if (distfactor < FAR_TREE_DIST_FACTOR) {
		// far-distance trees
		tss->lastSeenFar = gs->frameNum;

		if (!tss->farDisplist || dif.dot(tss->viewVector) < 0.97f) {
			varr = GetVertexArray();
			varr->Initialize();
			varr->EnlargeArrays(4 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector = dif;

			if (!tss->farDisplist)
				tss->farDisplist = glGenLists(1);

			const float3 side = UpVector.cross(dif);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexFar(varr, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexFar(varr, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->farDisplist, GL_COMPILE);
			varr->DrawArrayT(GL_QUADS);
			glEndList();
		}

		if (distfactor > FADE_TREE_DIST_FACTOR) {
			// faded far trees
			const float alpha = 1.0f - ((distfactor - FADE_TREE_DIST_FACTOR) / (FAR_TREE_DIST_FACTOR - FADE_TREE_DIST_FACTOR));

			glEnable(GL_BLEND);
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glAlphaFunc(GL_GREATER, alpha * 0.5f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glDisable(GL_BLEND);
			glAlphaFunc(GL_GREATER, 0.5f);
		}

		glCallList(tss->farDisplist);
	}
}




void CAdvTreeDrawer::Draw(float treeDistance, bool drawReflection)
{
	const int activeFarTex = (camera->forward.z < 0.0f)? treeGen->farTex[0]: treeGen->farTex[1];
	const bool drawDetailed = ((treeDistance >= 4.0f) || drawReflection);

	const int cx = int(camera->pos.x / (SQUARE_SIZE * TREE_SQUARE_SIZE));
	const int cy = int(camera->pos.z / (SQUARE_SIZE * TREE_SQUARE_SIZE));

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	Shader::IProgramObject* treeShader = NULL;

	#define L mapInfo->light

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);

	if (globalRendering->drawFog) {
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
		glEnable(GL_FOG);
	}


	if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);

		treeShader = treeShaders[TREE_PROGRAM_DIST_SHADOW];
		treeShader->Enable();

		if (globalRendering->haveGLSL) {
			treeShader->SetUniformMatrix4fv(7, false, &shadowHandler->shadowMatrix.m[0]);
			treeShader->SetUniform4fv(8, const_cast<float*>(&(shadowHandler->GetShadowParams().x)));
		} else {
			treeShader->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
			treeShader->SetUniform4f(10, L.groundAmbientColor.x, L.groundAmbientColor.y, L.groundAmbientColor.z, 1.0f);
			treeShader->SetUniform4f(11, 0.0f, 0.0f, 0.0f, 1.0f - (L.groundShadowDensity * 0.5f));
			treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);

			glMatrixMode(GL_MATRIX0_ARB);
			glLoadMatrixf(shadowHandler->shadowMatrix.m);
			glMatrixMode(GL_MODELVIEW);
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
	}



	CAdvTreeSquareDrawer drawer;
	drawer.td = this;
	drawer.cx = cx;
	drawer.cy = cy;
	drawer.treeDistance = treeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE;
	drawer.drawDetailed = drawDetailed;

	GML_STDMUTEX_LOCK(tree); // Draw

	oldTreeDistance = treeDistance;

	// draw far-trees using map-dependent grid-visibility
	readmap->GridVisibility(camera, TREE_SQUARE_SIZE, drawer.treeDistance * 2.0f, &drawer);


	if (drawDetailed) {
		// draw near-trees
		const int xstart = std::max(                              0, cx - 2);
		const int xend   = std::min(gs->mapx / TREE_SQUARE_SIZE - 1, cx + 2);
		const int ystart = std::max(                              0, cy - 2);
		const int yend   = std::min(gs->mapy / TREE_SQUARE_SIZE - 1, cy + 2);

		if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
			treeShader->Disable();
			treeShader = treeShaders[TREE_PROGRAM_NEAR_SHADOW];
			treeShader->Enable();

			if (globalRendering->haveGLSL) {
				treeShader->SetUniformMatrix4fv(7, false, &shadowHandler->shadowMatrix.m[0]);
				treeShader->SetUniform4fv(8, const_cast<float*>(&(shadowHandler->GetShadowParams().x)));
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
				#define MX (gs->pwr2mapx * SQUARE_SIZE)
				#define MY (gs->pwr2mapy * SQUARE_SIZE)
				treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				treeShader->SetUniform4f(15, 1.0f / MX, 1.0f / MY, 1.0f / MX, 1.0f);
				#undef MX
				#undef MY
			}
		}


		if (globalRendering->haveGLSL) {
			treeShader->SetUniform3fv(0, &camera->right[0]);
			treeShader->SetUniform3fv(1, &camera->up[0]);
			treeShader->SetUniform2f(5, 0.20f * (1.0f / MAX_TREE_HEIGHT), 0.85f);
		} else {
			treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			treeShader->SetUniform3f(13, camera->right.x, camera->right.y, camera->right.z);
			treeShader->SetUniform3f( 9, camera->up.x,    camera->up.y,    camera->up.z   );
			treeShader->SetUniform4f(11, L.groundSunColor.x,     L.groundSunColor.y,     L.groundSunColor.z,     0.85f);
			treeShader->SetUniform4f(14, L.groundAmbientColor.x, L.groundAmbientColor.y, L.groundAmbientColor.z, 0.85f);
			treeShader->SetUniform4f(12, 0.0f, 0.0f, 0.0f, 0.20f * (1.0f / MAX_TREE_HEIGHT)); // w = alpha/height modifier
		}


		glAlphaFunc(GL_GREATER, 0.5f);
		glDisable(GL_BLEND);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		varr = GetVertexArray();
		varr->Initialize();

		static FadeTree fadeTrees[3000];
		FadeTree* pFT = fadeTrees;


		for (TreeSquareStruct* pTSS = trees + ystart * treesX; pTSS <= trees + yend * treesX; pTSS += treesX) {
			for (TreeSquareStruct* tss = pTSS + xstart; tss <= pTSS + xend; ++tss) {
				tss->lastSeen = gs->frameNum;
				varr->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

				for (std::map<int, TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
					const TreeStruct* ts = &ti->second;
					const float3 pos(ts->pos);

					if (!camera->InView(pos + float3(0.0f, MAX_TREE_HEIGHT / 2.0f, 0.0f), MAX_TREE_HEIGHT / 2.0f)) {
						continue;
					}

					const float camDist = (pos - camera->pos).SqLength();
					int type = ts->type;
					float dy = 0.0f;
					unsigned int displist;

					if (type < 8) {
						dy = 0.5f;
						displist = treeGen->pineDL + type;
					} else {
						type -= 8;
						dy = 0.0f;
						displist = treeGen->leafDL + type;
					}

					if (camDist < (SQUARE_SIZE * SQUARE_SIZE * 110 * 110)) {
						// draw detailed near-distance tree (same as mid-distance trees without alpha)
						treeShader->SetUniform3f(((globalRendering->haveGLSL)? 2: 10), pos.x, pos.y, pos.z);
						glCallList(displist);
					} else if (camDist < (SQUARE_SIZE * SQUARE_SIZE * 125 * 125)) {
						// draw mid-distance tree
						const float relDist = (pos.distance(camera->pos) - SQUARE_SIZE * 110) / (SQUARE_SIZE * 15);

						treeShader->SetUniform3f(((globalRendering->haveGLSL)? 2: 10), pos.x, pos.y, pos.z);

						glAlphaFunc(GL_GREATER, 0.8f + relDist * 0.2f);
						glCallList(displist);
						glAlphaFunc(GL_GREATER, 0.5f);

						// save for second pass
						pFT->pos = pos;
						pFT->deltaY = dy;
						pFT->type = type;
						pFT->relDist = relDist;
						++pFT;
					} else {
						// draw far-distance tree
						DrawTreeVertex(varr, pos, type * 0.125f, dy, false);
					}
				}
			}
		}


		// reset the world-offset
		treeShader->SetUniform3f(((globalRendering->haveGLSL)? 2: 10), 0.0f, 0.0f, 0.0f);

		// draw trees that have been marked as falling
		for (std::list<FallingTree>::iterator fti = fallingTrees.begin(); fti != fallingTrees.end(); ++fti) {
			const float3 pos = fti->pos - UpVector * (fti->fallPos * 20);

			if (camera->InView(pos + float3(0.0f, MAX_TREE_HEIGHT / 2, 0.0f), MAX_TREE_HEIGHT / 2.0f)) {
				const float ang = fti->fallPos * PI;

				const float3 yvec(fti->dir.x * sin(ang), cos(ang), fti->dir.z * sin(ang));
				const float3 zvec((yvec.cross(float3(-1.0f, 0.0f, 0.0f))).ANormalize());
				const float3 xvec(yvec.cross(zvec));

				CMatrix44f transMatrix(pos, xvec, yvec, zvec);

				glPushMatrix();
				glMultMatrixf(&transMatrix[0]);

				int type = fti->type;
				int displist = 0;

				if (type < 8) {
					displist = treeGen->pineDL + type;
				} else {
					type -= 8;
					displist = treeGen->leafDL + type;
				}

				glCallList(displist);
				glPopMatrix();
			}
		}


		if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
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
		varr->DrawArrayT(GL_QUADS);

		// draw faded mid-distance trees
		for (FadeTree* pFTree = fadeTrees; pFTree < pFT; ++pFTree) {
			varr = GetVertexArray();
			varr->Initialize();
			varr->CheckInitSize(12 * VA_SIZE_T);

			DrawTreeVertex(varr, pFTree->pos, pFTree->type * 0.125f, pFTree->deltaY, false);

			glAlphaFunc(GL_GREATER, 1.0f - (pFTree->relDist * 0.5f));
			varr->DrawArrayT(GL_QUADS);
		}
	}

	if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
		treeShader->Disable();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_ALPHA_TEST);



	// clean out squares from memory that are no longer visible
	const int startClean = lastListClean * 20 % (nTrees);
	const int endClean = gs->frameNum * 20 % (nTrees);

	lastListClean = gs->frameNum;

	if (startClean > endClean) {
		for (TreeSquareStruct* pTSS = trees + startClean; pTSS < trees + nTrees; ++pTSS) {
			if ((pTSS->lastSeen < gs->frameNum - 50) && pTSS->displist) {
				glDeleteLists(pTSS->displist, 1);
				pTSS->displist = 0;
			}
			if ((pTSS->lastSeenFar < gs->frameNum - 50) && pTSS->farDisplist) {
				glDeleteLists(pTSS->farDisplist, 1);
				pTSS->farDisplist = 0;
			}
		}
		for (TreeSquareStruct* pTSS = trees; pTSS < trees + endClean; ++pTSS) {
			if ((pTSS->lastSeen < gs->frameNum - 50) && pTSS->displist) {
				glDeleteLists(pTSS->displist, 1);
				pTSS->displist = 0;
			}
			if ((pTSS->lastSeenFar < gs->frameNum - 50) && pTSS->farDisplist) {
				glDeleteLists(pTSS->farDisplist, 1);
				pTSS->farDisplist = 0;
			}
		}
	} else {
		for (TreeSquareStruct* pTSS = trees + startClean; pTSS < trees + endClean; ++pTSS) {
			if ((pTSS->lastSeen < gs->frameNum - 50) && pTSS->displist) {
				glDeleteLists(pTSS->displist, 1);
				pTSS->displist = 0;
			}
			if ((pTSS->lastSeenFar < gs->frameNum - 50) && pTSS->farDisplist) {
				glDeleteLists(pTSS->farDisplist, 1);
				pTSS->farDisplist = 0;
			}
		}
	}

	#undef L
}



struct CAdvTreeSquareDrawer_SP: CReadMap::IQuadDrawer
{
	void DrawQuad(int x, int y);

	CAdvTreeDrawer* td;
	int cx, cy;
	bool drawDetailed;
	float treeDistance;
};

void CAdvTreeSquareDrawer_SP::DrawQuad(int x, int y)
{
	const int treesX = td->treesX;
	CAdvTreeDrawer::TreeSquareStruct* tss = &td->trees[y * treesX + x];

	if (abs(cy - y) <= 2 && abs(cx - x) <= 2 && drawDetailed) {
		// skip the closest squares
		return;
	}

	float3 dif;
		dif.x = camera->pos.x - (x * SQUARE_SIZE * TREE_SQUARE_SIZE + SQUARE_SIZE * TREE_SQUARE_SIZE / 2);
		dif.y = 0.0f;
		dif.z = camera->pos.z - (y * SQUARE_SIZE * TREE_SQUARE_SIZE + SQUARE_SIZE * TREE_SQUARE_SIZE / 2);
	const float dist = dif.Length();
	const float distfactor = dist / treeDistance;

	dif /= dist;

	if (distfactor < MID_TREE_DIST_FACTOR) {
		// midle distance trees
		tss->lastSeen = gs->frameNum;

		if (!tss->displist) {
			varr = GetVertexArray();
			varr->Initialize();
			varr->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes
			tss->displist = glGenLists(1);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexMid(varr, ts->pos, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexMid(varr, ts->pos, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->displist, GL_COMPILE);
			varr->DrawArrayT(GL_QUADS);
			glEndList();
		}

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glAlphaFunc(GL_GREATER, 0.5f);
		glCallList(tss->displist);
	}
	else if (distfactor < FAR_TREE_DIST_FACTOR) {
		// far trees
		tss->lastSeenFar = gs->frameNum;

		if (!tss->farDisplist || dif.dot(tss->viewVector) < 0.97f) {
			varr = GetVertexArray();
			varr->Initialize();
			varr->EnlargeArrays(4 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector = dif;

			if (!tss->farDisplist)
				tss->farDisplist = glGenLists(1);

			const float3 side = UpVector.cross(dif);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexFar(varr, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexFar(varr, ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->farDisplist, GL_COMPILE);
			varr->DrawArrayT(GL_QUADS);
			glEndList();
		}
		if (distfactor > FADE_TREE_DIST_FACTOR) {
			// faded far trees
			const float alpha = 1.0f - (distfactor - FADE_TREE_DIST_FACTOR) / (FAR_TREE_DIST_FACTOR - FADE_TREE_DIST_FACTOR);
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
			glAlphaFunc(GL_GREATER, alpha * 0.5f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glAlphaFunc(GL_GREATER, 0.5f);
		}
		glCallList(tss->farDisplist);
	}
}



void CAdvTreeDrawer::DrawShadowPass(void)
{
	const float treeDistance = oldTreeDistance;
	const int activeFarTex = (camera->forward.z < 0.0f)? treeGen->farTex[0] : treeGen->farTex[1];
	const bool drawDetailed = (treeDistance >= 4.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, activeFarTex);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	CAdvTreeSquareDrawer_SP drawer;
	const int cx = drawer.cx = (int)(camera->pos.x / (SQUARE_SIZE * TREE_SQUARE_SIZE));
	const int cy = drawer.cy = (int)(camera->pos.z / (SQUARE_SIZE * TREE_SQUARE_SIZE));

	drawer.drawDetailed = drawDetailed;
	drawer.td = this;
	drawer.treeDistance = treeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE;

	Shader::IProgramObject* po = NULL;

	GML_STDMUTEX_LOCK(tree); // DrawShadowPass

	// draw with extraSize=1
	readmap->GridVisibility(camera, TREE_SQUARE_SIZE, drawer.treeDistance * 2.0f, &drawer, 1);

	if (drawDetailed) {
		const int xstart = std::max(                              0, cx - 2);
		const int xend   = std::min(gs->mapx / TREE_SQUARE_SIZE - 1, cx + 2);
		const int ystart = std::max(                              0, cy - 2);
		const int yend   = std::min(gs->mapy / TREE_SQUARE_SIZE - 1, cy + 2);

		glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
		glEnable(GL_TEXTURE_2D);

		po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE_NEAR);
		po->Enable();

		if (globalRendering->haveGLSL) {
			po->SetUniform3fv(1, &camera->right[0]);
			po->SetUniform3fv(2, &camera->up[0]);
		} else {
			po->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			po->SetUniform4f(13, camera->right.x, camera->right.y, camera->right.z, 0.0f);
			po->SetUniform4f(9,  camera->up.x,    camera->up.y,    camera->up.z,    0.0f);
			po->SetUniform4f(11, 1.0f, 1.0f, 1.0f, 0.85f                           );
			po->SetUniform4f(12, 0.0f, 0.0f, 0.0f, 0.20f * (1.0f / MAX_TREE_HEIGHT));   // w = alpha/height modifier
		}

		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		varr = GetVertexArray();
		varr->Initialize();

		static FadeTree fadeTrees[3000];
		FadeTree* pFT = fadeTrees;

		for (TreeSquareStruct* pTSS = trees + ystart * treesX; pTSS <= trees + yend * treesX; pTSS += treesX) {
			for (TreeSquareStruct* tss = pTSS + xstart; tss <= pTSS + xend; ++tss) {
				tss->lastSeen = gs->frameNum;
				varr->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

				for (std::map<int, TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
					const TreeStruct* ts = &ti->second;
					const float3 pos(ts->pos);

					if (!camera->InView(pos + float3(0, MAX_TREE_HEIGHT / 2, 0), MAX_TREE_HEIGHT / 2 + 150)) {
						continue;
					}

					const float camDist = (pos - camera->pos).SqLength();
					int type = ts->type;
					float dy = 0.0f;
					unsigned int displist;

					if (type < 8) {
						dy = 0.5f;
						displist = treeGen->pineDL + type;
					} else {
						type -= 8;
						dy = 0;
						displist = treeGen->leafDL + type;
					}

					if (camDist < SQUARE_SIZE * SQUARE_SIZE * 110 * 110) {
						po->SetUniform3f((globalRendering->haveGLSL? 3: 10), pos.x, pos.y, pos.z);
						glCallList(displist);
					} else if (camDist < SQUARE_SIZE * SQUARE_SIZE * 125 * 125) {
						const float relDist = (pos.distance(camera->pos) - SQUARE_SIZE * 110) / (SQUARE_SIZE * 15);

						glAlphaFunc(GL_GREATER, 0.8f + relDist * 0.2f);
						po->SetUniform3f((globalRendering->haveGLSL? 3: 10), pos.x, pos.y, pos.z);
						glCallList(displist);
						glAlphaFunc(GL_GREATER, 0.5f);

						pFT->pos = pos;
						pFT->deltaY = dy;
						pFT->type = type;
						pFT->relDist = relDist;
						++pFT;
					} else {
						DrawTreeVertex(varr, pos, type * 0.125f, dy, false);
					}
				}
			}
		}


		po->SetUniform3f((globalRendering->haveGLSL? 3: 10), 0.0f, 0.0f, 0.0f);

		for (std::list<FallingTree>::iterator fti = fallingTrees.begin(); fti != fallingTrees.end(); ++fti) {
			const float3 pos = fti->pos - UpVector * (fti->fallPos * 20);

			if (camera->InView(pos + float3(0, MAX_TREE_HEIGHT / 2, 0), MAX_TREE_HEIGHT / 2)) {
				const float ang = fti->fallPos * PI;

				const float3 yvec(fti->dir.x * sin(ang), cos(ang), fti->dir.z * sin(ang));
				const float3 zvec((yvec.cross(float3(1.0f, 0.0f, 0.0f))).ANormalize());
				const float3 xvec(zvec.cross(yvec));

				CMatrix44f transMatrix(pos, xvec, yvec, zvec);

				glPushMatrix();
				glMultMatrixf(&transMatrix[0]);

				int type = fti->type;
				int displist;

				if (type < 8) {
					displist = treeGen->pineDL + type;
				} else {
					type -= 8;
					displist = treeGen->leafDL + type;
				}

				glCallList(displist);
				glPopMatrix();
			}
		}

		po->Disable();
		po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE_FAR);
		po->Enable();

		glBindTexture(GL_TEXTURE_2D, activeFarTex);
		varr->DrawArrayT(GL_QUADS);

		for (FadeTree* pFTree = fadeTrees; pFTree < pFT; ++pFTree) {
			// faded close trees
			varr = GetVertexArray();
			varr->Initialize();
			varr->CheckInitSize(12 * VA_SIZE_T);

			DrawTreeVertex(varr, pFTree->pos, pFTree->type * 0.125f, pFTree->deltaY, false);

			glAlphaFunc(GL_GREATER, 1.0f - (pFTree->relDist * 0.5f));
			varr->DrawArrayT(GL_QUADS);
		}

		po->Disable();
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
}

void CAdvTreeDrawer::DrawGrass(void)
{
	if (drawTrees) {
		grassDrawer->Draw();
	}
}

void CAdvTreeDrawer::ResetPos(const float3& pos)
{
	const int x = (int) pos.x / TREE_SQUARE_SIZE / SQUARE_SIZE;
	const int y = (int) pos.z / TREE_SQUARE_SIZE / SQUARE_SIZE;
	TreeSquareStruct* pTSS = trees + y * treesX + x;

	if (pTSS->displist) {
		delDispLists.push_back(pTSS->displist);
		pTSS->displist = 0;
	}
	if (pTSS->farDisplist) {
		delDispLists.push_back(pTSS->farDisplist);
		pTSS->farDisplist = 0;
	}
	grassDrawer->ResetPos(pos);
}

void CAdvTreeDrawer::AddTree(int type, float3 pos, float size)
{
	GML_STDMUTEX_LOCK(tree); // AddTree

	TreeStruct ts;
	ts.pos = pos;
	ts.type = type;
	const int hash = (int)pos.x + ((int)(pos.z)) * 20000;
	const int square =
		((int)pos.x) / (SQUARE_SIZE * TREE_SQUARE_SIZE) +
		((int)pos.z) / (SQUARE_SIZE * TREE_SQUARE_SIZE) * treesX;
	trees[square].trees[hash] = ts;
	ResetPos(pos);
}

void CAdvTreeDrawer::DeleteTree(float3 pos)
{
	GML_STDMUTEX_LOCK(tree); // DeleteTree

	const int hash = (int)pos.x + ((int)(pos.z)) * 20000;
	const int square =
		((int)pos.x) / (SQUARE_SIZE * TREE_SQUARE_SIZE) +
		((int)pos.z) / (SQUARE_SIZE * TREE_SQUARE_SIZE) * treesX;

	trees[square].trees.erase(hash);

	ResetPos(pos);
}

int CAdvTreeDrawer::AddFallingTree(float3 pos, float3 dir, int type)
{
	GML_STDMUTEX_LOCK(tree); // AddFallingTree

	FallingTree ft;

	ft.pos=pos;
	dir.y=0;
	float s=dir.Length();
	if(s>500)
		return 0;
	ft.dir=dir/s;
	ft.speed=std::max(0.01f,s*0.0004f);
	ft.type=type;
	ft.fallPos=0;

	fallingTrees.push_back(ft);
	return 0;
}

void CAdvTreeDrawer::AddGrass(float3 pos)
{
	GML_STDMUTEX_LOCK(tree); // AddGrass

	grassDrawer->AddGrass(pos);
}

void CAdvTreeDrawer::RemoveGrass(int x, int z)
{
	GML_STDMUTEX_LOCK(tree); // RemoveGrass

	grassDrawer->RemoveGrass(x,z);
}

