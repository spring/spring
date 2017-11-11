/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"

#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"

#include <algorithm>
#include <cctype>
#include <cstring>

CR_BIND(LocalModelPiece, (nullptr))
CR_REG_METADATA(LocalModelPiece, (
	CR_MEMBER(pos),
	CR_MEMBER(rot),
	CR_MEMBER(dir),
	CR_MEMBER(colvol),
	CR_MEMBER(scriptSetVisible),
	CR_MEMBER(blockScriptAnims),
	CR_MEMBER(lmodelPieceIndex),
	CR_MEMBER(scriptPieceIndex),
	CR_MEMBER(parent),
	CR_MEMBER(children),

	// reload
	CR_IGNORED(dispListID),
	CR_IGNORED(original),

	CR_IGNORED(dirty),
	CR_IGNORED(modelSpaceMat),
	CR_IGNORED(pieceSpaceMat)
))

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_MEMBER(pieces),

	CR_IGNORED(boundingVolume),
	CR_IGNORED(luaMaterialData)
))


/** ****************************************************************************************************
 * S3DModel
 */
void S3DModel::DeletePieces()
{
	assert(!pieces.empty());

	for (size_t n = 0; n < pieces.size(); n++) {
		spring::SafeDelete(pieces[n]);
	}

	pieces.clear();
}



/** ****************************************************************************************************
 * S3DModelPiece
 */

S3DModelPiece::~S3DModelPiece()
{
	glDeleteLists(dispListID, 1);
}

void S3DModelPiece::CreateDispList()
{
	glNewList(dispListID = glGenLists(1), GL_COMPILE);
	DrawForList();
	glEndList();
}

void S3DModelPiece::DrawStatic() const
{
	if (!HasGeometryData())
		return;

	GL::PushMatrix();
	GL::MultMatrix(pieceMatrix);
	glCallList(dispListID);
	GL::PopMatrix();
}


float3 S3DModelPiece::GetEmitPos() const
{
	switch (GetVertexCount()) {
		case 0:
		case 1: { return ZeroVector; } break;
		default: { return GetVertexPos(0); } break;
	}
}

float3 S3DModelPiece::GetEmitDir() const
{
	switch (GetVertexCount()) {
		case 0: { return FwdVector; } break;
		case 1: { return GetVertexPos(0); } break;
		default: { return (GetVertexPos(1) - GetVertexPos(0)); } break;
	}
}


void S3DModelPiece::CreateShatterPieces()
{
	if (!HasGeometryData())
		return;

	vboShatterIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	vboShatterIndices.Resize(S3DModelPiecePart::SHATTER_VARIATIONS * GetVertexIndices().size() * sizeof(unsigned int));

	for (int i = 0; i < S3DModelPiecePart::SHATTER_VARIATIONS; ++i) {
		CreateShatterPiecesVariation(i);
	}

	vboShatterIndices.Unbind();
}


void S3DModelPiece::CreateShatterPiecesVariation(const int num)
{
	typedef  std::pair<S3DModelPiecePart::RenderData, std::vector<unsigned> >  ShatterPartDataPair;
	typedef  std::array< ShatterPartDataPair, S3DModelPiecePart::SHATTER_MAX_PARTS>  ShatterPartsBuffer;

	// operate on a buffer; indices are not needed once VBO has been created
	ShatterPartsBuffer shatterPartsBuf;

	for (ShatterPartDataPair& cp: shatterPartsBuf) {
		cp.first.dir = (guRNG.NextVector()).ANormalize();
	}

	// helper
	const std::vector<unsigned>& indices = GetVertexIndices();
	const auto GetPolygonDir = [&](const size_t idx) -> float3
	{
		float3 midPos;
		midPos += GetVertexPos(indices[idx + 0]);
		midPos += GetVertexPos(indices[idx + 1]);
		midPos += GetVertexPos(indices[idx + 2]);
		midPos *= 0.333f;
		return (midPos.ANormalize());
	};

	// add vertices to splitter parts
	for (size_t i = 0; i < indices.size(); i += 3) {
		const float3& dir = GetPolygonDir(i);

		// find the closest shatter part (the one that points into same dir)
		float md = -2.0f;

		ShatterPartDataPair* mcp = nullptr;
		S3DModelPiecePart::RenderData* rd = nullptr;

		for (ShatterPartDataPair& cp: shatterPartsBuf) {
			rd = &cp.first;

			if (rd->dir.dot(dir) < md)
				continue;

			md = rd->dir.dot(dir);
			mcp = &cp;
		}

		(mcp->second).push_back(indices[i + 0]);
		(mcp->second).push_back(indices[i + 1]);
		(mcp->second).push_back(indices[i + 2]);
	}

	{
		// fill the vertex index vbo
		const size_t isize = indices.size() * sizeof(unsigned int);
		size_t vboPos = 0;

		auto* vboMem = reinterpret_cast<unsigned char*>(vboShatterIndices.MapBuffer(num * isize, isize, GL_WRITE_ONLY));

		for (ShatterPartDataPair& cp: shatterPartsBuf) {
			S3DModelPiecePart::RenderData& rd = cp.first;

			rd.indexCount = (cp.second).size();
			rd.vboOffset  = num * isize + vboPos;

			if (rd.indexCount > 0) {
				memcpy(vboMem + vboPos, &(cp.second)[0], rd.indexCount * sizeof(unsigned int));
				vboPos += (rd.indexCount * sizeof(unsigned int));
			}
		}

		vboShatterIndices.UnmapBuffer();
	}

	{
		// delete empty splitter parts
		size_t backIdx = shatterPartsBuf.size() - 1;

		for (size_t j = 0; j < shatterPartsBuf.size() && j < backIdx; ) {
			const ShatterPartDataPair& cp = shatterPartsBuf[j];
			const S3DModelPiecePart::RenderData& rd = cp.first;

			if (rd.indexCount == 0) {
				std::swap(shatterPartsBuf[j], shatterPartsBuf[backIdx--]);
				continue;
			}

			j++;
		}

		shatterParts[num].renderData.clear();
		shatterParts[num].renderData.reserve(backIdx + 1);

		// finish: copy buffer to actual memory
		for (size_t n = 0; n <= backIdx; n++) {
			shatterParts[num].renderData.push_back(shatterPartsBuf[n].first);
		}
	}
}


