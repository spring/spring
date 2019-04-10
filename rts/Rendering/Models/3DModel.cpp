/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"

#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>

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
	CR_IGNORED(original),

	CR_IGNORED(dirty),
	CR_IGNORED(modelSpaceMat),
	CR_IGNORED(pieceSpaceMat)
))

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_MEMBER(pieces),
	CR_IGNORED(matrices),

	CR_IGNORED(boundingVolume),
	CR_IGNORED(luaMaterialData),

	CR_IGNORED(pmuFrameNum),
	// reload
	CR_IGNORED(vertexArray),
	CR_IGNORED(elemsBuffer),
	CR_IGNORED(indcsBuffer),
	CR_IGNORED(vboNumVerts),
	CR_IGNORED(vboNumIndcs)
))


/** ****************************************************************************************************
 * S3DModel
 */
void S3DModel::DeleteBuffers()
{
	if (vertexArray != 0)
		glDeleteVertexArrays(1, &vertexArray);
	if (elemsBuffer != 0)
		glDeleteBuffers(1, &elemsBuffer);
	if (indcsBuffer != 0)
		glDeleteBuffers(1, &indcsBuffer);

	vertexArray = 0;
	elemsBuffer = 0;
	indcsBuffer = 0;
}

void S3DModel::UploadBuffers()
{
	{
		size_t numVerts = 0;
		size_t numIndcs = 0;

		for (S3DModelPiece* omp: pieceObjects) {
			if (!omp->HasGeometryData())
				continue;

			const std::vector<SVertexData>& elems = omp->GetVertexElements();
			const std::vector<unsigned int>& indcs = omp->GetVertexIndices();

			// cache the starting offsets per piece
			omp->vboStartElem = numVerts;
			omp->vboStartIndx = numIndcs;

			numVerts += elems.size();
			numIndcs += indcs.size();
		}

		for (S3DModelPiece* omp: pieceObjects) {
			omp->CreateShatterPieces();
		}

		vboNumVerts = numVerts;
		vboNumIndcs = numIndcs;
	}

	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);


	{
		// geometry
		glGenBuffers(1, &elemsBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, elemsBuffer);
		glBufferData(GL_ARRAY_BUFFER, vboNumVerts * sizeof(SVertexData), nullptr, GL_STATIC_DRAW);

		for (size_t i = 0, n = pieceObjects.size(); i < n; i++) {
			const S3DModelPiece* omp = pieceObjects[i];

			if (!omp->HasGeometryData())
				continue;

			std::vector<SVertexData> elems = omp->GetVertexElements();

			// pieceIndex is initialized by parsers in traversal order
			// (i.e. in general i != pieceIndex), but shaders want the
			// linear indices
			std::for_each(elems.begin(), elems.end(), [&i](SVertexData& e) { e.pieceIndex = i; });

			assert(!elems.empty());
			glBufferSubData(GL_ARRAY_BUFFER, omp->vboStartElem * sizeof(SVertexData), elems.size() * sizeof(SVertexData), elems.data());
		}
	}
	{
		// indices
		glGenBuffers(1, &indcsBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indcsBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, vboNumIndcs * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

		for (const S3DModelPiece* omp : pieceObjects) {

			if (!omp->HasGeometryData())
				continue;

			std::vector<unsigned int> indcs = omp->GetVertexIndices();

			// shift piece-relative indices; vertices of all pieces are packed together
			std::for_each(indcs.begin(), indcs.end(), [&omp](unsigned int& idx) { idx += omp->vboStartElem; });

			assert(!indcs.empty());
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, omp->vboStartIndx * sizeof(uint32_t), indcs.size() * sizeof(uint32_t), indcs.data());
		}
	}

	EnableAttribs();
	glBindVertexArray(0);
	DisableAttribs();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void S3DModel::EnableAttribs() const
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);

	#if 0
	glVertexAttribPointer (0,  3, GL_FLOAT       , false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, pos         ));
	glVertexAttribPointer (1,  3, GL_FLOAT       , false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, normal      ));
	glVertexAttribPointer (2,  3, GL_FLOAT       , false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, sTangent    ));
	glVertexAttribPointer (3,  3, GL_FLOAT       , false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, tTangent    ));
	glVertexAttribPointer (4,  2, GL_FLOAT       , false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, texCoords[0]));
	glVertexAttribPointer (5,  2, GL_FLOAT       , false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, texCoords[1]));
	glVertexAttribIPointer(6,  1, GL_UNSIGNED_INT, false,  sizeof(SVertexData), (const void*) offsetof(SVertexData, pieceIndex  ));
	#else
	glVertexAttribPointer (0,  3, GL_FLOAT       , false,  sizeof(SVertexData), VA_TYPE_OFFSET(   float,  0));
	glVertexAttribPointer (1,  3, GL_FLOAT       , false,  sizeof(SVertexData), VA_TYPE_OFFSET(   float,  3));
	glVertexAttribPointer (2,  3, GL_FLOAT       , false,  sizeof(SVertexData), VA_TYPE_OFFSET(   float,  6));
	glVertexAttribPointer (3,  3, GL_FLOAT       , false,  sizeof(SVertexData), VA_TYPE_OFFSET(   float,  9));
	glVertexAttribPointer (4,  2, GL_FLOAT       , false,  sizeof(SVertexData), VA_TYPE_OFFSET(   float, 12));
	glVertexAttribPointer (5,  2, GL_FLOAT       , false,  sizeof(SVertexData), VA_TYPE_OFFSET(   float, 14));
	glVertexAttribIPointer(6,  1, GL_UNSIGNED_INT,         sizeof(SVertexData), VA_TYPE_OFFSET(uint32_t, 16));
	#endif
}

