/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include "Matrix44f.h"



const int
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_OBJ   = 2,
	MODELTYPE_OTHER = 3;

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;


struct S3DModelPiece {
	std::string name;
	std::vector<S3DModelPiece*> childs;

	S3DModelPiece* parent;

	bool isEmpty;
	unsigned int displist;

	//! MODELTYPE_*
	int type;

	// defaults to a box
	CollisionVolume* colvol;

	// float3 dir;    // TODO?
	float3 mins;
	float3 maxs;
	float3 offset;    // wrt. parent
	float3 goffset;   // wrt. root

	virtual ~S3DModelPiece();
	virtual void DrawList() const = 0;
	virtual int GetVertexCount() const { return 0; }
	virtual int GetNormalCount() const { return 0; }
	virtual int GetTxCoorCount() const { return 0; }
	virtual void SetMinMaxExtends() {}
	virtual void SetVertexTangents() {}
	virtual const float3& GetVertexPos(int) const = 0;
	virtual void Shatter(float, int, int, const float3&, const float3&) const {}
	void DrawStatic() const;
};


struct S3DModel
{
	int id; //! unsynced ID, starting with 1

	int type;               //! MODELTYPE_*
	int textureType;        //! FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or OBJ)

	std::string name;
	std::string tex1;
	std::string tex2;

	int numobjects;
	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	S3DModelPiece* rootobject;

	inline void DrawStatic() const { rootobject->DrawStatic(); };
};


struct LocalModelPiece
{
	// TODO: add (visibility) maxradius!

	float3 pos;
	float3 rot; //! in radian
	bool updated; //FIXME unused?
	bool visible;

	//! MODELTYPE_*
	int type;
	std::string name;
	S3DModelPiece* original;
	LocalModelPiece* parent;
	std::vector<LocalModelPiece*> childs;

	// initially always a clone
	// of the original->colvol
	CollisionVolume* colvol;

	unsigned int displist;
	std::vector<unsigned int> lodDispLists;

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);
	void ApplyTransform() const;
	void GetPiecePosIter(CMatrix44f* mat) const;
	float3 GetPos() const;
	CMatrix44f GetMatrix() const;
	float3 GetDirection() const;
	bool GetEmitDirPos(float3 &pos, float3 &dir) const;
};

struct LocalModel
{
	LocalModel() : lodCount(0) {};
	~LocalModel();

	int type;  //! MODELTYPE_*

	std::vector<LocalModelPiece*> pieces;
	unsigned int lodCount;

	inline void Draw() const { pieces[0]->Draw(); };
	inline void DrawLOD(unsigned int lod) const { if (lod <= lodCount) pieces[0]->DrawLOD(lod);};
	void SetLODCount(unsigned int count);

	//! raw forms, the piecenum must be valid
	void ApplyRawPieceTransform(int piecenum) const;
	float3 GetRawPiecePos(int piecenum) const;
	CMatrix44f GetRawPieceMatrix(int piecenum) const;
	float3 GetRawPieceDirection(int piecenum) const;
	void GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const;
};


#endif /* _3DMODEL_H */