void S3DModelPiece::Shatter(float pieceChance, int modelType, int texType, int team, const float3 pos, const float3 speed, const CMatrix44f& m) const
{
	const float2  pieceParams = {float3::max(float3::fabs(maxs), float3::fabs(mins)).Length(), pieceChance};
	const   int2 renderParams = {texType, team};

	projectileHandler->AddFlyingPiece(modelType, this, m, pos, speed, pieceParams, renderParams);
}



/** ****************************************************************************************************
 * LocalModel
 */

void LocalModel::DrawPieces() const
{
	for (const auto& p: pieces) {
		p.Draw();
	}
}


void LocalModel::SetModel(const S3DModel* model, bool initialize)
{
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	if (!initialize) {
		assert(pieces.size() == model->numPieces);

		// PostLoad; only update the pieces
		for (size_t n = 0; n < pieces.size(); n++) {
			S3DModelPiece* omp = model->GetPiece(n);

			pieces[n].original = omp;
			pieces[n].dispListID = omp->GetDisplayListID();
		}

		pieces[0].UpdateChildMatricesRec(true);
		UpdateBoundingVolume();
		return;
	}

	assert(pieces.empty());

	pieces.clear();
	pieces.reserve(model->numPieces);

	CreateLocalModelPieces(model->GetRootPiece());

	// must recursively update matrices here too: for features
	// LocalModel::Update is never called, but they might have
	// baked piece rotations (in the case of .dae)
	pieces[0].UpdateChildMatricesRec(false);
	UpdateBoundingVolume();

	assert(pieces.size() == model->numPieces);
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	LocalModelPiece* lmpChild = nullptr;

	// construct an LMP(mp) in-place
	pieces.emplace_back(mpParent);
	LocalModelPiece* lmpParent = &pieces.back();

	lmpParent->SetLModelPieceIndex(pieces.size() - 1);
	lmpParent->SetScriptPieceIndex(pieces.size() - 1);

	// the mapping is 1:1 for Lua scripts, but not necessarily for COB
	// CobInstance::MapScriptToModelPieces does the remapping (if any)
	assert(lmpParent->GetLModelPieceIndex() == lmpParent->GetScriptPieceIndex());

	for (const S3DModelPiece* mpChild: mpParent->children) {
		lmpChild = CreateLocalModelPieces(mpChild);
		lmpChild->SetParent(lmpParent);
		lmpParent->AddChild(lmpChild);
	}

	return lmpParent;
}


