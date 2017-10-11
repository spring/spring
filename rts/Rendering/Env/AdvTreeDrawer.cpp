/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AdvTreeDrawer.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
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
	LoadTreeShaders();

	treeGen.Init();
	treeGen.CreateFarTex(treeShaders[TREE_PROGRAM_BASIC]);

	lastListClean = 0;

	treeSquares.resize(nTrees);
}

CAdvTreeDrawer::~CAdvTreeDrawer()
{
	for (TreeSquareStruct& tss: treeSquares) {
		if (tss.dispList != 0)
			glDeleteLists(tss.dispList, 1);

		if (tss.farDispList != 0)
			glDeleteLists(tss.farDispList, 1);
	}

	shaderHandler->ReleaseProgramObjects("[TreeDrawer]");
}



void CAdvTreeDrawer::LoadTreeShaders() {
	treeShaders.fill(nullptr);

	const static std::string shaderNames[TREE_PROGRAM_LAST] = {
		"treeDefShader", // no-shadow default shader
		"treeAdvShader",
	};
	const static std::string shaderDefines[TREE_PROGRAM_LAST] = {
		"#define TREE_BASIC\n",
		"#define TREE_SHADOW\n",
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

	treeShaders[TREE_PROGRAM_BASIC] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_BASIC] + "GLSL");
	treeShaders[TREE_PROGRAM_BASIC]->AttachShaderObject(
		shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_BASIC], GL_VERTEX_SHADER)
	);
	treeShaders[TREE_PROGRAM_BASIC]->Link();

	treeShaders[TREE_PROGRAM_SHADOW] = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_SHADOW] + "GLSL");

	if (CShadowHandler::ShadowsSupported()) {
		treeShaders[TREE_PROGRAM_SHADOW]->AttachShaderObject(
			shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_SHADOW], GL_VERTEX_SHADER)
		);
		treeShaders[TREE_PROGRAM_SHADOW]->AttachShaderObject(
			shaderHandler->CreateShaderObject("GLSL/TreeFragProg.glsl", shaderDefines[TREE_PROGRAM_SHADOW], GL_FRAGMENT_SHADER)
		);
	}

	treeShaders[TREE_PROGRAM_SHADOW]->Link();

	// ND, NA: indices [0, numUniformNamesNDNA - 1]
	for (int i = 0; i < numUniformNamesNDNA; i++) {
		treeShaders[TREE_PROGRAM_BASIC ]->SetUniformLocation(uniformNamesNDNA[i]);
		treeShaders[TREE_PROGRAM_SHADOW]->SetUniformLocation(uniformNamesNDNA[i]);
	}

	// ND: index <numUniformNamesNDNA>
	treeShaders[TREE_PROGRAM_BASIC ]->SetUniformLocation("invMapSizePO2");
	treeShaders[TREE_PROGRAM_SHADOW]->SetUniformLocation("$UNUSED$");

	// NA, DA: indices [numUniformNamesNDNA + 1, numUniformNamesNDNA + numUniformNamesNADA]
	for (int i = 0; i < numUniformNamesNADA; i++) {
		treeShaders[TREE_PROGRAM_BASIC ]->SetUniformLocation("$UNUSED$");
		treeShaders[TREE_PROGRAM_SHADOW]->SetUniformLocation(uniformNamesNADA[i]);
	}

	treeShaders[TREE_PROGRAM_BASIC]->Enable();
	treeShaders[TREE_PROGRAM_BASIC]->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
	treeShaders[TREE_PROGRAM_BASIC]->SetUniform3fv(4, &sunLighting->groundDiffuseColor[0]);
	treeShaders[TREE_PROGRAM_BASIC]->SetUniform4f(6, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f);
	treeShaders[TREE_PROGRAM_BASIC]->Disable();
	treeShaders[TREE_PROGRAM_BASIC]->Validate();

	treeShaders[TREE_PROGRAM_SHADOW]->Enable();
	treeShaders[TREE_PROGRAM_SHADOW]->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
	treeShaders[TREE_PROGRAM_SHADOW]->SetUniform3fv(4, &sunLighting->groundDiffuseColor[0]);
	treeShaders[TREE_PROGRAM_SHADOW]->SetUniform1f(9, 1.0f - (sunLighting->groundShadowDensity * 0.5f));
	treeShaders[TREE_PROGRAM_SHADOW]->SetUniform1i(10, 0);
	treeShaders[TREE_PROGRAM_SHADOW]->SetUniform1i(11, 1);
	treeShaders[TREE_PROGRAM_SHADOW]->Disable();
	treeShaders[TREE_PROGRAM_SHADOW]->Validate();
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
	CAdvTreeSquareDrawer(
		CAdvTreeDrawer* td,
		CAdvTreeGenerator* gen,
		CCamera* cam,
		Shader::IProgramObject* po
	)
		: td(td)
		, gen(gen)
		, cam(cam)
		, po(po)
	{
	}

	void ResetState() {
		td = nullptr;
		gen = nullptr;
		cam = nullptr;
		po = nullptr;
	}

	void DrawQuad(int x, int y)
	{
		ITreeDrawer::TreeSquareStruct* tss = &td->treeSquares[(y * td->NumTreesX()) + x];
		CVertexArray* va = GetVertexArray();

		constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;

		const float3 camPos = cam->GetPos();
		const float3 sqrPos = {(x * sqrWorldSize * 1.0f) + (sqrWorldSize >> 1), 0.0f, (y * sqrWorldSize * 1.0f) + (sqrWorldSize >> 1)};

		if (sqrPos.SqDistance2D(camPos) > Square(td->GetDrawDistance()))
			return;

		{
			tss->lastSeen = gs->frameNum;

			va->Initialize();
			va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

			for (const ITreeDrawer::TreeStruct& ts: tss->trees) {
				const CFeature* f = featureHandler->GetFeature(ts.id);

				if (f == nullptr)
					continue;
				if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;
				if (!cam->InView(ts.pos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
					continue;

				po->SetUniform3fv(2, &ts.pos.x);

				if (ts.type < 8) {
					glCallList(gen->pineDL + ts.type);
				} else {
					glCallList(gen->leafDL + ts.type - 8);
				}
			}
		}
	}

public:
	CAdvTreeDrawer* td;
	CAdvTreeGenerator* gen;
	CCamera* cam;
	Shader::IProgramObject* po;
};



void CAdvTreeDrawer::DrawPass()
{
	// trees are never drawn in any special (non-opaque) pass
	CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_PLAYER);
	Shader::IProgramObject* treeShader = treeShaders[shadowHandler->ShadowsLoaded()];

	glDepthMask(GL_TRUE);

	{
		sky->SetupFog();
		treeShader->Enable();

		if (shadowHandler->ShadowsLoaded()) {
			shadowHandler->SetupShadowTexSampler(GL_TEXTURE0);

			treeShader->SetUniformMatrix4fv(7, false, shadowHandler->GetShadowMatrixRaw());
			treeShader->SetUniform4fv(8, &(shadowHandler->GetShadowParams().x));

			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, treeGen.barkTex);
			glActiveTexture(GL_TEXTURE0);
		} else {
			glBindTexture(GL_TEXTURE_2D, treeGen.barkTex);
		}


		treeShader->SetUniform3fv(0, &cam->GetRight()[0]);
		treeShader->SetUniform3fv(1, &cam->GetUp()[0]);
		treeShader->SetUniform2f(5, 0.20f * (1.0f / MAX_TREE_HEIGHT), 0.85f);


		glAlphaFunc(GL_GREATER, 0.5f);
		glDisable(GL_BLEND);

		CAdvTreeSquareDrawer drawer(this, &treeGen, camera, treeShader);
		readMap->GridVisibility(nullptr, &drawer, drawTreeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE * 2.0f, TREE_SQUARE_SIZE);

		// reset the world-offset
		treeShader->SetUniform3fv(2, &ZeroVector.x);

		// draw trees that have been marked as falling
		for (const FallingTree& ft: fallingTrees) {
			// const CFeature* f = featureHandler->GetFeature(ft.id);
			const float3 pos = ft.pos - UpVector * (ft.fallPos * 20);

			// featureID is invalid for falling trees
			// if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
			//   continue;
			if (!losHandler->InLos(pos, gu->myAllyTeam))
				continue;
			if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
				continue;

			const float ang = ft.fallPos * math::PI;

			const float3 yvec(ft.dir.x * std::sin(ang), std::cos(ang), ft.dir.z * std::sin(ang));
			const float3 zvec((yvec.cross(-RgtVector)).ANormalize());
			const float3 xvec(yvec.cross(zvec));

			const CMatrix44f transMatrix(pos, xvec, yvec, zvec);

			glPushMatrix();
			glMultMatrixf(transMatrix);

			if (ft.type < 8) {
				glCallList(treeGen.pineDL + ft.type);
			} else {
				glCallList(treeGen.leafDL + ft.type - 8);
			}

			glPopMatrix();
		}
	}

	if (shadowHandler->ShadowsLoaded()) {
		treeShader->Disable();

		// barkTex
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		shadowHandler->ResetShadowTexSampler(GL_TEXTURE0, true);
	}


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
	CAdvTreeSquareShadowPassDrawer(
		CAdvTreeDrawer* td,
		CAdvTreeGenerator* gen,
		CCamera* cam,
		Shader::IProgramObject* po
	)
		: td(td)
		, gen(gen)
		, cam(cam)
		, po(po)
	{
	}

	void ResetState() {
		td = nullptr;
		gen = nullptr;
		cam = nullptr;
		po = nullptr;
	}

	void DrawQuad(int x, int y)
	{
		ITreeDrawer::TreeSquareStruct* tss = &td->treeSquares[(y * td->NumTreesX()) + x];
		CVertexArray* va = GetVertexArray();

		constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;

		const float3 camPos = cam->GetPos();
		const float3 sqrPos = {(x * sqrWorldSize * 1.0f) + (sqrWorldSize >> 1), 0.0f, (y * sqrWorldSize * 1.0f) + (sqrWorldSize >> 1)};

		if (sqrPos.SqDistance2D(camPos) > Square(td->GetDrawDistance()))
			return;

		{
			tss->lastSeen = gs->frameNum;

			va->Initialize();
			va->EnlargeArrays(12 * tss->trees.size(), 0, VA_SIZE_T); //!alloc room for all tree vertexes

			for (const ITreeDrawer::TreeStruct& ts: tss->trees) {
				const CFeature* f = featureHandler->GetFeature(ts.id);

				if (f == nullptr)
					continue;
				if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
					continue;
				if (!cam->InView(ts.pos + float3(0, MAX_TREE_HEIGHT * 0.5f, 0), MAX_TREE_HEIGHT * 0.5f + 150))
					continue;

				po->SetUniform3fv(3, &ts.pos.x);

				if (ts.type < 8) {
					glCallList(gen->pineDL + ts.type);
				} else {
					glCallList(gen->leafDL + ts.type - 8);
				}
			}
		}
	}

