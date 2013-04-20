/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DModel.h"


#include "3DOParser.h"
#include "S3OParser.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/Util.h"

#include <algorithm>
#include <cctype>

CR_BIND(LocalModelPiece, (NULL));
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
));

CR_BIND(LocalModel, (NULL));
CR_REG_METADATA(LocalModel, (
	CR_IGNORED(original),
	CR_IGNORED(dirtyPieces),
	CR_IGNORED(lodCount), //FIXME?
	CR_MEMBER(pieces)
));


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

S3DModelPiece* S3DModel::FindPiece(const std::string& name) const
{
    const ModelPieceMap::const_iterator it = pieces.find(name);
    if (it != pieces.end()) return it->second;
    return NULL;
}


/** ****************************************************************************************************
 * S3DModelPiece
 */

S3DModelPiece::S3DModelPiece()
	: parent(NULL)
	, colvol(NULL)
	, type(MODELTYPE_OTHER)
	, isEmpty(true)
	, mIsIdentity(true)
	, dispListID(0)
{
}

S3DModelPiece::~S3DModelPiece()
{
	glDeleteLists(dispListID, 1);
	delete colvol;
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
	const bool transform = (offset.SqLength() != 0.0f || !mIsIdentity);

	if (transform) {
		glPushMatrix();
		if (!mIsIdentity) glMultMatrixf(scaleRotMatrix);
		glTranslatef(offset.x, offset.y, offset.z);
	}

		if (!isEmpty)
			glCallList(dispListID);

		for (std::vector<S3DModelPiece*>::const_iterator ci = children.begin(); ci != children.end(); ++ci) {
			(*ci)->DrawStatic();
		}

	if (transform) {
		glPopMatrix();
	}
}



/** ****************************************************************************************************
 * LocalModel
 */

void LocalModel::DrawPieces() const
{
	for (unsigned int i = 0; i < pieces.size(); i++) {
		pieces[i]->Draw();
	}
}

void LocalModel::DrawPiecesLOD(unsigned int lod) const
{
	for (unsigned int i = 0; i < pieces.size(); i++) {
		pieces[i]->DrawLOD(lod);
	}
}

void LocalModel::SetLODCount(unsigned int count)
{
	pieces[0]->SetLODCount(lodCount = count);
}



void LocalModel::ReloadDisplayLists()
{
	for (std::vector<LocalModelPiece*>::iterator piece = pieces.begin(); piece != pieces.end(); ++piece) {
		(*piece)->dispListID = (*piece)->original->GetDisplayListID();
	}
}

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent)
{
	LocalModelPiece* lmpParent = new LocalModelPiece(mpParent);
	LocalModelPiece* lmpChild = NULL;

	pieces.push_back(lmpParent);

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



/** ****************************************************************************************************
 * LocalModelPiece
 */

LocalModelPiece::LocalModelPiece(const S3DModelPiece* piece)
	: colvol(new CollisionVolume(piece->GetCollisionVolume()))

	, numUpdatesSynced(1)
	, lastMatrixUpdate(0)

	, scriptSetVisible(!piece->isEmpty)
	, identityTransform(true)

	, lmodelPieceIndex(-1)
	, scriptPieceIndex(-1)

	, original(piece)
	, parent(NULL) // set later
{
	assert(piece != NULL);

	dispListID =  piece->GetDisplayListID();
	pos        =  piece->offset;
	rot        =  piece->rot;

	children.reserve(piece->children.size());

	if (piece->GetVertexCount() < 2) {
		dir = OnesVector;
	} else {
		dir = piece->GetVertexPos(0) - piece->GetVertexPos(1);
	}

	identityTransform = UpdateMatrix();
}

LocalModelPiece::~LocalModelPiece() {
	delete colvol; colvol = NULL;
}

bool LocalModelPiece::UpdateMatrix()
{
	bool r = original->mIsIdentity;

	{
		// for Assimp models SRM = T * R * S (SRT vs. RT in spring)
		// for all other models SRM = identity
		pieceSpaceMat = original->scaleRotMatrix;

		// Translate & Rotate are faster than matrix-mul!
		if (pos.SqLength() != 0.0f) { pieceSpaceMat.Translate(pos);  r = false; }
		if (         rot.y != 0.0f) { pieceSpaceMat.RotateY(-rot.y); r = false; } // yaw
		if (         rot.x != 0.0f) { pieceSpaceMat.RotateX(-rot.x); r = false; } // pitch
		if (         rot.z != 0.0f) { pieceSpaceMat.RotateZ(-rot.z); r = false; } // roll
	}

	return r;
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
			modelSpaceMat *= parent->modelSpaceMat;
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
	const unsigned int oldCount = lodDispLists.size();

	lodDispLists.resize(count);
	for (unsigned int i = oldCount; i < count; i++) {
		lodDispLists[i] = 0;
	}

	for (unsigned int i = 0; i < children.size(); i++) {
		children[i]->SetLODCount(count);
	}
}


#if defined(USE_GML) && defined(__GNUC__) && (__GNUC__ == 4)
// This is supposed to fix some GCC crashbug related to threading
// The MOVAPS SSE instruction is otherwise getting misaligned data
__attribute__ ((force_align_arg_pointer))
#endif
float3 LocalModelPiece::GetAbsolutePos() const
{
	// note: actually OBJECT_TO_WORLD but transform is the same
	return (modelSpaceMat.GetPos() * WORLD_TO_OBJECT_SPACE);
}


bool LocalModelPiece::GetEmitDirPos(float3& pos, float3& dir) const
{
	const S3DModelPiece* piece = original;

	if (piece == NULL)
		return false;

	const unsigned int count = piece->GetVertexCount();

	if (count == 0) {
		pos = modelSpaceMat.GetPos();
		dir = modelSpaceMat.Mul(FwdVector) - pos;
	} else if (count == 1) {
		pos = modelSpaceMat.GetPos();
		dir = modelSpaceMat.Mul(piece->GetVertexPos(0)) - pos;
	} else if (count >= 2) {
		float3 p1 = modelSpaceMat.Mul(piece->GetVertexPos(0));
		float3 p2 = modelSpaceMat.Mul(piece->GetVertexPos(1));

		pos = p1;
		dir = p2 - p1;
	}

	// note: actually OBJECT_TO_WORLD but transform is the same
	pos *= WORLD_TO_OBJECT_SPACE;
	dir *= WORLD_TO_OBJECT_SPACE;
	return true;
}

/******************************************************************************/
/******************************************************************************/
