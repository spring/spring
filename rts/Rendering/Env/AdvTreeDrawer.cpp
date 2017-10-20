/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AdvTreeDrawer.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/ShadowHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "System/GlobalRNG.h"
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

static const float PART_MAX_TREE_HEIGHT = MAX_TREE_HEIGHT * 0.4f;
static const float HALF_MAX_TREE_HEIGHT = MAX_TREE_HEIGHT * 0.5f;

// global; sequence-id should be shared by CAdvTreeSquare*Drawer
static CGlobalUnsyncedRNG rng;


CAdvTreeDrawer::CAdvTreeDrawer(): ITreeDrawer()
{
	LoadTreeShaders();

	treeGen.Init();
	treeGen.CreateFarTex(treeShaders[TREE_PROGRAM_BASIC]);
	rng.SetSeed(reinterpret_cast<CGlobalUnsyncedRNG::rng_val_type>(this), true);

	treeSquares.resize(nTrees);
}

CAdvTreeDrawer::~CAdvTreeDrawer()
{
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
	const static char* uniformNamesNDNA[numUniformNamesNDNA] = {
		"cameraDirX",          // VP
		"cameraDirY",          // VP
		"treeOffset",          // VP
		"groundAmbientColor",  // VP + FP
		"groundDiffuseColor",  // VP
		"alphaModifiers",      // VP
	};
	const static int numUniformNamesNADA = 5;
	const char* uniformNamesNADA[numUniformNamesNADA] = {
		"shadowMatrix",        // VP
		"shadowParams",        // VP
		"groundShadowDensity", // FP
		"shadowTex",           // FP
		"diffuseTex",          // FP
	};

	Shader::IProgramObject*& tpb = treeShaders[TREE_PROGRAM_BASIC];
	Shader::IProgramObject*& tps = treeShaders[TREE_PROGRAM_SHADOW];

	tpb = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_BASIC] + "GLSL");
	tps = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_SHADOW] + "GLSL");

	tpb->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_BASIC], GL_VERTEX_SHADER));

	if (CShadowHandler::ShadowsSupported()) {
		tps->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_SHADOW], GL_VERTEX_SHADER));
		tps->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeFragProg.glsl", shaderDefines[TREE_PROGRAM_SHADOW], GL_FRAGMENT_SHADER));
	}

	tpb->Link();
	tps->Link();

	// ND, NA: indices [0, numUniformNamesNDNA - 1]
	for (int i = 0; i < numUniformNamesNDNA; i++) {
		tpb->SetUniformLocation(uniformNamesNDNA[i]);
		tps->SetUniformLocation(uniformNamesNDNA[i]);
	}

	// ND: index <numUniformNamesNDNA>
	tpb->SetUniformLocation("invMapSizePO2");
	tps->SetUniformLocation("$UNUSED$");

	// NA, DA: indices [numUniformNamesNDNA + 1, numUniformNamesNDNA + numUniformNamesNADA]
	for (int i = 0; i < numUniformNamesNADA; i++) {
		tpb->SetUniformLocation("$UNUSED$");
		tps->SetUniformLocation(uniformNamesNADA[i]);
	}

	tpb->Enable();
	tpb->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
	tpb->SetUniform3fv(4, &sunLighting->groundDiffuseColor[0]);
	tpb->SetUniform4f(6, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f);
	tpb->Disable();
	tpb->Validate();

	tps->Enable();
	tps->SetUniform3fv(3, &sunLighting->groundAmbientColor[0]);
	tps->SetUniform3fv(4, &sunLighting->groundDiffuseColor[0]);
	tps->SetUniform1f(9, 1.0f - (sunLighting->groundShadowDensity * 0.5f));
	tps->SetUniform1i(10, 0);
	tps->SetUniform1i(11, 1);
	tps->Disable();
	tps->Validate();
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




struct CAdvTreeSquareDrawer : public CReadMap::IQuadDrawer
{
	CAdvTreeSquareDrawer(CAdvTreeDrawer* _atd, CCamera* _cam): atd(_atd), cam(_cam) {}