public:
	CAdvTreeDrawer* td;
	CAdvTreeGenerator* gen;
	CCamera* cam;
	Shader::IProgramObject* po;
};



void CAdvTreeDrawer::DrawShadowPass()
{
	if (!drawTrees)
		return;

	CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_SHADOW);
	Shader::IProgramObject* po = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE);

	glEnable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, treeGen.barkTex);
		glEnable(GL_TEXTURE_2D);

		po->Enable();
		po->SetUniform3fv(1, &cam->GetRight()[0]);
		po->SetUniform3fv(2, &cam->GetUp()[0]);

		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);

		// note: use the player camera s.t. all trees it can see are shadowed
		// CAdvTreeSquareShadowPassDrawer drawer(this, &treeGen, camera, po);
		CAdvTreeSquareShadowPassDrawer drawer(this, &treeGen, CCamera::GetCamera(CCamera::CAMTYPE_PLAYER), po);
		readMap->GridVisibility(nullptr, &drawer, drawTreeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE * 2.0f, TREE_SQUARE_SIZE, 1);

		po->SetUniform3fv(3, &ZeroVector.x);

		for (const FallingTree& ft: fallingTrees) {
			// const CFeature* f = featureHandler->GetFeature(ft.id);
			const float3 pos = ft.pos - UpVector * (ft.fallPos * 20);

			// featureID is invalid for falling trees
			// if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
			//   continue;
			if (!losHandler->InLos(pos, gu->myAllyTeam))
				continue;
			if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
				continue;

			const float ang = ft.fallPos * math::PI;

			const float3 yvec(ft.dir.x * std::sin(ang), std::cos(ang), ft.dir.z * std::sin(ang));
			const float3 zvec((yvec.cross(RgtVector)).ANormalize());
			const float3 xvec(zvec.cross(yvec));

			const CMatrix44f transMatrix(pos, xvec, yvec, zvec);

			glPushMatrix();
			glMultMatrixf(transMatrix);

			if (ft.type < 8) {
				glCallList(treeGen.pineDL + ft.type);
			} else {
				glCallList(treeGen.leafDL + ft.type - 8);
			}

			glPopMatrix();
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

