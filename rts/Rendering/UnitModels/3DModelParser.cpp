#include "StdAfx.h"
#include "3DModelParser.h"
#include "3DOParser.h"
#include "s3oParser.h"
#include "Sim/Units/COB/CobInstance.h"
#include <algorithm>
#include <cctype>

C3DModelParser* modelParser=0;

C3DModelParser::C3DModelParser(void)
{
	unit3doparser=SAFE_NEW C3DOParser();
	units3oparser=SAFE_NEW CS3OParser();
}

C3DModelParser::~C3DModelParser(void)
{
	delete unit3doparser;
	delete units3oparser;
}

S3DOModel* C3DModelParser::Load3DModel(std::string name, float scale, int side)
{
	// TODO: abstract this
	StringToLowerInPlace(name);
	if (name.find(".s3o") != std::string::npos)
		return units3oparser->LoadS3O(name, scale, side);
	else
		return unit3doparser->Load3DO(name, scale, side);
}

/*
S3DOModel* C3DModelParser::Load3DO(string name,float scale,int side,const float3& offsets)
{
	StringToLowerInPlace(name);
	if(name.find(".s3o")!=string::npos)
		return units3oparser->LoadS3O(name,scale,side);
	else
		return unit3doparser->Load3DO(name,scale,side,offsets);
}
*/

LocalS3DOModel *C3DModelParser::CreateLocalModel(S3DOModel *model, std::vector<struct PieceInfo> *pieces)
{
	LocalS3DOModel* lm;
	if (model->rootobject3do) {
		lm = unit3doparser->CreateLocalModel(model,pieces);
	} else {
		lm = units3oparser->CreateLocalModel(model,pieces);
	}
	return lm;
}


/******************************************************************************/
/******************************************************************************/
//
//  S3DOModel
//

void S3DOModel::DrawStatic()
{
	if (rootobject3do)
		rootobject3do->DrawStatic();
	else
		rootobjects3o->DrawStatic();
}


/******************************************************************************/
/******************************************************************************/
//
//  LocalS3DOModel
//

LocalS3DOModel::~LocalS3DOModel()
{
	delete [] pieces;
	delete [] scritoa;
}


static const float CORDDIV = 65536.0f;
static const float ANGDIV  = 182.0f;


void LocalS3DOModel::Draw() const
{
	pieces->Draw();
}


void LocalS3DOModel::DrawLOD(unsigned int lod) const
{
	if (lod > lodCount) {
		return;
	}
	pieces->DrawLOD(lod);
}


void LocalS3DOModel::SetLODCount(unsigned int count)
{
	lodCount = count;
	pieces->SetLODCount(count);
}


void LocalS3DOModel::ApplyRawPieceTransform(int piecenum) const
{
	pieces[piecenum].ApplyTransform();
}


float3 LocalS3DOModel::GetRawPiecePos(int piecenum) const
{
	CMatrix44f mat;
	pieces[piecenum].GetPiecePosIter(&mat);

	// stupid fix for valkyres
	const S3DO* p3do = pieces[piecenum].original3do;
	if (p3do && (p3do->vertices.size() == 2)) {
		const std::vector<S3DOVertex>& pv = p3do->vertices;
		if (pv[0].pos.y > pv[1].pos.y) {
			mat.Translate(pv[0].pos.x, pv[0].pos.y, -pv[0].pos.z);
		} else {
			mat.Translate(pv[1].pos.x, pv[1].pos.y, -pv[1].pos.z);
		}
	}

/*
	logOutput.Print("%f %f %f %f",mat[0],mat[4],mat[8],mat[12]);
	logOutput.Print("%f %f %f %f",mat[1],mat[5],mat[9],mat[13]);
	logOutput.Print("%f %f %f %f",mat[2],mat[6],mat[10],mat[14]);
	logOutput.Print("%f %f %f %f",mat[3],mat[7],mat[11],mat[15]);/**/
	float3 pos = mat.GetPos();
	pos.z *= -1.0f;
	pos.x *= -1.0f;

	return pos;
}


CMatrix44f LocalS3DOModel::GetRawPieceMatrix(int piecenum) const
{
	CMatrix44f mat;
	pieces[piecenum].GetPiecePosIter(&mat);

	return mat;
}


//gets the number of vertices in the piece
int LocalS3DOModel::GetRawPieceVertCount(int piecenum) const
{
	if (pieces[piecenum].original3do) {
		S3DO &orig = *pieces[piecenum].original3do;
		return orig.vertices.size();
	} else {
		SS3O &orig = *pieces[piecenum].originals3o;
		return orig.vertices.size();
	}
}


void LocalS3DOModel::GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const
{
	CMatrix44f mat;
	pieces[piecenum].GetPiecePosIter(&mat);

	//hm...
	static const float3 invAxis(-1, 1, -1);
	static const float3 invVertAxis(1, 1, -1);

	if (pieces[piecenum].original3do) {
		S3DO &orig = *pieces[piecenum].original3do;

		if (orig.vertices.size() == 0) {
			pos = mat.GetPos()*invAxis;
			dir = mat.Mul(float3(0,0,-1))*invAxis - pos;
		}
		else if (orig.vertices.size() == 1) {
			pos = mat.GetPos()*invAxis;
			dir = mat.Mul(orig.vertices[0].pos*invVertAxis)*invAxis - pos;
		}
		else {
			float3 p1 = mat.Mul(orig.vertices[0].pos * invVertAxis) * invAxis;

			float3 p2 = mat.Mul(orig.vertices[1].pos * invVertAxis) * invAxis;

			pos = p1;
			dir = p2 - p1;
		}
	}
	else {
		SS3O &orig = *pieces[piecenum].originals3o;

		if (orig.vertices.size() == 0) {
			pos = mat.GetPos()*invAxis;
			dir = mat.Mul(float3(0,0,-1))*invAxis - pos;
		}
		else if(orig.vertices.size() == 1) {
			pos = mat.GetPos()*invAxis;
			dir = mat.Mul(orig.vertices[0].pos*invVertAxis)*invAxis - pos;
		}
		else {
			float3 p1 = mat.Mul(orig.vertices[0].pos * invVertAxis) * invAxis;

			float3 p2 = mat.Mul(orig.vertices[1].pos * invVertAxis) * invAxis;

			pos = p1;
			dir = p2 - p1;
		}
	}
}


