/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AdvTreeDrawer.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
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


struct CAdvTreeSquareDrawer : public CReadMap::IQuadDrawer
{
	void ResetState() override {
		inViewQuads.clear();
		inViewQuads.reserve(32);
	}
	void DrawQuad(int x, int y) override {
		inViewQuads.emplace_back(x, y);
	}

public:
	std::vector<int2> inViewQuads;
};


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

static CAdvTreeSquareDrawer squareDrawer;


CAdvTreeDrawer::CAdvTreeDrawer()
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
		"alphaTestCtrl",       // FP, idx  2
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
		"gammaExponent",       // FP, idx 13

		"treeMat",             // VP, idx TREE_MAT_IDX
		"viewMat",             // VP, idx VIEW_MAT_IDX
		"projMat",             // VP, idx PROJ_MAT_IDX
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

	for (const auto& uniformName: uniformNames) {
		tpb->SetUniformLocation(uniformName);
		tps->SetUniformLocation(uniformName);
	}

	for (Shader::IProgramObject* tp: {tpb, tps}) {
		tp->Enable();
		tp->SetUniform3fv(3, &sunLighting->groundAmbientColor.x);
		tp->SetUniform3fv(4, &sunLighting->groundDiffuseColor.x);
		tp->SetUniform2f(5, 0.20f * (1.0f / MAX_TREE_HEIGHT), 0.85f);
		tp->SetUniform3fv(6, &ZeroVector.x);
		tp->SetUniform4fv(7, sky->fogColor);
		tp->SetUniform1i(8, 1);
		tp->SetUniform1i(9, 0);
		tp->SetUniform1f(12, 1.0f - (sunLighting->groundShadowDensity * 0.5f));
		tp->SetUniform1f(13, globalRendering->gammaExponent);
		tp->Disable();
		tp->Validate();
	}
}



void CAdvTreeDrawer::Update()
{
	{
		CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

		updateVisibility |= (prvUpdateCamPos != cam->GetPos());
		updateVisibility |= (prvUpdateCamDir != cam->GetDir());

		if (updateVisibility) {
			prvUpdateCamPos = cam->GetPos();
			prvUpdateCamDir = cam->GetDir();

			squareDrawer.ResetState();
			readMap->GridVisibility(nullptr, &squareDrawer, drawTreeDistance * SQUARE_SIZE * TREE_SQUARE_SIZE * 2.0f, TREE_SQUARE_SIZE);

			updateVisibility = false;
		}
	}

	for (std::vector<FallingTree>& v : fallingTrees) {

		for (size_t n = 0; n < v.size(); /*no-op*/) {
			FallingTree& ft = v[n];

			// angle relative to positive y-axis, divided by PI
			ft.fallAngle += (ft.fallSpeed * 0.1f);
			ft.fallSpeed += (std::sin(ft.fallAngle) * 0.04f);

			if (ft.fallAngle > 1.0f) {
				// remove the tree after it has fallen 180 degrees; removing
				// after 90 degrees (angle > 0.5) would look silly for trees
				// placed on inclines
				v[n] = v.back();
				v.pop_back();
				continue;
			}

			const float ca = std::cos(ft.fallAngle * math::PI);
			const float sa = std::sin(ft.fallAngle * math::PI);

			const float3 yvec(ft.fallDir.x * sa, ca, ft.fallDir.z * sa);
			const float3 zvec((yvec.cross(-RgtVector)).ANormalize());
			const float3 xvec((yvec.cross(zvec)).ANormalize());

			ft.fallMat = CMatrix44f(ft.fallMat.GetPos(), xvec, yvec, zvec);

			n += 1;
		}
	}
}



