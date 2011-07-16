/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include <algorithm>
#include <cctype>
#include "System/mmgr.h"

#include "3DModel.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/LogOutput.h"


/** ****************************************************************************************************
 * S3DModel
 */

S3DModelPiece* S3DModel::FindPiece( std::string name )
{
    const ModelPieceMap::const_iterator it = pieces.find(name);
    if (it != pieces.end()) return it->second;
    return NULL;
}


/** ****************************************************************************************************
 * S3DModelPiece
 */

void S3DModelPiece::DrawStatic() const
{
	const bool needTrafo = (offset.SqLength() != 0.f);
	if (needTrafo) {
		glPushMatrix();
		glTranslatef(offset.x, offset.y, offset.z);
	}

		if (!isEmpty)
			glCallList(dispListID);

		for (std::vector<S3DModelPiece*>::const_iterator ci = childs.begin(); ci != childs.end(); ++ci) {
			(*ci)->DrawStatic();
		}

	if (needTrafo) {
		glPopMatrix();
	}
}


S3DModelPiece::~S3DModelPiece()
{
	glDeleteLists(dispListID, 1);
	delete colvol;
}


/** ****************************************************************************************************
 * LocalModel
 */

LocalModelPiece* LocalModel::CreateLocalModelPieces(const S3DModelPiece* mpParent, size_t pieceNum)
{
	LocalModelPiece* lmpParent = new LocalModelPiece(mpParent);
	pieces.push_back(lmpParent);

	LocalModelPiece* lmpChild = NULL;
	for (unsigned int i = 0; i < mpParent->GetChildCount(); i++) {
		lmpChild = CreateLocalModelPieces(mpParent->GetChild(i), ++pieceNum);
		lmpChild->SetParent(lmpParent);
		lmpParent->AddChild(lmpChild);
	}

	return lmpParent;
}


void LocalModel::SetLODCount(unsigned int count)
{
	lodCount = count;
	pieces[0]->SetLODCount(count);
}


void LocalModel::ApplyRawPieceTransformUnsynced(int piecenum) const
{
	pieces[piecenum]->ApplyTransformUnsynced();
}


float3 LocalModel::GetRawPiecePos(int piecenum) const
{
	return pieces[piecenum]->GetAbsolutePos();
}


CMatrix44f LocalModel::GetRawPieceMatrix(int piecenum) const
{
	return pieces[piecenum]->GetMatrix();
}


void LocalModel::GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const
{
	pieces[piecenum]->GetEmitDirPos(pos, dir);
}


//! Only useful for special pieces. Used for emit-sfx.
float3 LocalModel::GetRawPieceDirection(int piecenum) const
{
	return pieces[piecenum]->GetDirection();
}


/** ****************************************************************************************************
 * LocalModelPiece
 */

static const float RADTOANG  = 180 / PI;

LocalModelPiece::LocalModelPiece(const S3DModelPiece* piece)
	: numUpdatesSynced(1)
	, lastMatrixUpdate(0)
{
	assert(piece);
	original   =  piece;
	parent     =  NULL; // set later
	dispListID =  piece->dispListID;
	visible    = !piece->isEmpty;
	identity   =  true;
	pos        =  piece->offset;
	colvol     =  new CollisionVolume(piece->GetCollisionVolume());
	childs.reserve(piece->childs.size());
}

LocalModelPiece::~LocalModelPiece() {
	delete colvol; colvol = NULL;
}


inline void LocalModelPiece::CheckUpdateMatrixUnsynced()
{
	if (lastMatrixUpdate != numUpdatesSynced) {
		lastMatrixUpdate = numUpdatesSynced;
		identity = true;

		transfMat.LoadIdentity();

		if (pos.SqLength() > 0.0f) { transfMat.Translate(pos.x, pos.y, pos.z); identity = false; }
		if (rot[1] != 0.0f) { transfMat.RotateY(-rot[1]); identity = false; }
		if (rot[0] != 0.0f) { transfMat.RotateX(-rot[0]); identity = false; }
		if (rot[2] != 0.0f) { transfMat.RotateZ(-rot[2]); identity = false; }
	}
}