void S3DModel::DisableAttribs() const
{
	glDisableVertexAttribArray(6);
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}


void S3DModel::Draw() const
{
	// draw pieces in their static bind-pose (ie. without script-transforms)
	// this now requires setting up bind-pose matrices via IUnitRenderState
	// TODO: should convert S3O's that do not use PRIMTYPE=TRIANGLES?
	glBindVertexArray(vertexArray);
	glDrawElements(GL_TRIANGLES, vboNumIndcs, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void S3DModel::DrawPiece(const S3DModelPiece* omp) const
{
	assert(std::find_if(pieceObjects.cbegin(), pieceObjects.cend(), [&](const S3DModelPiece* p) { return (p == omp); }) != pieceObjects.cend());

	const std::vector<unsigned int>& indcs = omp->GetVertexIndices();

	// draw the buffer sub-region corresponding to this piece
	glBindVertexArray(vertexArray);
	glDrawElements(GL_TRIANGLES, indcs.size(), GL_UNSIGNED_INT, VA_TYPE_OFFSET(uint32_t, omp->vboStartIndx));
	glBindVertexArray(0);
}

// only used by projectiles with the PF_Recursive flag
void S3DModel::DrawPieceRec(const S3DModelPiece* omp) const
{
	DrawPiece(omp);

	for (const S3DModelPiece* childPiece: omp->children) {
		DrawPiece(childPiece);
	}
}



void S3DModel::SetPieceMatrices() {
	pieceObjects[0]->SetBindPoseMatrix(CMatrix44f());
	pieceMatrices.resize(pieceObjects.size());

	for (size_t i = 0, n = pieceObjects.size(); i < n; i++) {
		pieceMatrices[i] = pieceObjects[i]->bposeMatrix;
	}
}

#if 0
void S3DModel::FlattenPieceTree(S3DModelPiece* root) {
	assert(root != nullptr);

	pieceObjects.clear();
	pieceObjects.reserve(numPieces);

	std::vector<S3DModelPiece*> stack = {root};

	while (!stack.empty()) {
		S3DModelPiece* p = stack.back();

		stack.pop_back();
		pieceObjects.push_back(p);

		// add children in reverse for the correct DF traversal order
		for (size_t n = 0; n < p->children.size(); n++) {
			stack.push_back(p->children[p->children.size() - n - 1]);
		}
	}
}
#endif

void S3DModel::FlattenPieceTree(S3DModelPiece* root) {
	assert(root != nullptr);

	pieceObjects.clear();
	pieceObjects.reserve(numPieces);

	FlattenPieceTreeRec(root);
}

void S3DModel::FlattenPieceTreeRec(S3DModelPiece* piece) {
	pieceObjects.push_back(piece);

	for (S3DModelPiece* childPiece: piece->children) {
		FlattenPieceTreeRec(childPiece);
	}
}


/** ****************************************************************************************************
 * S3DModelPiece
 */

float3 S3DModelPiece::GetEmitPos() const
{
	switch (GetVertexCount()) {
		case  0:
		case  1: { return   ZeroVector   ; } break;
		default: { return GetVertexPos(0); } break;
	}
}

float3 S3DModelPiece::GetEmitDir() const
{
	switch (GetVertexCount()) {
		case  0: { return (                     FwdVector   ); } break;
		case  1: { return (                  GetVertexPos(0)); } break;
		default: { return (GetVertexPos(1) - GetVertexPos(0)); } break;
	}
}


void S3DModelPiece::CreateShatterPieces()
{
	if (!HasGeometryData())
		return;

	shatterIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	shatterIndices.New(S3DModelPiecePart::SHATTER_VARIATIONS * GetVertexDrawIndexCount() * sizeof(unsigned int));
	// spams performance warnings ("Buffer object 123 (bound to GL_ELEMENT_ARRAY_BUFFER_ARB, usage hint is GL_STREAM_DRAW) is being copied/moved from VIDEO memory to HOST memory.")
	// shatterIndices.Resize(S3DModelPiecePart::SHATTER_VARIATIONS * GetVertexDrawIndexCount() * sizeof(unsigned int));

	for (int i = 0; i < S3DModelPiecePart::SHATTER_VARIATIONS; ++i) {
		CreateShatterPiece(i);
	}

	shatterIndices.Unbind();
}


void S3DModelPiece::CreateShatterPiece(int pieceNum)
{
	typedef  std::pair<S3DModelPiecePart::RenderData, std::vector<unsigned int> >  ShatterPartDataPair;
	typedef  std::array< ShatterPartDataPair, S3DModelPiecePart::SHATTER_MAX_PARTS>  ShatterPartsBuffer;

	// operate on a buffer; indices are not needed once VBO has been created
	ShatterPartsBuffer shatterPartsBuf;

	for (ShatterPartDataPair& cp: shatterPartsBuf) {
		cp.first.dir = (guRNG.NextVector()).ANormalize();
	}

	// helper
	const std::vector<unsigned>& indices = GetVertexIndices();
	const std::function<float3(size_t idx)> GetPolygonDir = [&](size_t idx)
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

		// find the closest-direction shatter part
		float minDirAngle = -2.0f;
		float curDirAngle =  0.0f;

		ShatterPartDataPair* nearestPart = nullptr;
		S3DModelPiecePart::RenderData* renderData = nullptr;

		for (ShatterPartDataPair& currentPart: shatterPartsBuf) {
			renderData = &currentPart.first;

			if ((curDirAngle = renderData->dir.dot(dir)) < minDirAngle)
				continue;

			minDirAngle = curDirAngle;
			nearestPart = &currentPart;
		}

		// NB:
		//   indices are relative to the piece, not the (packed) model-buffer
		//   FlyingPiece::Draw binds the model-buffer and piece shatter-indcs
		(nearestPart->second).push_back(indices[i + 0]  +  vboStartElem * 1);
		(nearestPart->second).push_back(indices[i + 1]  +  vboStartElem * 1);
		(nearestPart->second).push_back(indices[i + 2]  +  vboStartElem * 1);
	}

	{
		// fill the IBO sub-region for this variation
		const size_t numBytes = indices.size() * sizeof(unsigned int);
		      size_t vboIndex = 0;

		auto* idxBufMem = reinterpret_cast<unsigned char*>(shatterIndices.MapBuffer(pieceNum * numBytes, numBytes, GL_WRITE_ONLY));

		if (idxBufMem != nullptr) {
			for (ShatterPartDataPair& cp: shatterPartsBuf) {
				S3DModelPiecePart::RenderData& rdata = cp.first;
				const std::vector<unsigned int>& idcs = cp.second;

				rdata.indexCount = idcs.size();
				rdata.vboOffset  = pieceNum * numBytes + vboIndex;

				if (rdata.indexCount != 0) {
					memcpy(idxBufMem + vboIndex, idcs.data(), rdata.indexCount * sizeof(unsigned int));
					vboIndex += (rdata.indexCount * sizeof(unsigned int));
				}
			}
		}

		shatterIndices.UnmapBuffer();
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

		shatterParts[pieceNum].renderData.clear();
		shatterParts[pieceNum].renderData.reserve(backIdx + 1);

		// finish: copy buffer to actual memory
		for (size_t n = 0; n <= backIdx; n++) {
			shatterParts[pieceNum].renderData.push_back(shatterPartsBuf[n].first);
		}
	}
}


void S3DModelPiece::Shatter(const S3DModel* mdl, int team, float pieceChance, const float3& pos, const float3& speed, const CMatrix44f& m) const
{
	const float2  pieceParams = {float3::max(float3::fabs(maxs), float3::fabs(mins)).Length(), pieceChance};
	const   int2 renderParams = {mdl->textureType, team};

	projectileHandler.AddFlyingPiece(mdl, this, m, pos, speed, pieceParams, renderParams);
}



/** ****************************************************************************************************
 * LocalModel
 */

void LocalModel::UpdatePieceMatrices(unsigned int gsFrameNum)
{
	if (gsFrameNum == pmuFrameNum)
		return;

	// could be combined with UpdateChildMatricesRec, but KISS
	for (size_t i = 0, n = pieces.size(); i < n; i++) {
		const LocalModelPiece& lmp = pieces[i];

		// set a null-matrix for invisible pieces; empty pieces are not uploaded
		matrices[i] = lmp.GetModelSpaceMatrix();
		matrices[i] *= float(lmp.scriptSetVisible);
	}

	pmuFrameNum = gsFrameNum;
}

void LocalModel::Draw() const
{
	glBindVertexArray(vertexArray);

	#if 0
	switch (primType) {
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			glPrimitiveRestartIndex(-1u);
			glEnableClientState(GL_PRIMITIVE_RESTART);
			glDrawElements(GL_TRIANGLE_STRIP,  0, vboNumIndcs, GL_UNSIGNED_INT, nullptr);
			glDisableClientState(GL_PRIMITIVE_RESTART);
		} break;
	}
	#else
	glDrawElements(GL_TRIANGLES, vboNumIndcs, GL_UNSIGNED_INT, nullptr);
	#endif

	glBindVertexArray(0);
}