//Only useful for special pieces used for emit-sfx
float3 LocalS3DOModel::GetRawPieceDirection(int piecenum) const
{
	if (pieces[piecenum].original3do) {
		S3DO &orig = *pieces[piecenum].original3do;
		if (orig.vertices.size() < 2) {
			//logOutput.Print("Use of GetPieceDir on strange piece (%d vertices)", orig.vertices.size());
			return float3(1,1,1);
		}
		else if (orig.vertices.size() > 2) {
			//this is strange too, but probably caused by an incorrect 3rd party unit
		}
		//logOutput.Print("Vertexes %f %f %f", orig.vertices[0].pos.x, orig.vertices[0].pos.y, orig.vertices[0].pos.z);
		//logOutput.Print("Vertexes %f %f %f", orig.vertices[1].pos.x, orig.vertices[1].pos.y, orig.vertices[1].pos.z);
		return orig.vertices[0].pos - orig.vertices[1].pos;
	}
	else {
		SS3O &orig = *pieces[piecenum].originals3o;
		if (orig.vertices.size() < 2) {
			return float3(1.0f, 1.0f, 1.0f);
		}
		else if (orig.vertices.size() > 2) {
			//this is strange too, but probably caused by an incorrect 3rd party unit
		}
		return orig.vertices[0].pos - orig.vertices[1].pos;
	}
}


/******************************************************************************/
/******************************************************************************/
//
//  LocalS3DO
//

void LocalS3DO::Draw() const
{
	glPushMatrix();
	glTranslatef(offset.x, offset.y, offset.z);

	if (!anim) {
		glCallList(displist);
	}
	else {
		glTranslatef(-anim->coords[0] / CORDDIV,
		              anim->coords[1] / CORDDIV,
		              anim->coords[2] / CORDDIV);
		if (anim->rot[1]) { glRotatef( anim->rot[1] / ANGDIV, 0.0f, 1.0f, 0.0f); }
		if (anim->rot[0]) { glRotatef( anim->rot[0] / ANGDIV, 1.0f, 0.0f, 0.0f); }
		if (anim->rot[2]) { glRotatef(-anim->rot[2] / ANGDIV, 0.0f, 0.0f, 1.0f); }
		if (anim->visible) {
			glCallList(displist);
		}
	}

	for (unsigned int i = 0; i < childs.size(); i++) {
		childs[i]->Draw();
	}

	glPopMatrix();
}


void LocalS3DO::DrawLOD(unsigned int lod) const
{
	const	unsigned int lodDispList = lodDispLists[lod];

	glPushMatrix();
	glTranslatef(offset.x, offset.y, offset.z);

	if (!anim) {
		glCallList(lodDispList);
	}
	else {
		glTranslatef(-anim->coords[0] / CORDDIV,
		              anim->coords[1] / CORDDIV,
		              anim->coords[2] / CORDDIV);
		if (anim->rot[1]) { glRotatef( anim->rot[1] / ANGDIV, 0.0f, 1.0f, 0.0f); }
		if (anim->rot[0]) { glRotatef( anim->rot[0] / ANGDIV, 1.0f, 0.0f, 0.0f); }
		if (anim->rot[2]) { glRotatef(-anim->rot[2] / ANGDIV, 0.0f, 0.0f, 1.0f); }
		if (anim->visible) {
			glCallList(lodDispList);
		}
	}

	for (unsigned int i = 0; i < childs.size(); i++) {
		childs[i]->DrawLOD(lod);
	}

	glPopMatrix();
}


void LocalS3DO::SetLODCount(unsigned int count)
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


void LocalS3DO::GetPiecePosIter(CMatrix44f* mat) const
{
	if (parent) {
		parent->GetPiecePosIter(mat);
	}

	mat->Translate(offset.x, offset.y, -offset.z);

	if (anim) {
		mat->Translate(-anim->coords[0] / CORDDIV,
		                anim->coords[1] / CORDDIV,
		               -anim->coords[2] / CORDDIV);
		if (anim->rot[1]) { mat->RotateY(anim->rot[1] * (PI / 32768)); }
		if (anim->rot[0]) { mat->RotateX(anim->rot[0] * (PI / 32768)); }
		if (anim->rot[2]) { mat->RotateZ(anim->rot[2] * (PI / 32768)); }
	}
}


void LocalS3DO::ApplyTransform() const
{
	if (parent) {
		parent->ApplyTransform();
	}
	
	glTranslatef(offset.x, offset.y, offset.z);

	if (anim) {
		glTranslatef(-anim->coords[0] / CORDDIV,
									anim->coords[1] / CORDDIV,
									anim->coords[2] / CORDDIV);
		if (anim->rot[1]) { glRotatef( anim->rot[1] / ANGDIV, 0.0f, 1.0f, 0.0f); }
		if (anim->rot[0]) { glRotatef( anim->rot[0] / ANGDIV, 1.0f, 0.0f, 0.0f); }
		if (anim->rot[2]) { glRotatef(-anim->rot[2] / ANGDIV, 0.0f, 0.0f, 1.0f); }
	}
}


/******************************************************************************/
/******************************************************************************/