void LocalModelPiece::Draw()
{
	if (!visible && childs.empty())
		return;

	CheckUpdateMatrixUnsynced();

	if (!identity) {
		glPushMatrix();
		glMultMatrixf(transfMat);
	}

		if (visible)
			glCallList(dispListID);

		for (unsigned int i = 0; i < childs.size(); i++) {
			childs[i]->Draw();
		}
	
	if (!identity) {
		glPopMatrix();
	}
}


void LocalModelPiece::DrawLOD(unsigned int lod)
{
	if (!visible && childs.empty())
		return;

	CheckUpdateMatrixUnsynced();
	
	if (!identity) {
		glPushMatrix();
		glMultMatrixf(transfMat);
	}

		if (visible)
			glCallList(lodDispLists[lod]);

		for (unsigned int i = 0; i < childs.size(); i++) {
			childs[i]->DrawLOD(lod);
		}
	
	if (!identity) {
		glPopMatrix();
	}
}


void LocalModelPiece::ApplyTransformUnsynced()
{
	if (parent) {
		parent->ApplyTransformUnsynced();
	}

	CheckUpdateMatrixUnsynced();

	if (!identity) {
		glMultMatrixf(transfMat);
	}
}


void LocalModelPiece::GetPiecePosIter(CMatrix44f* mat) const
{
	if (parent) {
		parent->GetPiecePosIter(mat);
	}

/**/
	if (pos.SqLength()) { mat->Translate(pos.x, pos.y, pos.z); }
	if (rot[1]) { mat->RotateY(-rot[1]); }
	if (rot[0]) { mat->RotateX(-rot[0]); }
	if (rot[2]) { mat->RotateZ(-rot[2]); }
/**/
	//(*mat) *= transMat; //! Translate & Rotate are faster than matrix-mul!
}


void LocalModelPiece::SetLODCount(unsigned int count)
{
	const unsigned int oldCount = lodDispLists.size();

	lodDispLists.resize(count);
	for (unsigned int i = oldCount; i < count; i++) {
		lodDispLists[i] = 0;
	}

	for (unsigned int i = 0; i < childs.size(); i++) {
		childs[i]->SetLODCount(count);
	}
}


#if defined(USE_GML) && defined(__GNUC__) && (__GNUC__ == 4)
//! This is supposed to fix some GCC crashbug related to threading
//! The MOVAPS SSE instruction is otherwise getting misaligned data
__attribute__ ((force_align_arg_pointer))
#endif
float3 LocalModelPiece::GetAbsolutePos() const
{
	CMatrix44f mat;
	GetPiecePosIter(&mat);

	mat.Translate(original->GetPosOffset());

	//! we use a 'right' vector, and the positive x axis points to the left
	float3 pos = mat.GetPos();
	pos.x = -pos.x;
	return pos;
}


CMatrix44f LocalModelPiece::GetMatrix() const
{
	CMatrix44f mat;
	GetPiecePosIter(&mat);
	return mat;
}


float3 LocalModelPiece::GetDirection() const
{
	const S3DModelPiece* piece = original;
	const unsigned int count = piece->GetVertexCount();
	if (count < 2) {
		return float3(1.0f, 1.0f, 1.0f);
	}
	return (piece->GetVertexPos(0) - piece->GetVertexPos(1));
}


bool LocalModelPiece::GetEmitDirPos(float3& pos, float3& dir) const
{
	CMatrix44f mat;
	GetPiecePosIter(&mat);

	const S3DModelPiece* piece = original;

	if (piece == NULL)
		return false;

	const unsigned int count = piece->GetVertexCount();

	if (count == 0) {
		pos = mat.GetPos();
		dir = mat.Mul(float3(0.0f, 0.0f, 1.0f)) - pos;
	} else if (count == 1) {
		pos = mat.GetPos();
		dir = mat.Mul(piece->GetVertexPos(0)) - pos;
	} else if (count >= 2) {
		float3 p1 = mat.Mul(piece->GetVertexPos(0));
		float3 p2 = mat.Mul(piece->GetVertexPos(1));

		pos = p1;
		dir = p2 - p1;
	} else {
		return false;
	}

	//! we use a 'right' vector, and the positive x axis points to the left
	pos.x = -pos.x;
	dir.x = -dir.x;

	return true;
}

/******************************************************************************/
/******************************************************************************/
