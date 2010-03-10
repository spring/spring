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


CAdvTreeDrawer::CAdvTreeDrawer()
{
	LoadTreeShaders();

	treeGen = new CAdvTreeGenerator();
	treeGen->CreateFarTex(treeNearDefShader);
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
}



void CAdvTreeDrawer::LoadTreeShaders() {
	// no-shadow default shader
	treeNearDefShader = NULL;
	// these are only used when shadows are enabled
	// (which is not possible unless canUseShadows)
	treeNearAdvShader = NULL;
	treeDistAdvShader = NULL;

	if (gu->haveGLSL) {
		treeNearDefShader = shaderHandler->CreateProgramObject("[TreeDrawer]", "treeNearDefShaderGLSL", false);
		treeNearDefShader->AttachShaderObject(shaderHandler->CreateShaderObject("TreeVertProg.glsl", "#define TREE_BASIC\n", GL_VERTEX_SHADER));
		treeNearDefShader->Link();

		if (shadowHandler->canUseShadows) {
			treeNearAdvShader = shaderHandler->CreateProgramObject("[TreeDrawer]", "treeNearAdvShaderGLSL", false);
			treeNearAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("TreeVertProg.glsl", "#define TREE_NEAR\n", GL_VERTEX_SHADER));
			treeNearAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("TreeFragProg.glsl", "#define TREE_NEAR\n", GL_FRAGMENT_SHADER));
			treeNearAdvShader->Link();
			treeDistAdvShader = shaderHandler->CreateProgramObject("[TreeDrawer]", "treeDistAdvShaderGLSL", false);
			treeDistAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("TreeVertProg.glsl", "#define TREE_DIST\n", GL_VERTEX_SHADER));
			treeDistAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("TreeFragProg.glsl", "#define TREE_DIST\n", GL_FRAGMENT_SHADER));
			treeDistAdvShader->Link();
		}


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

		// ND, NA: indices [0, numUniformNamesNDNA - 1]
		for (int i = 0; i < numUniformNamesNDNA; i++) {
			treeNearDefShader->SetUniformLocation(uniformNamesNDNA[i]);
			treeNearAdvShader->SetUniformLocation(uniformNamesNDNA[i]);
			treeDistAdvShader->SetUniformLocation("$UNUSED$");
		}

		// ND: index <numUniformNamesNDNA>
		treeNearDefShader->SetUniformLocation("invMapSizePO2");
		treeNearAdvShader->SetUniformLocation("$UNUSED$");
		treeDistAdvShader->SetUniformLocation("$UNUSED$");

		// NA, DA: indices [numUniformNamesNDNA + 1, numUniformNamesNDNA + numUniformNamesNADA]
		for (int i = 0; i < numUniformNamesNADA; i++) {
			treeNearDefShader->SetUniformLocation("$UNUSED$");
			treeNearAdvShader->SetUniformLocation(uniformNamesNADA[i]);
			treeDistAdvShader->SetUniformLocation(uniformNamesNADA[i]);
		}

		treeNearDefShader->Enable();
		treeNearDefShader->SetUniform3fv(3, const_cast<float*>(&mapInfo->light.groundAmbientColor[0]));
		treeNearDefShader->SetUniform3fv(4, const_cast<float*>(&mapInfo->light.groundSunColor[0]));
		treeNearDefShader->SetUniform4f(6, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f);
		treeNearDefShader->Disable();

		treeNearAdvShader->Enable();
		treeNearAdvShader->SetUniform3fv(3, const_cast<float*>(&mapInfo->light.groundAmbientColor[0]));
		treeNearAdvShader->SetUniform3fv(4, const_cast<float*>(&mapInfo->light.groundSunColor[0]));
		treeNearAdvShader->SetUniform1f(9, 1.0f - (mapInfo->light.groundShadowDensity * 0.5f));
		treeNearAdvShader->SetUniform1i(10, 0);
		treeNearAdvShader->SetUniform1i(11, 1);
		treeNearAdvShader->Disable();

		treeDistAdvShader->Enable();
		treeDistAdvShader->SetUniform1f(9, 1.0f - (mapInfo->light.groundShadowDensity * 0.5f));
		treeDistAdvShader->SetUniform1i(10, 0);
		treeDistAdvShader->SetUniform1i(11, 1);
		treeDistAdvShader->Disable();
	} else {
		treeNearDefShader = shaderHandler->CreateProgramObject("[TreeDrawer]", "treeNearDefShaderARB", true);
		treeNearDefShader->AttachShaderObject(shaderHandler->CreateShaderObject("treeNS.vp", "", GL_VERTEX_PROGRAM_ARB));
		treeNearDefShader->Link();

		if (shadowHandler->canUseShadows) {
			treeNearAdvShader = shaderHandler->CreateProgramObject("[TreeDrawer]", "treeNearAdvShaderARB", true);
			treeNearAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("tree.vp", "", GL_VERTEX_PROGRAM_ARB));
			treeNearAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("treeFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			treeNearAdvShader->Link();
			treeDistAdvShader = shaderHandler->CreateProgramObject("[TreeDrawer]", "treeDistAdvShaderARB", true);
			treeDistAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("treeFar.vp", "", GL_VERTEX_PROGRAM_ARB));
			treeDistAdvShader->AttachShaderObject(shaderHandler->CreateShaderObject("treeFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			treeDistAdvShader->Link();
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



static CVertexArray* va;

static void inline SetArrayQ(float t1, float t2, float3 v)
{
	va->AddVertexQT(v, t1, t2);
}

inline void DrawTreeVertexA(float3 &ftpos, float dx, float dy) {
	SetArrayQ(TEX_LEAF_START_X1 + dx, TEX_LEAF_START_Y1 + dy, ftpos); ftpos.y += MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X1 + dx, TEX_LEAF_END_Y1   + dy, ftpos); ftpos.x -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X1   + dx, TEX_LEAF_END_Y1   + dy, ftpos); ftpos.y -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X1   + dx, TEX_LEAF_START_Y1 + dy, ftpos); ftpos.x += HALF_MAX_TREE_HEIGHT;

	ftpos.z += HALF_MAX_TREE_HEIGHT;

	SetArrayQ(TEX_LEAF_START_X2 + dx, TEX_LEAF_START_Y2 + dy, ftpos); ftpos.y += MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X2 + dx, TEX_LEAF_END_Y2   + dy, ftpos); ftpos.z -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X2   + dx, TEX_LEAF_END_Y2   + dy, ftpos); ftpos.y -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X2   + dx, TEX_LEAF_START_Y2 + dy, ftpos);

	ftpos.x += HALF_MAX_TREE_HEIGHT;
	ftpos.y += PART_MAX_TREE_HEIGHT;
}

