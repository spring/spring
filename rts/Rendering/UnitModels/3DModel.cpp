#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <algorithm>
#include <cctype>
#include "mmgr.h"

#include "3DModel.h"
#include "3DOParser.h"
#include "s3oParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/FartextureHandler.h"
#include "Util.h"
#include "LogOutput.h"
#include "Exceptions.h"


//////////////////////////////////////////////////////////////////////
// S3DModelPiece
//

void S3DModelPiece::DrawStatic() const
{
	glPushMatrix();
	glTranslatef(offset.x, offset.y, offset.z);
	glCallList(displist);
	for (std::vector<S3DModelPiece*>::const_iterator ci = childs.begin(); ci != childs.end(); ci++) {
		(*ci)->DrawStatic();
	}
	glPopMatrix();
}


S3DModelPiece::~S3DModelPiece()
{
	glDeleteLists(displist, 1);
	delete colvol;
}


/******************************************************************************/
/******************************************************************************/
//
//  LocalModel
//

LocalModel::~LocalModel()
{
	// delete the local piece copies
	for (std::vector<LocalModelPiece*>::iterator pi = pieces.begin(); pi != pieces.end(); pi++) {
		delete (*pi)->colvol;
		delete (*pi);
	}
	pieces.clear();
}


void LocalModel::SetLODCount(unsigned int count)
{
	lodCount = count;
	pieces[0]->SetLODCount(count);
}


void LocalModel::ApplyRawPieceTransform(int piecenum) const
{
	pieces[piecenum]->ApplyTransform();
}


float3 LocalModel::GetRawPiecePos(int piecenum) const
{
	return pieces[piecenum]->GetPos();
}


CMatrix44f LocalModel::GetRawPieceMatrix(int piecenum) const
{
	return pieces[piecenum]->GetMatrix();
}


//gets the number of vertices in the piece
int LocalModel::GetRawPieceVertCount(int piecenum) const
{
	return pieces[piecenum]->original->vertexCount;
}


void LocalModel::GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const
{
	pieces[piecenum]->GetEmitDirPos(pos, dir);
}


//Only useful for special pieces used for emit-sfx
float3 LocalModel::GetRawPieceDirection(int piecenum) const
{
	return pieces[piecenum]->GetDirection();
}


/******************************************************************************/
/******************************************************************************/
//
//  LocalModelPiece
//

static const float RADTOANG  = 180 / PI;

void LocalModelPiece::Draw() const
{
	if (!visible && childs.size()==0)
		return;

	glPushMatrix();

	if (pos.x || pos.y || pos.z) { glTranslatef(pos.x, pos.y, pos.z); }
	if (rot[1]) { glRotatef( rot[1] * RADTOANG, 0.0f, 1.0f, 0.0f); }
	if (rot[0]) { glRotatef( rot[0] * RADTOANG, 1.0f, 0.0f, 0.0f); }
	if (rot[2]) { glRotatef(-rot[2] * RADTOANG, 0.0f, 0.0f, 1.0f); }

	if (visible)
		glCallList(displist);

	for (unsigned int i = 0; i < childs.size(); i++) {
		childs[i]->Draw();
	}

	glPopMatrix();
}


void LocalModelPiece::DrawLOD(unsigned int lod) const
{
	if (!visible && childs.size()==0)
		return;

	glPushMatrix();

	if (pos.x || pos.y || pos.z) { glTranslatef(pos.x, pos.y, pos.z); }
	if (rot[1]) { glRotatef( rot[1] * RADTOANG, 0.0f, 1.0f, 0.0f); }
	if (rot[0]) { glRotatef( rot[0] * RADTOANG, 1.0f, 0.0f, 0.0f); }
	if (rot[2]) { glRotatef(-rot[2] * RADTOANG, 0.0f, 0.0f, 1.0f); }

	if (visible)
		glCallList(lodDispLists[lod]);

	for (unsigned int i = 0; i < childs.size(); i++) {
		childs[i]->DrawLOD(lod);
	}

	glPopMatrix();
}


void LocalModelPiece::ApplyTransform() const
{
	if (parent) {
		parent->ApplyTransform();
	}

	if (pos.x || pos.y || pos.z) { glTranslatef(pos.x, pos.y, pos.z); }
	if (rot[1]) { glRotatef( rot[1] * RADTOANG, 0.0f, 1.0f, 0.0f); }
	if (rot[0]) { glRotatef( rot[0] * RADTOANG, 1.0f, 0.0f, 0.0f); }
	if (rot[2]) { glRotatef(-rot[2] * RADTOANG, 0.0f, 0.0f, 1.0f); }
}


void LocalModelPiece::GetPiecePosIter(CMatrix44f* mat) const
{
	if (parent) {
		parent->GetPiecePosIter(mat);
	}

	if (pos.x || pos.y || pos.z) { mat->Translate(pos.x, pos.y, -pos.z); }
	if (rot[1]) { mat->RotateY(rot[1]); }
	if (rot[0]) { mat->RotateX(rot[0]); }
	if (rot[2]) { mat->RotateZ(rot[2]); }
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


float3 LocalModelPiece::GetPos() const
{
	CMatrix44f mat;
	GetPiecePosIter(&mat);

	if (type==MODELTYPE_3DO) {
		// fix for valkyres
		const S3DOPiece* p3 = static_cast<S3DOPiece*>(original);
		if (p3 && (p3->vertices.size() == 2)) {
			const std::vector<S3DOVertex>& pv = p3->vertices;
			if (pv[0].pos.y > pv[1].pos.y) {
				mat.Translate(pv[0].pos.x, pv[0].pos.y, -pv[0].pos.z);
			} else {
				mat.Translate(pv[1].pos.x, pv[1].pos.y, -pv[1].pos.z);
			}
		}
	}

	float3 pos = mat.GetPos();
	pos.z *= -1.0f;
	pos.x *= -1.0f;

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
	const unsigned int count = piece->vertexCount;
	if (count < 2) {
		return float3(1.0f, 1.0f, 1.0f);
	}else if (count > 2) {
		//this is strange too, but probably caused by an incorrect 3rd party unit
	}
	return piece->GetVertexPos(0) - piece->GetVertexPos(1);
}


bool LocalModelPiece::GetEmitDirPos(float3 &pos, float3 &dir) const
{
	CMatrix44f mat;
	GetPiecePosIter(&mat);

	//hm...
	static const float3 invAxis(-1, 1, -1);
	static const float3 invVertAxis(1, 1, -1);

	const S3DModelPiece* piece = original;
	if (!piece)
		return false;

	if (piece->vertexCount == 0) {
		pos = mat.GetPos()*invAxis;
		dir = mat.Mul(float3(0,0,-1))*invAxis - pos;
	}
	else if(piece->vertexCount == 1) {
		pos = mat.GetPos()*invAxis;
		dir = mat.Mul(piece->GetVertexPos(0)*invVertAxis)*invAxis - pos;
	}
	else if(piece->vertexCount >= 2) {
		float3 p1 = mat.Mul(piece->GetVertexPos(0) * invVertAxis) * invAxis;
		float3 p2 = mat.Mul(piece->GetVertexPos(1) * invVertAxis) * invAxis;

		pos = p1;
		dir = p2 - p1;
	} else {
		return false;
	}
	return true;
}

/******************************************************************************/
/******************************************************************************/
