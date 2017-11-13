/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AdvTreeDrawer.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
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
		"#define TREE_SHADOW 0\n",
		"#define TREE_SHADOW 1\n",
	};

	const static char* uniformNames[] = {
		"cameraDirX",          // VP, idx  0
		"cameraDirY",          // VP, idx  1
		"treeOffset",          // VP, idx  2
		"groundAmbientColor",  // FP, idx  3
		"groundDiffuseColor",  // FP, idx  4
		"alphaModifiers",      // VP, idx  5

		"fogParams",           // VP, idx  6
		"fogColor",            // FP, idx  7

		"diffuseTex",          // FP, idx  8
		"shadowTex",           // FP, idx  9 (unused when TREE_SHADOW=0)
		"shadowMatrix",        // VP, idx 10 (unused when TREE_SHADOW=0)
		"shadowParams",        // VP, idx 11 (unused when TREE_SHADOW=0)
		"groundShadowDensity", // FP, idx 12 (unused when TREE_SHADOW=0)

		"fallTreeMat",         // VP, idx 13
		"viewMat",             // VP, idx 14
		"projMat",             // VP, idx 15
	};


	Shader::IProgramObject*& tpb = treeShaders[TREE_PROGRAM_BASIC];
	Shader::IProgramObject*& tps = treeShaders[TREE_PROGRAM_SHADOW];

	tpb = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_BASIC] + "GLSL");
	tps = shaderHandler->CreateProgramObject("[TreeDrawer]", shaderNames[TREE_PROGRAM_SHADOW] + "GLSL");

	tpb->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_BASIC], GL_VERTEX_SHADER));
	tpb->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeFragProg.glsl", shaderDefines[TREE_PROGRAM_BASIC], GL_FRAGMENT_SHADER));

	if (CShadowHandler::ShadowsSupported()) {
		tps->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeVertProg.glsl", shaderDefines[TREE_PROGRAM_SHADOW], GL_VERTEX_SHADER));
		tps->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/TreeFragProg.glsl", shaderDefines[TREE_PROGRAM_SHADOW], GL_FRAGMENT_SHADER));
	}

	tpb->Link();
	tps->Link();

	for (size_t i = 0; i < (sizeof(uniformNames) / sizeof(uniformNames[0])); i++) {
		tpb->SetUniformLocation(uniformNames[i]);
		tps->SetUniformLocation(uniformNames[i]);
	}

	for (Shader::IProgramObject* tp: {tpb, tps}) {
		tp->Enable();
		tp->SetUniform3fv(3, &sunLighting->groundAmbientColor.x);
		tp->SetUniform3fv(4, &sunLighting->groundDiffuseColor.x);
		tp->SetUniform2f(5, 0.20f * (1.0f / MAX_TREE_HEIGHT), 0.85f);
		tp->SetUniform3fv(6, &ZeroVector.x);
		tp->SetUniform4fv(7, &sky->fogColor.x);
		tp->SetUniform1i(8, 1);
		tp->SetUniform1i(9, 0);
		tp->SetUniform1f(12, 1.0f - (sunLighting->groundShadowDensity * 0.5f));
		tp->Disable();
		tp->Validate();
	}
}



void CAdvTreeDrawer::Update()
{
	for (int i = 0; i < 2; i++) {
		std::vector<FallingTree>& v = fallingTrees[i];

		for (size_t n = 0; n < v.size(); /*no-op*/) {
			FallingTree& ft = v[n];

			ft.fallPos += (ft.speed * 0.1f);
			ft.speed += (std::sin(ft.fallPos) * 0.04f);

			if (ft.fallPos > 1.0f) {
				// remove the tree
				v[n] = v.back();
				v.pop_back();
				continue;
			}

			n += 1;
		}
	}
}




struct CAdvTreeSquareDrawer : public CReadMap::IQuadDrawer
{
	CAdvTreeSquareDrawer(CAdvTreeDrawer* _atd, CCamera* _cam): atd(_atd), cam(_cam) {}

