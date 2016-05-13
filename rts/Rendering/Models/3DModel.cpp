/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"


#include "3DOParser.h"
#include "S3OParser.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/Exceptions.h"
#include "System/Util.h"

#include <algorithm>
#include <cctype>

CR_BIND(LocalModelPiece, (NULL))
CR_REG_METADATA(LocalModelPiece, (
	CR_MEMBER(pos),
	CR_MEMBER(rot),
	CR_MEMBER(dir),
	CR_MEMBER(colvol),
	CR_MEMBER(scriptSetVisible),
	CR_MEMBER(lmodelPieceIndex),
	CR_MEMBER(scriptPieceIndex),
	CR_MEMBER(parent),
	CR_MEMBER(children),

	// reload
	CR_IGNORED(dispListID),
	CR_IGNORED(original),

	CR_IGNORED(dirty),
	CR_IGNORED(modelSpaceMat),
	CR_IGNORED(pieceSpaceMat),

	CR_IGNORED(lodDispLists) //FIXME GL idx!
))

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_IGNORED(lodCount), //FIXME?
	CR_MEMBER(pieces),

	CR_IGNORED(boundingVolume),
	CR_IGNORED(luaMaterialData)
))


/** ****************************************************************************************************
 * S3DModel
 */
void S3DModel::DeletePieces(S3DModelPiece* piece)
{
	for (unsigned int n = 0; n < piece->GetChildCount(); n++) {
		DeletePieces(piece->GetChild(n));
	}

	delete piece;
}



/** ****************************************************************************************************
 * S3DModelPiece
 */

S3DModelPiece::S3DModelPiece()
	: parent(NULL)
	, axisMapType(AXIS_MAPPING_XYZ)
	, scales(OnesVector)
	, mins(DEF_MIN_SIZE)
	, maxs(DEF_MAX_SIZE)
	, rotAxisSigns(-OnesVector)
	, hasIdentityRot(true)
	, dispListID(0)
{
}

S3DModelPiece::~S3DModelPiece()
{
	glDeleteLists(dispListID, 1);
}

void S3DModelPiece::CreateDispList()
{
	dispListID = glGenLists(1);
	glNewList(dispListID, GL_COMPILE);
	DrawForList();
	glEndList();
}

void S3DModelPiece::DrawStatic() const
{
	CMatrix44f mat;

	// get the static transform (sans script influences)
	ComposeTransform(mat, offset, ZeroVector, scales);

	glPushMatrix();
	glMultMatrixf(mat);

	if (HasGeometryData())
		glCallList(dispListID);

	for (unsigned int n = 0; n < children.size(); n++) {
		children[n]->DrawStatic();
	}

	glPopMatrix();
}


float3 S3DModelPiece::GetEmitPos() const
{
	switch (GetVertexCount()) {
		case 0:
		case 1: {
			return ZeroVector;
		} break;
		default: {
			return GetVertexPos(0);
		} break;
	}
}