void LocalModel::UpdateBoundingVolume()
{
	// bounding-box extrema (local space)
	float3 bbMins = DEF_MIN_SIZE;
	float3 bbMaxs = DEF_MAX_SIZE;

	for (unsigned int n = 0; n < pieces.size(); n++) {
		const CMatrix44f& matrix = pieces[n].GetModelSpaceMatrix();
		const S3DModelPiece* piece = pieces[n].original;

		// skip empty pieces or bounds will not be sensible
		if (!piece->HasGeometryData())
			continue;

		#if 0
		const unsigned int vcount = piece->GetVertexCount();

		if (vcount >= 8) {
		#endif
			// transform only the corners of the piece's bounding-box
			const float3 pMins = piece->mins;
			const float3 pMaxs = piece->maxs;
			const float3 verts[8] = {
				// bottom
				float3(pMins.x,  pMins.y,  pMins.z),
				float3(pMaxs.x,  pMins.y,  pMins.z),
				float3(pMaxs.x,  pMins.y,  pMaxs.z),
				float3(pMins.x,  pMins.y,  pMaxs.z),
				// top
				float3(pMins.x,  pMaxs.y,  pMins.z),
				float3(pMaxs.x,  pMaxs.y,  pMins.z),
				float3(pMaxs.x,  pMaxs.y,  pMaxs.z),
				float3(pMins.x,  pMaxs.y,  pMaxs.z),
			};

			for (unsigned int k = 0; k < 8; k++) {
				const float3 vertex = matrix * verts[k];

				bbMins = float3::min(bbMins, vertex);
				bbMaxs = float3::max(bbMaxs, vertex);
			}
		#if 0
		} else {
			// note: not as efficient because of branching and virtual calls
			for (unsigned int k = 0; k < vcount; k++) {
				const float3 vertex = matrix * piece->GetVertexPos(k);

				bbMins = float3::min(bbMins, vertex);
				bbMaxs = float3::max(bbMaxs, vertex);
			}
		}
		#endif
	}

	// note: offset is relative to object->pos
	boundingVolume.InitBox(bbMaxs - bbMins, (bbMaxs + bbMins) * 0.5f);
}



/** ****************************************************************************************************
 * LocalModelPiece
 */

LocalModelPiece::LocalModelPiece(const S3DModelPiece* piece)
	: colvol(piece->GetCollisionVolume())

	, dirty(true)

	, scriptSetVisible(piece->HasGeometryData())
	, blockScriptAnims(false)

	, lmodelPieceIndex(-1)
	, scriptPieceIndex(-1)

	, original(piece)
	, parent(nullptr) // set later
{
	assert(piece != nullptr);

	pos = piece->offset;
	dir = piece->GetEmitDir();

	pieceSpaceMat = std::move(CalcPieceSpaceMatrix(pos, rot, original->scales));
	dispListID = piece->GetDisplayListID();

	children.reserve(piece->children.size());
}

void LocalModelPiece::SetDirty() {
	dirty = true;

	for (LocalModelPiece* child: children) {
		if (child->dirty)
			continue;
		child->SetDirty();
	}
}

void LocalModelPiece::SetPosOrRot(const float3& src, float3& dst) {
	if (blockScriptAnims)
		return;
	if (!dirty && !dst.same(src))
		SetDirty();

	dst = src;
}


void LocalModelPiece::UpdateChildMatricesRec(bool updateChildMatrices) const
{
	if (dirty) {
		dirty = false;
		updateChildMatrices = true;

		pieceSpaceMat = std::move(CalcPieceSpaceMatrix(pos, rot, original->scales));
	}

	if (updateChildMatrices) {
		modelSpaceMat = pieceSpaceMat;

		if (parent != nullptr) {
			modelSpaceMat >>= parent->modelSpaceMat;
		}
	}

	for (unsigned int i = 0; i < children.size(); i++) {
		children[i]->UpdateChildMatricesRec(updateChildMatrices);
	}
}

void LocalModelPiece::UpdateParentMatricesRec() const
{
	if (parent != nullptr && parent->dirty)
		parent->UpdateParentMatricesRec();

	dirty = false;

	pieceSpaceMat = std::move(CalcPieceSpaceMatrix(pos, rot, original->scales));
	modelSpaceMat = pieceSpaceMat;

	if (parent != nullptr)
		modelSpaceMat >>= parent->modelSpaceMat;
}


void LocalModelPiece::Draw() const
{
	if (!scriptSetVisible)
		return;

	GL::PushMatrix();
	GL::MultMatrix(GetModelSpaceMatrix());
	glCallList(dispListID);
	GL::PopMatrix();
}


bool LocalModelPiece::GetEmitDirPos(float3& emitPos, float3& emitDir) const
{
	if (original == nullptr)
		return false;

	// note: actually OBJECT_TO_WORLD but transform is the same
	emitPos = GetModelSpaceMatrix() *        original->GetEmitPos()        * WORLD_TO_OBJECT_SPACE;
	emitDir = GetModelSpaceMatrix() * float4(original->GetEmitDir(), 0.0f) * WORLD_TO_OBJECT_SPACE;
	return true;
}

/******************************************************************************/
/******************************************************************************/