void CAdvTreeDrawer::DrawTree(const TreeStruct& ts, int treeMatIdx)
{
	const CFeature* f = featureHandler.GetFeature(ts.id);

	if (f == nullptr)
		return;
	if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
		return;

	#if 0
	// assume all trees in a draw-quad are visible
	if (!cam->InView(pos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
		return;
	#endif

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(treeMatIdx, false, ts.mat);

	// done by DrawQuad
	// BindTreeGeometry(ts.type);
	DrawTreeGeometry(ts.type);
}

// only called from ITreeDrawer; no in-view test
void CAdvTreeDrawer::DrawTree(const float3& pos, int treeType, int treeMatIdx)
{
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(treeMatIdx, false, CMatrix44f(pos));

	BindTreeGeometry(treeType);
	DrawTreeGeometry(treeType);
}


void CAdvTreeDrawer::BindTreeGeometry(int treeType) const { treeGen.BindTreeBuffer(treeType); }
void CAdvTreeDrawer::DrawTreeGeometry(int treeType) const { treeGen.DrawTreeBuffer(treeType); }




void CAdvTreeDrawer::SetupDrawState() { SetupDrawState(CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER), treeShaders[shadowHandler.ShadowsLoaded()]); }
void CAdvTreeDrawer::SetupDrawState(const CCamera* cam, Shader::IProgramObject* ipo)
{
	treeShaders[TREE_PROGRAM_ACTIVE] = ipo;
	treeShaders[TREE_PROGRAM_ACTIVE]->Enable();

	if (shadowHandler.ShadowsLoaded()) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, treeGen.GetBarkTex());

		shadowHandler.SetupShadowTexSampler(GL_TEXTURE0);

		treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(10, false, shadowHandler.GetShadowViewMatrixRaw());
		treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform4fv(11, shadowHandler.GetShadowParams());
	} else {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, treeGen.GetBarkTex());
	}


	const float3& cameraDirX = cam->GetRight();
	const float3& cameraDirY = cam->GetUp();
	const float3   fogParams = {sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist()};

	const CMatrix44f& treeMat = CMatrix44f::Identity();
	const CMatrix44f& viewMat = camera->GetViewMatrix();
	const CMatrix44f& projMat = camera->GetProjectionMatrix();

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(0, &cameraDirX.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(1, &cameraDirY.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(6, &fogParams.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform4fv(2, float4{0.5f, 1.0f, 0.0f, 0.0f}); // test > 0.5
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform1f(13, globalRendering->gammaExponent);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(TREE_MAT_IDX, false, &treeMat.m[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(VIEW_MAT_IDX, false, &viewMat.m[0]);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(PROJ_MAT_IDX, false, &projMat.m[0]);


	glAttribStatePtr->EnableDepthMask();
	glAttribStatePtr->DisableBlendMask();
}

void CAdvTreeDrawer::ResetDrawState()
{
	glBindVertexArray(0);

	if (shadowHandler.ShadowsLoaded()) {
		// barkTex
		// glActiveTexture(GL_TEXTURE1);

		shadowHandler.ResetShadowTexSampler(GL_TEXTURE0);
	} else {
		glActiveTexture(GL_TEXTURE0);
	}

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform4fv(2, float4{0.0f, 0.0f, 0.0f, 1.0f}); // no test
	treeShaders[TREE_PROGRAM_ACTIVE]->Disable();
	treeShaders[TREE_PROGRAM_ACTIVE] = nullptr;
}


void CAdvTreeDrawer::SetupShadowDrawState() { SetupShadowDrawState(CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW), shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE)); }
void CAdvTreeDrawer::SetupShadowDrawState(const CCamera* cam, Shader::IProgramObject* ipo)
{
	glAttribStatePtr->DisableCullFace();

	glAttribStatePtr->PolygonOffset(1.0f, 1.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, treeGen.GetBarkTex()); // alpha-mask; threshold set in ShadowHandler


	const float3& cameraDirX = cam->GetRight();
	const float3& cameraDirY = cam->GetUp();

	treeShaders[TREE_PROGRAM_ACTIVE] = ipo;
	treeShaders[TREE_PROGRAM_ACTIVE]->Enable();
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(4, &cameraDirX.x);
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniform3fv(5, &cameraDirY.x);

	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(1, false, shadowHandler.GetShadowViewMatrix());
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(2, false, shadowHandler.GetShadowProjMatrix());
	treeShaders[TREE_PROGRAM_ACTIVE]->SetUniformMatrix4fv(3, false, CMatrix44f::Identity());
}

void CAdvTreeDrawer::ResetShadowDrawState()
{
	glBindVertexArray(0);

	treeShaders[TREE_PROGRAM_ACTIVE]->Disable();
	treeShaders[TREE_PROGRAM_ACTIVE] = nullptr;

	glAttribStatePtr->EnableCullFace();
	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);
}




void CAdvTreeDrawer::DrawTrees(const CCamera* cam, Shader::IProgramObject* ipo)
{
	constexpr int sqrWorldSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const     int matUniformIdx = mix(TREE_MAT_IDX, 3, shadowHandler.InShadowPass());

	for (const int2 idx: squareDrawer.inViewQuads) {
		const float3 camPos  = cam->GetPos();
		const float2 midPos = {(idx.x + 0.5f) * sqrWorldSize, (idx.y + 0.5f) * sqrWorldSize};
		const float3 sqrPos = {midPos.x, CGround::GetHeightReal(midPos.x, midPos.y, false), midPos.y};

		// soft cutoff (gradual density reduction)
		const float drawProb = std::min(1.0f, Square(GetDrawDistance()) / sqrPos.SqDistance(camPos));

		if (drawProb > 0.001f) {
			rng.SetSeed(rng.GetInitSeed());

			for (int i = 0; i < 2; i++) {
				const auto& treeSquare = treeSquares[(idx.y * NumTreesX()) + idx.x];
				const auto& treeStructs = treeSquare.trees[i];

				if (treeStructs.empty())
					continue;

				// bind once per draw-quad per tree-type (bush, pine) batch
				BindTreeGeometry(i * NUM_TREE_TYPES);

				for (const ITreeDrawer::TreeStruct& ts: treeStructs) {
					if (rng.NextFloat() > drawProb)
						continue;

					DrawTree(ts, matUniformIdx);
				}
			}
		}
	}
}

void CAdvTreeDrawer::DrawFallingTrees(const CCamera* cam, Shader::IProgramObject* ipo) const
{
	const int matUniformIdx = mix(TREE_MAT_IDX, 3, shadowHandler.InShadowPass());

	// draw trees that have been marked as falling
	for (int i = 0; i < 2; i++) {
		BindTreeGeometry(i * NUM_TREE_TYPES);

		for (const FallingTree& ft: fallingTrees[i]) {
			// const CFeature* f = featureHandler.GetFeature(ft.id);
			const float3 fpos = ft.fallMat.GetPos() - (UpVector * ft.fallAngle * 20.0f);

			// featureID is invalid for falling trees
			// if (!f->IsInLosForAllyTeam(gu->myAllyTeam))
			//   continue;
			if (!losHandler->InLos(fpos, gu->myAllyTeam))
				continue;
			if (!cam->InView(fpos + (UpVector * (MAX_TREE_HEIGHT * 0.5f)), MAX_TREE_HEIGHT * 0.5f))
				continue;

			ipo->SetUniformMatrix4fv(matUniformIdx, false, ft.fallMat);
			DrawTreeGeometry(ft.type);
		}
	}
}

void CAdvTreeDrawer::DrawPass()
{
	// trees are never drawn in any special (non-opaque) pass
	CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	Shader::IProgramObject* ipo = treeShaders[shadowHandler.ShadowsLoaded()];

	SetupDrawState(cam, ipo);
	DrawTrees(cam, ipo);
	DrawFallingTrees(cam, ipo);
	ResetDrawState();
}



void CAdvTreeDrawer::DrawShadowPass()
{
	CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW);
	Shader::IProgramObject* ipo = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_TREE);

	SetupShadowDrawState(cam, ipo);
	#if 0
	DrawTrees(cam, ipo);
	DrawFallingTrees(cam, ipo);
	#else
	// note: use player camera here s.t. all trees it can see are shadowed
	DrawTrees(CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER), ipo);
	DrawFallingTrees(CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER), ipo);
	#endif
	ResetShadowDrawState();
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
	ft.fallMat = CMatrix44f(pos);
	ft.fallDir = dir / len;
	ft.fallSpeed = std::max(0.01f, len * 0.0004f);
	ft.fallAngle = 0.0f;
}

