/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ITreeDrawer.h"
#include "AdvTreeDrawer.h"
#include "NullTreeDrawer.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"

CONFIG(int, TreeRadius)
	.defaultValue(1250) // elmos before density reduction
	.headlessValue(0)
	.minimumValue(0);

ITreeDrawer* treeDrawer = nullptr;



ITreeDrawer* ITreeDrawer::GetTreeDrawer()
{
	ITreeDrawer* td = nullptr;

	try {
		td = new CAdvTreeDrawer();
	} catch (const content_error& e) {
		LOG_L(L_ERROR, "[ITreeDrawer] exception \"%s\"; falling back to NullTreeDrawer", e.what());
		td = new CNullTreeDrawer();
	}

	td->AddTrees();
	return td;
}


ITreeDrawer::ITreeDrawer(): CEventClient("[ITreeDrawer]", 314444, false)
	, defDrawTrees(true)
	, luaDrawTrees(false)
	, wireFrameMode(false)
{
	eventHandler.AddClient(this);

	baseTreeDistance = configHandler->GetInt("TreeRadius");
	drawTreeDistance = Clamp(baseTreeDistance, 1.0f, CGlobalRendering::MAX_VIEW_RANGE);

	treesX = mapDims.mapx / TREE_SQUARE_SIZE;
	treesY = mapDims.mapy / TREE_SQUARE_SIZE;
	nTrees = treesX * treesY;
}

ITreeDrawer::~ITreeDrawer() {
	eventHandler.RemoveClient(this);
	configHandler->Set("TreeRadius", int(baseTreeDistance));
}


float ITreeDrawer::IncrDrawDistance() {
	return (drawTreeDistance = Clamp(baseTreeDistance *= 1.25f, 1.0f, CGlobalRendering::MAX_VIEW_RANGE));
}

float ITreeDrawer::DecrDrawDistance() {
	return (drawTreeDistance = Clamp(baseTreeDistance *= 0.8f, 1.0f, CGlobalRendering::MAX_VIEW_RANGE));
}



void ITreeDrawer::AddTrees()
{
	for (const int featureID: featureHandler->GetActiveFeatureIDs()) {
		const CFeature* f = featureHandler->GetFeature(featureID);
		const FeatureDef* fd = f->def;

		if (fd->drawType < DRAWTYPE_TREE)
			continue;

		AddTree(f->id, fd->drawType - DRAWTYPE_TREE, f->pos, 1.0f);
	}
}


void ITreeDrawer::AddTree(int treeID, int treeType, const float3& pos, float size)
{
	if (treeSquares.empty())
		return; // NullDrawer

	TreeStruct ts;
	ts.id = treeID;
	ts.type = treeType;
	ts.pos = pos;

	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x) / (treeSquareSize)) +
		(((int)pos.z) / (treeSquareSize) * treesX);

	spring::VectorInsertUnique(treeSquares[treeSquareIdx].trees, ts, true);
	ResetPos(pos);
}

void ITreeDrawer::DeleteTree(int treeID, const float3& pos)
{
	if (treeSquares.empty())
		return; // NullDrawer

	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x / (treeSquareSize))) +
		(((int)pos.z / (treeSquareSize) * treesX));

	spring::VectorEraseIf(treeSquares[treeSquareIdx].trees, [treeID](const TreeStruct& ts) { return (treeID == ts.id); });
	ResetPos(pos);
}



void ITreeDrawer::RenderFeatureCreated(const CFeature* feature) {
	const FeatureDef* fd = feature->def;

	// support /give'ing tree objects
	if (fd->drawType < DRAWTYPE_TREE)
		return;

	AddTree(feature->id, fd->drawType - DRAWTYPE_TREE, feature->pos, 1.0f);
}

void ITreeDrawer::FeatureMoved(const CFeature* feature, const float3& oldpos) {
	const FeatureDef* fd = feature->def;

	if (fd->drawType < DRAWTYPE_TREE)
		return;

	DeleteTree(feature->id, oldpos);
	AddTree(feature->id, fd->drawType - DRAWTYPE_TREE, feature->pos, 1.0f);
}

void ITreeDrawer::RenderFeatureDestroyed(const CFeature* feature) {
	const FeatureDef* fd = feature->def;

	if (fd->drawType < DRAWTYPE_TREE)
		return;

	DeleteTree(feature->id, feature->pos);

	if (feature->speed.SqLength2D() <= 0.25f)
		return;

	AddFallingTree(feature->id, fd->drawType - DRAWTYPE_TREE, feature->pos, feature->speed * XZVector);
}



void ITreeDrawer::SetupState() const {
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.005f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// NOTE:
	//   the info-texture now contains an alpha-component
	//   so binding it here means trees will be invisible
	//   when shadows are disabled
	if (infoTextureHandler->IsEnabled()) {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);
		glActiveTexture(GL_TEXTURE0);
	}
}

void ITreeDrawer::ResetState() const {
	if (infoTextureHandler->IsEnabled()) {
		glActiveTexture(GL_TEXTURE1);

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}

	glPopAttrib();
}



void ITreeDrawer::Draw()
{
	if (luaDrawTrees) {
		// bypass engine rendering
		eventHandler.DrawTrees();
		return;
	}

	if (!defDrawTrees)
		return;

	SetupState();
	DrawPass();
	ResetState();
}

void ITreeDrawer::DrawShadow()
{
	if (luaDrawTrees) {
		eventHandler.DrawTrees();
		return;
	}

	if (!defDrawTrees)
		return;

	DrawShadowPass();
}


void ITreeDrawer::DrawTree(const CFeature* f, bool setupState, bool resetState)
{
	if (treeSquares.empty())
		return; // NullDrawer

	const FeatureDef* fd = f->def;
	CAdvTreeDrawer* atd = static_cast<CAdvTreeDrawer*>(this);

	assert(fd->drawType >= DRAWTYPE_TREE);

	if (setupState) {
		SetupState();
		atd->SetupDrawState();
	}

	// TODO: check if in shadow-pass
	atd->DrawTree(f->pos, fd->drawType, 2);

	if (resetState) {
		atd->ResetDrawState();
		ResetState();
	}
}