void LocalModel::DrawPiece(const LocalModelPiece* lmp) const 
{
	assert((lmp - &pieces[0]) < pieces.size());

	const S3DModelPiece* omp = lmp->original;
	const std::vector<unsigned int>& indcs = omp->GetVertexIndices();

	// draw the buffer sub-region corresponding to this piece
	glBindVertexArray(vertexArray);
	glDrawElements(GL_TRIANGLES, indcs.size(), GL_UNSIGNED_INT, VA_TYPE_OFFSET(uint32_t, omp->vboStartIndx));
	glBindVertexArray(0);
}




void LocalModel::SetModel(const S3DModel* model, bool initialize)
{
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	pmuFrameNum = -1u;
	vertexArray = model->vertexArray;
	elemsBuffer = model->elemsBuffer;
	indcsBuffer = model->indcsBuffer;
	vboNumVerts = model->vboNumVerts;
	vboNumIndcs = model->vboNumIndcs;

	if (!initialize) {
		assert(pieces.size() == model->numPieces);
		matrices.clear();
		matrices.resize(pieces.size());

		// PostLoad; only update the pieces
		for (size_t n = 0; n < pieces.size(); n++) {
			pieces[n].original = model->GetPiece(n);
		}

		UpdateVolumeAndMatrices(true);
		return;
	}

	assert(pieces.empty());
	pieces.clear();
	pieces.reserve(model->numPieces);

	CreateLocalModelPieces(model->GetRootPiece());

	assert(pieces.size() == model->numPieces);
	matrices.clear();
	matrices.resize(pieces.size());

	// must recursively update matrices here too since
	// LocalModel::Update is never called for features
	// (which might have baked piece rotations if .dae)
	UpdateVolumeAndMatrices(false);
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

	for (const auto& lmPiece: pieces) {
		const CMatrix44f& matrix = lmPiece.GetModelSpaceMatrix();
		const S3DModelPiece* piece = lmPiece.original;

		// skip empty pieces or bounds will not be sensible
		if (!piece->HasGeometryData())
			continue;

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

		for (const float3& v: verts) {
			const float3 vertex = matrix * v;

			bbMins = float3::min(bbMins, vertex);
			bbMaxs = float3::max(bbMaxs, vertex);
		}
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

	pieceSpaceMat = CalcPieceSpaceMatrix(pos, rot, original->scales);

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

		pieceSpaceMat = CalcPieceSpaceMatrix(pos, rot, original->scales);
	}

	if (updateChildMatrices) {
		modelSpaceMat = pieceSpaceMat;

		if (parent != nullptr) {
			modelSpaceMat >>= parent->modelSpaceMat;
		}
	}

	for (auto& child : children) {
		child->UpdateChildMatricesRec(updateChildMatrices);
	}
}

void LocalModelPiece::UpdateParentMatricesRec() const
{
	if (parent != nullptr && parent->dirty)
		parent->UpdateParentMatricesRec();

	dirty = false;

	pieceSpaceMat = CalcPieceSpaceMatrix(pos, rot, original->scales);
	modelSpaceMat = pieceSpaceMat;

	if (parent != nullptr)
		modelSpaceMat >>= parent->modelSpaceMat;
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