inline void DrawTreeVertex(float3 pos, float dx, float dy, bool enlarge = true) {
	if (enlarge)
		va->EnlargeArrays(12, 0, VA_SIZE_T);

	float3 ftpos = pos;
	ftpos.x += HALF_MAX_TREE_HEIGHT;

	DrawTreeVertexA(ftpos, dx, dy);

	ftpos.z += MAX_TREE_HEIGHT;

	SetArrayQ(TEX_LEAF_START_X3 + dx, TEX_LEAF_START_Y3 + dy, ftpos); ftpos.z -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X3 + dx, TEX_LEAF_END_Y3   + dy, ftpos); ftpos.x -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3   + dx, TEX_LEAF_END_Y3   + dy, ftpos); ftpos.z += MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3   + dx, TEX_LEAF_START_Y3 + dy, ftpos);
}

inline void DrawTreeVertexMid(const float3 pos, float dx, float dy, bool enlarge = true) {
	if (enlarge)
		va->EnlargeArrays(12, 0, VA_SIZE_T);

	float3 ftpos = pos;
	ftpos.x += HALF_MAX_TREE_HEIGHT;

	DrawTreeVertexA(ftpos, dx, dy);

	ftpos.z += HALF_MAX_TREE_HEIGHT;

	SetArrayQ(TEX_LEAF_START_X3 + dx, TEX_LEAF_START_Y3 + dy,ftpos);
		ftpos.x -= HALF_MAX_TREE_HEIGHT;
		ftpos.z -= HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X3 + dx,TEX_LEAF_END_Y3 + dy,ftpos);
		ftpos.x -= HALF_MAX_TREE_HEIGHT;
		ftpos.z += HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3 + dx, TEX_LEAF_END_Y3 + dy,ftpos);
		ftpos.x += HALF_MAX_TREE_HEIGHT;
		ftpos.z += HALF_MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X3 + dx, TEX_LEAF_START_Y3 + dy, ftpos);
}

