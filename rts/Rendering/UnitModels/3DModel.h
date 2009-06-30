#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include "Matrix44f.h"
#include "Sim/Units/Unit.h"


const int MODELTYPE_3DO   = 0;
const int MODELTYPE_S3O   = 1;
const int MODELTYPE_OTHER = 2;

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;


struct S3DModelPiece {
	std::string name;
	std::vector<S3DModelPiece*> childs;

	unsigned int vertexCount;
	unsigned int displist;
	float3 offset;
	bool isEmpty;
	float maxx, maxy, maxz;
	float minx, miny, minz;

	int type;  //! MODELTYPE_3DO, MODELTYPE_S3O, MODELTYPE_OTHER

	// defaults to a box
	CollisionVolume* colvol;

	//todo: add float3 orientation;

	virtual ~S3DModelPiece();
	virtual const float3& GetVertexPos(const int& idx) const = 0;
	void DrawStatic() const;
};


struct S3DModel
{
	S3DModelPiece* rootobject;
	int numobjects;
	float radius;
	float height;
	std::string name;
	int farTextureNum;
	float maxx,maxy,maxz;
	float minx,miny,minz;
	float3 relMidPos;
	int type;        //! MODELTYPE_3DO, MODELTYPE_S3O, MODELTYPE_OTHER
	int textureType; // FIXME MAKE S3O ONLY      //0=3do, otherwise s3o
	std::string tex1;
	std::string tex2;
	inline void DrawStatic() const { rootobject->DrawStatic(); };
};


struct LocalModelPiece
{
	//todo: add (visibility) maxradius!

	float3 pos;
	float3 rot; //! in radian
	bool updated; //FIXME unused?
	bool visible;

	int type;  //! MODELTYPE_3DO, MODELTYPE_S3O, MODELTYPE_OTHER
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

//FIXME redundant struct!?
struct LocalModel
{
	LocalModel() : lodCount(0) {};
	~LocalModel();

	int type;  //! MODELTYPE_3DO, MODELTYPE_S3O, MODELTYPE_OTHER

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
	int GetRawPieceVertCount(int piecenum) const;
	void GetRawEmitDirPos(int piecenum, float3 &pos, float3 &dir) const;
};


#endif /* _3DMODEL_H */
