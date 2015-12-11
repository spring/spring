/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"


#include "3DOParser.h"
#include "S3OParser.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/Util.h"

#include <algorithm>
#include <cctype>

CR_BIND(LocalModelPiece, (NULL))
CR_REG_METADATA(LocalModelPiece, (
	CR_MEMBER(pos),
	CR_MEMBER(rot),
	CR_MEMBER(dir),
	CR_MEMBER(pieceSpaceMat),
	CR_MEMBER(modelSpaceMat),
	CR_MEMBER(colvol),
	CR_MEMBER(numUpdatesSynced),
	CR_MEMBER(lastMatrixUpdate),
	CR_MEMBER(scriptSetVisible),
	CR_MEMBER(identityTransform),
	CR_MEMBER(lmodelPieceIndex),
	CR_MEMBER(scriptPieceIndex),
	CR_MEMBER(parent),

	// reload
	CR_IGNORED(children),
	CR_IGNORED(dispListID),
	CR_IGNORED(original),

	CR_IGNORED(lodDispLists) //FIXME GL idx!
))

CR_BIND(LocalModel, )
CR_REG_METADATA(LocalModel, (
	CR_IGNORED(dirtyPieces),
	CR_IGNORED(lodCount), //FIXME?
	CR_MEMBER(pieces)
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
	, hasGeometryData(false)
	, hasIdentityRot(true)
	, scales(OnesVector)
	, mins(DEF_MIN_SIZE)
	, maxs(DEF_MAX_SIZE)
	, rotAxisSigns(-OnesVector)
	, dispListID(0)
{
}

S3DModelPiece::~S3DModelPiece()
{
	glDeleteLists(dispListID, 1);
}

unsigned int S3DModelPiece::CreateDrawForList() const
{
	const unsigned int dlistID = glGenLists(1);

	glNewList(dlistID, GL_COMPILE);
	DrawForList();
	glEndList();

	return dlistID;
}

void S3DModelPiece::DrawStatic() const
{
	CMatrix44f mat;

	// get the static transform (sans script influences)
	ComposeTransform(mat, offset, ZeroVector, scales);

	glPushMatrix();
	glMultMatrixf(mat);

	if (hasGeometryData)
		glCallList(dispListID);

	for (unsigned int n = 0; n < children.size(); n++) {
		children[n]->DrawStatic();
	}

	glPopMatrix();
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



void LocalModel::SetModel(const S3DModel* model)
{
	// make sure we do not get called for trees, etc
	assert(model != nullptr);
	assert(model->numPieces >= 1);

	dirtyPieces = model->numPieces;
	bvFrameTime = 0;
	lodCount = 0;

	pieces.reserve(model->numPieces);

	CreateLocalModelPieces(model->GetRootPiece());
	UpdateBoundingVolume(0);

	assert(pieces.size() == model->numPieces);
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	LocalModelPiece* lmpChild = NULL;

	pieces.emplace_back(mpParent);
	LocalModelPiece *lmpParent = &pieces.back();

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

void LocalModel::UpdateBoundingVolume(unsigned int frameNum)
{
	bvFrameTime = frameNum;

	bbMins = DEF_MIN_SIZE;
	bbMaxs = DEF_MAX_SIZE;

	for (unsigned int n = 0; n < pieces.size(); n++) {
		const CMatrix44f& matrix = pieces[n].GetModelSpaceMatrix();
		const S3DModelPiece* piece = pieces[n].original;

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

	, numUpdatesSynced(1)
	, lastMatrixUpdate(0)

	, scriptSetVisible(piece->HasGeometryData())
	, identityTransform(true)

	, lmodelPieceIndex(-1)
	, scriptPieceIndex(-1)

	, original(piece)
	, parent(NULL) // set later
{
	assert(piece != NULL);

	pos =  piece->offset;
	dir = (piece->GetVertexCount() >= 2)? (piece->GetVertexPos(0) - piece->GetVertexPos(1)): FwdVector;

	identityTransform = UpdateMatrix();
	dispListID = piece->GetDisplayListID();

	children.reserve(piece->children.size());
}


bool LocalModelPiece::UpdateMatrix()
{
	return (original->ComposeTransform(pieceSpaceMat.LoadIdentity(), pos, rot, original->scales));
}

void LocalModelPiece::UpdateMatricesRec(bool updateChildMatrices)
{
	if (lastMatrixUpdate != numUpdatesSynced) {
		lastMatrixUpdate = numUpdatesSynced;
		identityTransform = UpdateMatrix();
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



void LocalModelPiece::Draw() const
{
	if (!scriptSetVisible)
		return;

	glPushMatrix();
	glMultMatrixf(modelSpaceMat);
	glCallList(dispListID);
	glPopMatrix();
}

void LocalModelPiece::DrawLOD(unsigned int lod) const
{
	if (!scriptSetVisible)
		return;

	glPushMatrix();
	glMultMatrixf(modelSpaceMat);
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
	if (original == NULL)
		return false;

	switch (original->GetVertexCount()) {
		case 0: {
			emitPos = modelSpaceMat.GetPos();
			emitDir = modelSpaceMat.Mul(FwdVector) - emitPos;
		} break;
		case 1: {
			emitPos = modelSpaceMat.GetPos();
			emitDir = modelSpaceMat.Mul(original->GetVertexPos(0)) - emitPos;
		} break;
		default: {
			const float3 p1 = modelSpaceMat.Mul(original->GetVertexPos(0));
			const float3 p2 = modelSpaceMat.Mul(original->GetVertexPos(1));

			emitPos = p1;
			emitDir = p2 - p1;
		} break;
	}

	// note: actually OBJECT_TO_WORLD but transform is the same
	emitPos *= WORLD_TO_OBJECT_SPACE;
	emitDir *= WORLD_TO_OBJECT_SPACE;
	return true;
}

/******************************************************************************/
/******************************************************************************/