inline void DrawTreeVertexFar(float3 pos, float3 swd, float dx, float dy, bool enlarge = true) {
	if (enlarge)
		va->EnlargeArrays(4, 0, VA_SIZE_T);

	float3 base = pos + swd;

	SetArrayQ(TEX_LEAF_START_X1 + dx, TEX_LEAF_START_Y4 + dy, base); base.y += MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_START_X1 + dx, TEX_LEAF_END_Y4   + dy, base); base   -= (swd * 2.0f);
	SetArrayQ(TEX_LEAF_END_X1   + dx, TEX_LEAF_END_Y4   + dy, base); base.y -= MAX_TREE_HEIGHT;
	SetArrayQ(TEX_LEAF_END_X1   + dx, TEX_LEAF_START_Y4 + dy, base);
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
			va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

			tss->displist = glGenLists(1);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexMid(ts->pos, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexMid(ts->pos, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}
			glNewList(tss->displist, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
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
			va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(4 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector = dif;

			if (!tss->farDisplist)
				tss->farDisplist = glGenLists(1);

			const float3 side = UpVector.cross(dif);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexFar(ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexFar(ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->farDisplist, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
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

	float4 shadowParams = float4(shadowHandler->xmid, shadowHandler->ymid, shadowHandler->p17, shadowHandler->p18);

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();
	Shader::IProgramObject* treeShader = NULL;

	#define L mapInfo->light

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);

	if (gu->drawFog) {
		glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
		glEnable(GL_FOG);
	}


	if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);

		treeShader = treeDistAdvShader;
		treeShader->Enable();

		if (!gu->haveGLSL) {
			treeShader->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
			treeShader->SetUniform4f(10, L.groundAmbientColor.x, L.groundAmbientColor.y, L.groundAmbientColor.z, 1.0f);
			treeShader->SetUniform4f(11, 0.0f, 0.0f, 0.0f, 1.0f - (L.groundShadowDensity * 0.5f));
			treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);

			glMatrixMode(GL_MATRIX0_ARB);
			glLoadMatrixf(shadowHandler->shadowMatrix.m);
			glMatrixMode(GL_MODELVIEW);
		} else {
			treeShader->SetUniformMatrix4fv(7, false, &shadowHandler->shadowMatrix.m[0]);
			treeShader->SetUniform4fv(8, &shadowParams[0]);
		}

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, activeFarTex);
		glActiveTexture(GL_TEXTURE0);
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

	// draw far-trees using map-dependent grid-visibility
	oldTreeDistance = treeDistance;
	readmap->GridVisibility(camera, TREE_SQUARE_SIZE, drawer.treeDistance * 2.0f, &drawer);



	if (drawDetailed) {
		// draw near-trees
		const int xstart = std::max(                              0, cx - 2);
		const int xend   = std::min(gs->mapx / TREE_SQUARE_SIZE - 1, cx + 2);
		const int ystart = std::max(                              0, cy - 2);
		const int yend   = std::min(gs->mapy / TREE_SQUARE_SIZE - 1, cy + 2);

		if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
			treeShader->Disable();
			treeShader = treeNearAdvShader;
			treeShader->Enable();

			if (gu->haveGLSL) {
				treeShader->SetUniformMatrix4fv(7, false, &shadowHandler->shadowMatrix.m[0]);
				treeShader->SetUniform4fv(8, &shadowParams[0]);
			}

			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);
			glActiveTexture(GL_TEXTURE0);
		} else {
			glBindTexture(GL_TEXTURE_2D, treeGen->barkTex);

			treeShader = treeNearDefShader;
			treeShader->Enable();

			if (!gu->haveGLSL) {
				#define MX (gs->pwr2mapx * SQUARE_SIZE)
				#define MY (gs->pwr2mapy * SQUARE_SIZE)
				treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				treeShader->SetUniform4f(15, 1.0f / MX, 1.0f / MY, 1.0f / MX, 1.0f);
				#undef MX
				#undef MY
			}
		}


		if (gu->haveGLSL) {
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

		va = GetVertexArray();
		va->Initialize();

		static FadeTree fadeTrees[3000];
		FadeTree* pFT = fadeTrees;

		for (TreeSquareStruct* pTSS = trees + ystart * treesX; pTSS <= trees + yend * treesX; pTSS += treesX) {
			for (TreeSquareStruct* tss = pTSS + xstart; tss <= pTSS + xend; ++tss) {
				tss->lastSeen = gs->frameNum;
				va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

				for (std::map<int, TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
					const TreeStruct* ts = &ti->second;
					const float3 pos(ts->pos);

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

					if (camera->InView(pos + float3(0.0f, MAX_TREE_HEIGHT / 2.0f, 0.0f), MAX_TREE_HEIGHT / 2.0f)) {
						const float camDist = (pos - camera->pos).SqLength();

						if (camDist < (SQUARE_SIZE * SQUARE_SIZE * 110 * 110)) {
							// draw detailed tree
							treeShader->SetUniform3f(((gu->haveGLSL)? 2: 10), pos.x, pos.y, pos.z);
							glCallList(displist);
						} else if (camDist < (SQUARE_SIZE * SQUARE_SIZE * 125 * 125)) {
							// draw fading tree
							const float relDist = (pos.distance(camera->pos) - SQUARE_SIZE * 110) / (SQUARE_SIZE * 15);

							treeShader->SetUniform3f(((gu->haveGLSL)? 2: 10), pos.x, pos.y, pos.z);

							glAlphaFunc(GL_GREATER, 0.8f + relDist * 0.2f);
							glCallList(displist);
							glAlphaFunc(GL_GREATER, 0.5f);

							pFT->pos = pos;
							pFT->deltaY = dy;
							pFT->type = type;
							pFT->relDist = relDist;
							++pFT;
						} else {
							// draw undetailed tree
							DrawTreeVertex(pos, type * 0.125f, dy, false);
						}
					}
				}
			}
		}


		// draw trees that has been marked as falling
		treeShader->SetUniform3f(((gu->haveGLSL)? 2: 10), 0.0f, 0.0f, 0.0f);
		treeShader->SetUniform3f(((gu->haveGLSL)? 0: 13), camera->right.x, camera->right.y, camera->right.z);
		treeShader->SetUniform3f(((gu->haveGLSL)? 1:  9), camera->up.x,    camera->up.y,    camera->up.z   );

		for (std::list<FallingTree>::iterator fti = fallingTrees.begin(); fti != fallingTrees.end(); ++fti) {
			const float3 pos = fti->pos - UpVector * (fti->fallPos * 20);

			if (camera->InView(pos + float3(0.0f, MAX_TREE_HEIGHT / 2, 0.0f), MAX_TREE_HEIGHT / 2.0f)) {
				const float ang = fti->fallPos * PI;

				float3 up(fti->dir.x * sin(ang), cos(ang), fti->dir.z * sin(ang));
				float3 z(up.cross(float3(-1, 0, 0)));
				z.ANormalize();
				float3 x(up.cross(z));
				CMatrix44f transMatrix(pos, x, up, z);

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
			treeShader = treeDistAdvShader;
			treeShader->Enable();

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
			glActiveTexture(GL_TEXTURE0);
		} else {
			// keep using treeNearDefShader
			glBindTexture(GL_TEXTURE_2D, activeFarTex);
		}

		va->DrawArrayT(GL_QUADS);

		for (FadeTree* pFTree = fadeTrees; pFTree < pFT; ++pFTree) {
			// faded close trees
			va = GetVertexArray();
			va->Initialize();
			va->CheckInitSize(12 * VA_SIZE_T);

			DrawTreeVertex(pFTree->pos, pFTree->type * 0.125f, pFTree->deltaY, false);

			glAlphaFunc(GL_GREATER, 1.0f - (pFTree->relDist * 0.5f));
			va->DrawArrayT(GL_QUADS);
		}

		treeShader->Disable();
	}

	if (shadowHandler->drawShadows && !gd->DrawExtraTex()) {
		treeShader->Disable();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
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
			if (pTSS->lastSeen < gs->frameNum - 50 && pTSS->displist) {
				glDeleteLists(pTSS->displist, 1);
				pTSS->displist = 0;
			}
			if (pTSS->lastSeenFar < gs->frameNum - 50 && pTSS->farDisplist) {
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
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(12*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
			tss->displist=glGenLists(1);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexMid(ts->pos, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexMid(ts->pos, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->displist, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
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
			va=GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(4*tss->trees.size(),0,VA_SIZE_T); //!alloc room for all tree vertexes
			tss->viewVector=dif;
			if(!tss->farDisplist)
				tss->farDisplist=glGenLists(1);
			float3 up(0,1,0);
			float3 side=up.cross(dif);

			for (std::map<int, CAdvTreeDrawer::TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
				CAdvTreeDrawer::TreeStruct* ts = &ti->second;

				if (ts->type < 8) {
					DrawTreeVertexFar(ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type    ) * 0.125f, 0.5f, false);
				} else {
					DrawTreeVertexFar(ts->pos, side * HALF_MAX_TREE_HEIGHT, (ts->type - 8) * 0.125f, 0.0f, false);
				}
			}

			glNewList(tss->farDisplist, GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
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

		if (gu->haveGLSL) {
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
		va = GetVertexArray();
		va->Initialize();

		static FadeTree fadeTrees[3000];
		FadeTree* pFT = fadeTrees;

		for (TreeSquareStruct* pTSS = trees + ystart * treesX; pTSS <= trees + yend * treesX; pTSS += treesX) {
			for (TreeSquareStruct* tss = pTSS + xstart; tss <= pTSS + xend; ++tss) {
				tss->lastSeen = gs->frameNum;
				va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

				for (std::map<int, TreeStruct>::iterator ti = tss->trees.begin(); ti != tss->trees.end(); ++ti) {
					const TreeStruct* ts = &ti->second;
					const float3 pos(ts->pos);

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

					if (camera->InView(pos + float3(0, MAX_TREE_HEIGHT / 2, 0), MAX_TREE_HEIGHT / 2 + 150)) {
						const float camDist = (pos - camera->pos).SqLength();

						if (camDist < SQUARE_SIZE * SQUARE_SIZE * 110 * 110) {
							// draw detailed tree
							po->SetUniform3f((gu->haveGLSL? 3: 10), pos.x, pos.y, pos.z);
							glCallList(displist);
						} else if (camDist < SQUARE_SIZE * SQUARE_SIZE * 125 * 125) {
							// draw fading tree
							const float relDist = (pos.distance(camera->pos) - SQUARE_SIZE * 110) / (SQUARE_SIZE * 15);

							glAlphaFunc(GL_GREATER, 0.8f + relDist * 0.2f);
							po->SetUniform3f((gu->haveGLSL? 3: 10), pos.x, pos.y, pos.z);
							glCallList(displist);
							glAlphaFunc(GL_GREATER, 0.5f);

							pFT->pos = pos;
							pFT->deltaY = dy;
							pFT->type = type;
							pFT->relDist = relDist;
							++pFT;
						} else {
							// draw undetailed tree
							DrawTreeVertex(pos, type * 0.125f, dy, false);
						}
					}
				}
			}
		}

		// reset the world-offset
		po->SetUniform3f((gu->haveGLSL? 3: 10), 0.0f, 0.0f, 0.0f);

		// draw trees that have been marked as falling
		for (std::list<FallingTree>::iterator fti = fallingTrees.begin(); fti != fallingTrees.end(); ++fti) {
			const float3 pos = fti->pos - UpVector * (fti->fallPos * 20);

			if (camera->InView(pos + float3(0, MAX_TREE_HEIGHT / 2, 0), MAX_TREE_HEIGHT / 2)) {
				float ang = fti->fallPos * PI;

				float3 up(fti->dir.x * sin(ang), cos(ang), fti->dir.z * sin(ang));
				float3 z(up.cross(float3(1, 0, 0)));
				z.ANormalize();
				float3 x(z.cross(up));
				CMatrix44f transMatrix(pos, x, up, z);

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
		va->DrawArrayT(GL_QUADS);

		for (FadeTree* pFTree = fadeTrees; pFTree < pFT; ++pFTree) {
			// faded close trees
			va = GetVertexArray();
			va->Initialize();
			va->CheckInitSize(12 * VA_SIZE_T);

			DrawTreeVertex(pFTree->pos, pFTree->type * 0.125f, pFTree->deltaY, false);

			glAlphaFunc(GL_GREATER, 1.0f - (pFTree->relDist * 0.5f));
			va->DrawArrayT(GL_QUADS);
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