float3 S3DModelPiece::GetEmitDir() const
{
	switch (GetVertexCount()) {
		case 0: {
			return FwdVector;
		} break;
		case 1: {
			return GetVertexPos(0);
		} break;
		default: {
			return GetVertexPos(1) - GetVertexPos(0);
		} break;
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
	const std::vector<unsigned>& indices = GetVertexIndices();

	// we just operate on a buffer
	// (cause the indices aren't needed once the buffer has been created)
	struct ShatterPartDataBuffer : S3DModelPiecePart::RenderData {
		std::vector<unsigned> indices;
	};
	std::vector<ShatterPartDataBuffer> shatterPartsBuf;

	// initialize splitter parts
	shatterPartsBuf.resize(S3DModelPiecePart::SHATTER_MAX_PARTS);
	for (auto& cp: shatterPartsBuf) {
		cp.dir = gu->RandVector().ANormalize();
	}

	// helper
	auto GetPolygonDir = [&](const size_t idx) -> float3
	{
		float3 midPos;
		for (int j: {0,1,2}) {
			midPos += GetVertexPos(indices[idx + j]);
		}
		midPos *= 0.333f;
		return midPos.ANormalize();
	};

	// add vertices to splitter parts
	for (size_t i = 0; i < indices.size(); i += 3) {
		const float3& dir = GetPolygonDir(i);

		// find the closest shatter part (the one that points into same dir)
		float md = -2.f;
		ShatterPartDataBuffer* mcp = nullptr;
		for (auto& cp: shatterPartsBuf) {
			if (cp.dir.dot(dir) < md)
				continue;

			md = cp.dir.dot(dir);
			mcp = &cp;
		}

		mcp->indices.push_back(indices[i + 0]);
		mcp->indices.push_back(indices[i + 1]);
		mcp->indices.push_back(indices[i + 2]);
	}

	// fill the vertex index vbo
	const auto isize = indices.size() * sizeof(unsigned int);
	auto* memVBO = reinterpret_cast<unsigned char*>(vboShatterIndices.MapBuffer(num * isize, isize, GL_WRITE_ONLY));
	size_t curVboPos = 0;
	for (auto& cp: shatterPartsBuf) {
		cp.indexCount = cp.indices.size();
		cp.vboOffset  = num * isize + curVboPos;
		if (cp.indexCount > 0) {
			memcpy(memVBO + curVboPos, &cp.indices[0], cp.indexCount * sizeof(unsigned int));
			curVboPos    += cp.indexCount * sizeof(unsigned int);
		}
	}
	vboShatterIndices.UnmapBuffer();

	// delete empty splitter parts
	for (int i = shatterPartsBuf.size() - 1; i >= 0; --i) {
		auto& cp = shatterPartsBuf[i];
		if (cp.indexCount == 0) {
			cp = std::move(shatterPartsBuf.back());
			shatterPartsBuf.pop_back();
		}
	}


	// finish: copy buffer to actual memory
	shatterParts[num].renderData.reserve(shatterPartsBuf.size());
	for (const auto& cp: shatterPartsBuf) {
		shatterParts[num].renderData.push_back(cp);
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

void LocalModel::DrawPiecesLOD(unsigned int lod) const
{
	if (lod > lodCount)
		return;

	for (const auto& p: pieces) {
		p.DrawLOD(lod);
	}
}

void LocalModel::SetLODCount(unsigned int count)
{
	assert(Initialized());
	lodCount = count;

	luaMaterialData.SetLODCount(lodCount);
	pieces[0].SetLODCount(lodCount);
}


void LocalModel::SetOriginalPieces(const S3DModelPiece* mp, int& idx)
{
	pieces[idx].original = mp;
	pieces[idx++].dispListID = mp->GetDisplayListID();

	for (unsigned int i = 0; i < mp->GetChildCount(); i++) {
		SetOriginalPieces(mp->GetChild(i), idx);
	}
}

void LocalModel::SetModel(const S3DModel* model, bool initialize)
{
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	// Only update the pieces
	if (!initialize) {
		assert(pieces.size() == model->numPieces);
		int idx = 0;
		SetOriginalPieces(model->GetRootPiece(), idx);
		assert (idx == model->numPieces);
		pieces[0].UpdateMatricesRec(true);
		UpdateBoundingVolume();
		return;
	}

	assert(pieces.size() == 0);

	lodCount = 0;

	pieces.reserve(model->numPieces);

	CreateLocalModelPieces(model->GetRootPiece());

	// must recursively update matrices here too: for features
	// LocalModel::Update is never called, but they might have
	// baked piece rotations (if .dae)
	pieces[0].UpdateMatricesRec(false);
	UpdateBoundingVolume();

	assert(pieces.size() == model->numPieces);
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	LocalModelPiece* lmpChild = NULL;

	// construct an LMP(mp) in-place
	pieces.emplace_back(mpParent);
	LocalModelPiece* lmpParent = &pieces.back();

	lmpParent->SetLModelPieceIndex(pieces.size() - 1);
	lmpParent->SetScriptPieceIndex(pieces.size() - 1);

	// the mapping is 1:1 for Lua scripts, but not necessarily for COB
	// CobInstance::MapScriptToModelPieces does the remapping (if any)
	assert(lmpParent->GetLModelPieceIndex() == lmpParent->GetScriptPieceIndex());

	for (unsigned int i = 0; i < mpParent->GetChildCount(); i++) {
		lmpChild = CreateLocalModelPieces(mpParent->GetChild(i));
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

	, lmodelPieceIndex(-1)
	, scriptPieceIndex(-1)

	, original(piece)
	, parent(NULL) // set later
{
	assert(piece != NULL);

	pos = piece->offset;
	dir = piece->GetEmitDir();

	UpdateMatrix();
	dispListID = piece->GetDisplayListID();

	children.reserve(piece->children.size());
}

void LocalModelPiece::SetDirty() {
	dirty = true;
	for (LocalModelPiece* child: children) {
		if (!child->dirty)
			child->SetDirty();
	}
}

void LocalModelPiece::UpdateMatrix() const
{
	original->ComposeTransform(pieceSpaceMat.LoadIdentity(), pos, rot, original->scales);
}

void LocalModelPiece::UpdateMatricesRec(bool updateChildMatrices) const
{
	if (dirty) {
		dirty = false;
		UpdateMatrix();
		updateChildMatrices = true;
	}

	if (updateChildMatrices) {
		modelSpaceMat = pieceSpaceMat;

		if (parent != NULL) {
			modelSpaceMat >>= parent->modelSpaceMat;
		}
	}

	for (unsigned int i = 0; i < children.size(); i++) {
		children[i]->UpdateMatricesRec(updateChildMatrices);
	}
}

void LocalModelPiece::UpdateParentMatricesRec() const
{
	if (parent != nullptr && parent->dirty)
		parent->UpdateParentMatricesRec();

	dirty = false;
	UpdateMatrix();
	modelSpaceMat = pieceSpaceMat;
	if (parent != nullptr)
		modelSpaceMat >>= parent->modelSpaceMat;

	return;
}


void LocalModelPiece::Draw() const
{
	if (!scriptSetVisible)
		return;

	glPushMatrix();
	glMultMatrixf(GetModelSpaceMatrix());
	glCallList(dispListID);
	glPopMatrix();
}

void LocalModelPiece::DrawLOD(unsigned int lod) const
{
	if (!scriptSetVisible)
		return;

	glPushMatrix();
	glMultMatrixf(GetModelSpaceMatrix());
	glCallList(lodDispLists[lod]);
	glPopMatrix();
}



void LocalModelPiece::SetLODCount(unsigned int count)
{
	// any new LOD's get null-lists first
	lodDispLists.resize(count, 0);

	for (unsigned int i = 0; i < children.size(); i++) {
		children[i]->SetLODCount(count);
	}
}


bool LocalModelPiece::GetEmitDirPos(float3& emitPos, float3& emitDir) const
{
	if (original == nullptr)
		return false;

	emitPos = GetModelSpaceMatrix() * original->GetEmitPos();
	emitDir = GetModelSpaceMatrix() * float4(original->GetEmitDir(), 0.f);

	// note: actually OBJECT_TO_WORLD but transform is the same
	emitPos *= WORLD_TO_OBJECT_SPACE;
	emitDir *= WORLD_TO_OBJECT_SPACE;
	return true;
}

/******************************************************************************/
/******************************************************************************/