	void ResetState() {}
	void DrawQuad(int x, int y)
	{
		const auto& tss = atd->treeSquares[(y * atd->NumTreesX()) + x].trees;

		constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;

		const float3 camPos = cam->GetPos();
		const float3 sqrPos = {(x * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f, 0.0f, (y * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f};

		// soft cutoff (gradual density reduction)
		const float drawProb = std::min(1.0f, Square(atd->GetDrawDistance()) / sqrPos.SqDistance2D(camPos));

		if (drawProb > 0.001f) {
			rng.SetSeed(rng.GetInitSeed());

			for (const ITreeDrawer::TreeStruct& ts: tss) {
				if (rng.NextFloat() > drawProb)
					continue;

				atd->DrawTree(ts, 2);
			}
		}
	}

private:
	CAdvTreeDrawer* atd;
	CCamera* cam;
};




void CAdvTreeDrawer::DrawTree(const TreeStruct& ts, int posOffsetIdx)
{
	const CFeature* f = featureHandler->GetFeature(ts.id);

	if (f == nullptr)
		return;
	if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
		return;

	DrawTree(ts.pos, ts.type, posOffsetIdx);
}

void CAdvTreeDrawer::DrawTree(const float3& pos, int treeType, int posOffsetIdx)
{
	#if 0
	// redundant
	if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
		return;
	#endif

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(posOffsetIdx, &pos.x);

	if (treeType < 8) {
		glCallList(treeGen.pineDL + treeType);
	} else {
		glCallList(treeGen.leafDL + treeType - 8);
	}
}




void CAdvTreeDrawer::SetupDrawState() { SetupDrawState(CCamera::GetCamera(CCamera::CAMTYPE_PLAYER), treeShaders[shadowHandler->ShadowsLoaded()]); }
void CAdvTreeDrawer::SetupDrawState(const CCamera* cam, Shader::IProgramObject* ipo)
{
	treeShaders[TREE_PROGRAM_ACTIVE] = ipo;
	treeShaders[TREE_PROGRAM_ACTIVE]->Enable();

	sky->SetupFog();

	if (shadowHandler->ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, treeGen.barkTex);

		shadowHandler->SetupShadowTexSampler(GL_TEXTURE0, true);

		treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(7, false, shadowHandler->GetShadowMatrixRaw());
		treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform4fv(8, &(shadowHandler->GetShadowParams().x));
	} else {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, treeGen.barkTex);
	}


	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(0, &cam->GetRight()[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(1, &cam->GetUp()[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform2f(5, 0.20f * (1.0f / MAX_TREE_HEIGHT), 0.85f);


	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.5f);
	glDisable(GL_BLEND);
}

void CAdvTreeDrawer::ResetDrawState()
{
	if (shadowHandler->ShadowsLoaded()) {
		// barkTex
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		shadowHandler->ResetShadowTexSampler(GL_TEXTURE0, true);
	} else {
		glDisable(GL_TEXTURE_2D);
	}

	treeShaders[TREE_PROGRAM_ACTIVE]->Disable();
	treeShaders[TREE_PROGRAM_ACTIVE] = nullptr;
}


void CAdvTreeDrawer::SetupShadowDrawState() { SetupShadowDrawState(CCamera::GetCamera(CCamera::CAMTYPE_SHADOW), shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE)); }
void CAdvTreeDrawer::SetupShadowDrawState(const CCamera* cam, Shader::IProgramObject* ipo)
{
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, treeGen.barkTex);
	glEnable(GL_TEXTURE_2D);

	treeShaders[TREE_PROGRAM_ACTIVE] = ipo;
	treeShaders[TREE_PROGRAM_ACTIVE]->Enable();
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(1, &cam->GetRight()[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(2, &cam->GetUp()[0]);

	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);
}

void CAdvTreeDrawer::ResetShadowDrawState()
{
	treeShaders[TREE_PROGRAM_ACTIVE]->Disable();
	treeShaders[TREE_PROGRAM_ACTIVE] = nullptr;

	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
}




void CAdvTreeDrawer::DrawPass()
{
	// trees are never drawn in any special (non-opaque) pass
	CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_PLAYER);
	Shader::IProgramObject* ipo = treeShaders[shadowHandler->ShadowsLoaded()];

	{
		SetupDrawState(cam, ipo);

		CAdvTreeSquareDrawer drawer(this, cam);
		readMap->GridVisibility(nullptr, &drawer, drawTreeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE * 2.0f, TREE_SQUARE_SIZE);

		// reset the world-offset
		ipo->SetUniform3fv(2, &ZeroVector.x);

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

		ResetDrawState();
	}
}



struct CAdvTreeSquareShadowPassDrawer: public CReadMap::IQuadDrawer
{
	CAdvTreeSquareShadowPassDrawer(CAdvTreeDrawer* _atd, CCamera* _cam): atd(_atd), cam(_cam) {}

	void ResetState() {}
	void DrawQuad(int x, int y)
	{
		const auto& tss = atd->treeSquares[(y * atd->NumTreesX()) + x].trees;

		constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;

		const float3 camPos = cam->GetPos();
		const float3 sqrPos = {(x * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f, 0.0f, (y * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f};

		const float drawProb = std::min(1.0f, Square(atd->GetDrawDistance()) / sqrPos.SqDistance2D(camPos));

		if (drawProb > 0.001f) {
			rng.SetSeed(rng.GetInitSeed());

			for (const ITreeDrawer::TreeStruct& ts: tss) {
				if (rng.NextFloat() > drawProb)
					continue;

				atd->DrawTree(ts, 3);
			}
		}
	}

private:
	CAdvTreeDrawer* atd;
	CCamera* cam;
};



void CAdvTreeDrawer::DrawShadowPass()
{
	CCamera* cam = CCamera::GetCamera(CCamera::CAMTYPE_SHADOW);
	Shader::IProgramObject* ipo = shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE);

	SetupShadowDrawState(cam, ipo);

	// note: use the player camera s.t. all trees it can see are shadowed
	// CAdvTreeSquareShadowPassDrawer drawer(this, camera);
	CAdvTreeSquareShadowPassDrawer drawer(this, CCamera::GetCamera(CCamera::CAMTYPE_PLAYER));
	readMap->GridVisibility(nullptr, &drawer, drawTreeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE * 2.0f, TREE_SQUARE_SIZE, 1);

	ipo->SetUniform3fv(3, &ZeroVector.x);

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

	ResetShadowDrawState();
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

