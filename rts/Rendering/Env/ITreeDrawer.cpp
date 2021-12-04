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
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

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
	configHandler->NotifyOnChange(this, {"TreeRadius"});

	baseTreeDistance = configHandler->GetInt("TreeRadius");
	drawTreeDistance = Clamp(baseTreeDistance, 1.0f, CGlobalRendering::MAX_VIEW_RANGE);

	treesX = mapDims.mapx / TREE_SQUARE_SIZE;
	treesY = mapDims.mapy / TREE_SQUARE_SIZE;
	nTrees = treesX * treesY;
}

ITreeDrawer::~ITreeDrawer() {
	configHandler->RemoveObserver(this);
	configHandler->Set("TreeRadius", int(baseTreeDistance));

	eventHandler.RemoveClient(this);
}


float ITreeDrawer::IncrDrawDistance() { return (drawTreeDistance = Clamp(baseTreeDistance *= 1.25f, 1.0f, CGlobalRendering::MAX_VIEW_RANGE)); }
float ITreeDrawer::DecrDrawDistance() { return (drawTreeDistance = Clamp(baseTreeDistance *= 0.80f, 1.0f, CGlobalRendering::MAX_VIEW_RANGE)); }

void ITreeDrawer::ConfigNotify(const std::string& key, const std::string& value)
{
	baseTreeDistance = float(std::max(0, std::atoi(value.c_str())));
	drawTreeDistance = Clamp(baseTreeDistance, 1.0f, CGlobalRendering::MAX_VIEW_RANGE);
}


void ITreeDrawer::AddTrees()
{
	for (const int featureID: featureHandler.GetActiveFeatureIDs()) {
		const CFeature* f = featureHandler.GetFeature(featureID);
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

	CFeature* f = featureHandler.GetFeature(treeID);

	{
		// FeatureDrawer does not take care of this for trees, update the
		// draw-positions here since FeatureCreated and FeatureMoved both
		// call us
		f->drawPos = f->pos;
		f->drawMidPos = f->midPos;

		f->UpdateTransform(f->drawPos, false);
	}

	TreeStruct ts;
	ts.id = treeID;
	ts.type = treeType;
	ts.mat = f->GetTransformMatrix(false);

	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x) / (treeSquareSize)) +
		(((int)pos.z) / (treeSquareSize) * treesX);

	spring::VectorInsertUnique(treeSquares[treeSquareIdx].trees[treeType >= NUM_TREE_TYPES], ts, true);
}

void ITreeDrawer::DeleteTree(int treeID, int treeType, const float3& pos)
{
	if (treeSquares.empty())
		return; // NullDrawer

	const int treeSquareSize = SQUARE_SIZE * TREE_SQUARE_SIZE;
	const int treeSquareIdx =
		(((int)pos.x / (treeSquareSize))) +
		(((int)pos.z / (treeSquareSize) * treesX));

	spring::VectorEraseIf(treeSquares[treeSquareIdx].trees[treeType >= NUM_TREE_TYPES], [treeID](const TreeStruct& ts) { return (treeID == ts.id); });
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

	DeleteTree(feature->id, fd->drawType - DRAWTYPE_TREE, oldpos);
	AddTree(feature->id, fd->drawType - DRAWTYPE_TREE, feature->pos, 1.0f);
}

void ITreeDrawer::RenderFeatureDestroyed(const CFeature* feature) {
	const FeatureDef* fd = feature->def;

	if (fd->drawType < DRAWTYPE_TREE)
		return;

	DeleteTree(feature->id, fd->drawType - DRAWTYPE_TREE, feature->pos);

	if (feature->speed.SqLength2D() <= 0.25f)
		return;

	AddFallingTree(feature->id, fd->drawType - DRAWTYPE_TREE, feature->pos, feature->speed * XZVector);
}



void ITreeDrawer::SetupState() const {
	glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ITreeDrawer::ResetState() const {
	glAttribStatePtr->PopBits();
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
	atd->DrawTree(f->pos, fd->drawType, CAdvTreeDrawer::TREE_MAT_IDX);

	if (resetState) {
		atd->ResetDrawState();
		ResetState();
	}
}