	void ResetState() {}
	void DrawQuad(int x, int y)
	{
		constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;

		const float3 camPos = cam->GetPos();
		const float3 sqrPos = {(x * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f, 0.0f, (y * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f};

		// soft cutoff (gradual density reduction)
		const float drawProb = std::min(1.0f, Square(atd->GetDrawDistance()) / sqrPos.SqDistance2D(camPos));

		if (drawProb > 0.001f) {
			rng.SetSeed(rng.GetInitSeed());

			for (int i = 0; i < 2; i++) {
				const auto& treeStructs = atd->treeSquares[(y * atd->NumTreesX()) + x].trees[i];

				if (treeStructs.empty())
					continue;

				// bind once per draw-quad per tree-type (bush, pine) batch
				atd->BindTreeGeometry(i * NUM_TREE_TYPES);

				for (const ITreeDrawer::TreeStruct& ts: treeStructs) {
					if (rng.NextFloat() > drawProb)
						continue;

					atd->DrawTree(ts, 2);
				}
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

	#if 0
	// assume all trees in a draw-quad are visible
	if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
		return;
	#endif

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(posOffsetIdx, &ts.pos.x);

	// done by DrawQuad
	// BindTreeGeometry(ts.type);
	DrawTreeGeometry(ts.type);
}

// only called from ITreeDrawer; no in-view test
void CAdvTreeDrawer::DrawTree(const float3& pos, int treeType, int posOffsetIdx)
{
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(posOffsetIdx, &pos.x);

	BindTreeGeometry(treeType);
	DrawTreeGeometry(treeType);
}


void CAdvTreeDrawer::BindTreeGeometry(int treeType) const {
	switch (treeType < NUM_TREE_TYPES) {
		case  true: { treeGen.BindPineTreeBuffer(treeType                 ); } break;
		case false: { treeGen.BindBushTreeBuffer(treeType - NUM_TREE_TYPES); } break;
		default   : {                                                        } break;
	}
}

void CAdvTreeDrawer::DrawTreeGeometry(int treeType) const {
	assert(treeType >= 0);

	switch (treeType < NUM_TREE_TYPES) {
		case  true: { treeGen.DrawPineTreeBuffer(treeType                 ); } break;
		case false: { treeGen.DrawBushTreeBuffer(treeType - NUM_TREE_TYPES); } break;
		default   : {                                                        } break;
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
		glBindTexture(GL_TEXTURE_2D, treeGen.GetBarkTex());

		shadowHandler->SetupShadowTexSampler(GL_TEXTURE0, true);

		treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(10, false, shadowHandler->GetShadowViewMatrixRaw());
		treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform4fv(11, shadowHandler->GetShadowParams());
	} else {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, treeGen.GetBarkTex());
	}


	const float3& cameraDirX = cam->GetRight();
	const float3& cameraDirY = cam->GetUp();
	const float3   fogParams = {sky->fogStart, sky->fogEnd, globalRendering->viewRange};

	const CMatrix44f& treeMat = CMatrix44f::Identity();
	const CMatrix44f& viewMat = camera->GetViewMatrix();
	const CMatrix44f& projMat = camera->GetProjectionMatrix();

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(0, &cameraDirX.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(1, &cameraDirY.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(6, &fogParams.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(13, false, &treeMat.m[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(14, false, &viewMat.m[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(15, false, &projMat.m[0]);


	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GREATER, 0.5f);
	glDisable(GL_BLEND);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}

void CAdvTreeDrawer::ResetDrawState()
{
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0); // bound by BindTreeGeometry

	if (shadowHandler->ShadowsLoaded()) {
		// barkTex
		// glActiveTexture(GL_TEXTURE1);

		shadowHandler->ResetShadowTexSampler(GL_TEXTURE0, true);
	} else {
		glActiveTexture(GL_TEXTURE0);
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
	glBindTexture(GL_TEXTURE_2D, treeGen.GetBarkTex()); // alpha-mask


	const CMatrix44f& treeMat = CMatrix44f::Identity();
	const CMatrix44f& viewMat = shadowHandler->GetShadowViewMatrix();
	const CMatrix44f& projMat = shadowHandler->GetShadowProjMatrix();

	treeShaders[TREE_PROGRAM_ACTIVE] = ipo;
	treeShaders[TREE_PROGRAM_ACTIVE]->Enable();
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(4, &cam->GetRight()[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(5, &cam->GetUp()[0]);

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(1, false, &viewMat.m[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(2, false, &projMat.m[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(3, false, &treeMat.m[0]);


	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}

void CAdvTreeDrawer::ResetShadowDrawState()
{
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	treeShaders[TREE_PROGRAM_ACTIVE]->Disable();
	treeShaders[TREE_PROGRAM_ACTIVE] = nullptr;

	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
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

		// reset the world-offset; not used by falling trees
		ipo->SetUniform3fv(2, &ZeroVector.x);

		// draw trees that have been marked as falling
		for (int i = 0; i < 2; i++) {
			BindTreeGeometry(i * NUM_TREE_TYPES);

			for (const FallingTree& ft: fallingTrees[i]) {
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

				ipo->SetUniformMatrix4fv(13, false, CMatrix44f(pos, xvec, yvec, zvec));
				DrawTreeGeometry(ft.type);
			}
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
		constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;

		const float3 camPos = cam->GetPos();
		const float3 sqrPos = {(x * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f, 0.0f, (y * sqrWorldSize + (sqrWorldSize >> 1)) * 1.0f};

		const float drawProb = std::min(1.0f, Square(atd->GetDrawDistance()) / sqrPos.SqDistance2D(camPos));

		if (drawProb > 0.001f) {
			rng.SetSeed(rng.GetInitSeed());

			for (int i = 0; i < 2; i++) {
				const auto& treeStructs = atd->treeSquares[(y * atd->NumTreesX()) + x].trees[i];

				if (treeStructs.empty())
					continue;

				atd->BindTreeGeometry(i * NUM_TREE_TYPES);

				for (const ITreeDrawer::TreeStruct& ts: treeStructs) {
					if (rng.NextFloat() > drawProb)
						continue;

					atd->DrawTree(ts, 6);
				}
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

	{
		SetupShadowDrawState(cam, ipo);

		// note: use the player camera s.t. all trees it can see are shadowed
		// CAdvTreeSquareShadowPassDrawer drawer(this, camera);
		CAdvTreeSquareShadowPassDrawer drawer(this, CCamera::GetCamera(CCamera::CAMTYPE_PLAYER));
		readMap->GridVisibility(nullptr, &drawer, drawTreeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE * 2.0f, TREE_SQUARE_SIZE, 1);

		ipo->SetUniform3fv(6, &ZeroVector.x);

		for (int i = 0; i < 2; i++) {
			BindTreeGeometry(i * NUM_TREE_TYPES);

			for (const FallingTree& ft: fallingTrees[i]) {
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

				ipo->SetUniformMatrix4fv(3, false, CMatrix44f(pos, xvec, yvec, zvec));
				DrawTreeGeometry(ft.type);
			}
		}

		ResetShadowDrawState();
	}
}


void CAdvTreeDrawer::AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir)
{
	const float len = dir.Length();

	if (len > 500.0f)
		return;

	fallingTrees[treeType >= NUM_TREE_TYPES].emplace_back();
	FallingTree& ft = fallingTrees[treeType >= NUM_TREE_TYPES].back();

	ft.id = treeID;
	ft.type = treeType;
	ft.pos = pos;
	ft.dir = dir / len;
	ft.speed = std::max(0.01f, len * 0.0004f);
	ft.fallPos = 0.0f;
}

