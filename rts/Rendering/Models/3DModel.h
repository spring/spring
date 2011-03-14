/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include "System/Matrix44f.h"


const int
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_OBJ   = 2,
	MODELTYPE_ASS	= 3, // Model loaded by Assimp library
	MODELTYPE_OTHER	= 4; // For future use. Still used in some parts of code.

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;
struct aiScene;

typedef std::map<std::string, S3DModelPiece*> ModelPieceMap;

struct S3DModelPiece {
	S3DModelPiece(): type(-1) {
		parent = NULL;
		colvol = NULL;

		isEmpty = true;
		dispListID = 0;
	}

	virtual ~S3DModelPiece();
	virtual void DrawList() const = 0;
	virtual int GetVertexCount() const { return 0; }
	virtual int GetNormalCount() const { return 0; }
	virtual int GetTxCoorCount() const { return 0; }
	virtual void SetMinMaxExtends() {}
	virtual void SetVertexTangents() {}
	virtual const float3& GetVertexPos(int) const = 0;
	virtual float3 GetPosOffset() const { return ZeroVector; }
	virtual void Shatter(float, int, int, const float3&, const float3&) const {}
	void DrawStatic() const;

	void SetCollisionVolume(CollisionVolume* cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return colvol; }
	      CollisionVolume* GetCollisionVolume()       { return colvol; }

	unsigned int GetChildCount() const { return childs.size(); }
	S3DModelPiece* GetChild(unsigned int i) { return childs[i]; }


	std::string name;
	std::string parentName;
	std::vector<S3DModelPiece*> childs;

    S3DModel* model;
	S3DModelPiece* parent;
	CollisionVolume* colvol;

	bool isEmpty;
	unsigned int dispListID;

	//! MODELTYPE_*
	int type;

	float3 mins;
	float3 maxs;
	float3 offset;    // wrt. parent
	float3 goffset;   // wrt. root
	float3 rot;
	float3 scale;
};


struct S3DModel
{
	S3DModel(): id(-1), type(-1), textureType(-1) {
        height = 0.0f;
        radius = 0.0f;
        relMidPos = float3(0.0f, 0.0f, 0.0f);
        mins = float3(10000.0f, 10000.0f, 10000.0f);
        maxs = float3(-10000.0f, -10000.0f, -10000.0f);
        tex1 = "default.png";
        tex2 = "";
        flipTexY = false;
        invertTexAlpha = false;
        numPieces = 0;
        rootPiece = NULL;
        scene = NULL;
	}

	S3DModelPiece* GetRootPiece() { return rootPiece; }
	void SetRootPiece(S3DModelPiece* p) { rootPiece = p; }
	void DrawStatic() const { rootPiece->DrawStatic(); }
	S3DModelPiece* FindPiece( std::string name );

	std::string name;
	std::string tex1;
	std::string tex2;

	int id;                 //! unsynced ID, starting with 1
	int type;               //! MODELTYPE_*
	int textureType;        //! FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or OBJ)
	bool flipTexY;			//! Turn both textures upside down before use
	bool invertTexAlpha;	//! Invert teamcolor alpha channel in S3O texture 1

	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	int numPieces;
    S3DModelPiece* rootPiece;   //! The piece at the base of the model hierarchy
    ModelPieceMap pieces;       //! Lookup table for pieces by name
    const aiScene* scene;       //! Assimp scene containing all loaded model data. NULL for S30/3DO.
};



struct LocalModelPiece
{
	LocalModelPiece() {
		parent     = NULL;
		colvol     = NULL;
		original   = NULL;
		dispListID = 0;
		visible    = false;
	}
	void Init(S3DModelPiece* piece) {
		original   =  piece;
		dispListID =  piece->dispListID;
		visible    = !piece->isEmpty;
		pos        =  piece->offset;

		childs.reserve(piece->childs.size());
	}

	void AddChild(LocalModelPiece* c) { childs.push_back(c); }
	void SetParent(LocalModelPiece* p) { parent = p; }

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);
	void ApplyTransform() const;
	void GetPiecePosIter(CMatrix44f* mat) const;
	float3 GetPos() const;
	float3 GetDirection() const;
	bool GetEmitDirPos(float3& pos, float3& dir) const;
	CMatrix44f GetMatrix() const;

	void SetCollisionVolume(CollisionVolume* cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return colvol; }
	      CollisionVolume* GetCollisionVolume()       { return colvol; }


	float3 pos;
	float3 rot; //! in radian

	// TODO: add (visibility) maxradius!
	bool visible;

	CollisionVolume* colvol;
	S3DModelPiece* original;

	LocalModelPiece* parent;
	std::vector<LocalModelPiece*> childs;

	unsigned int dispListID;
	std::vector<unsigned int> lodDispLists;
};

struct LocalModel
{
	LocalModel(const S3DModel* model) : type(-1), lodCount(0) {
		type = model->type;
		pieces.reserve(model->numPieces);

		assert(model->numPieces >= 1);

		for (unsigned int i = 0; i < model->numPieces; i++) {
			pieces.push_back(new LocalModelPiece());
		}
	}

	~LocalModel();
	void CreatePieces(S3DModelPiece*, unsigned int*);

	LocalModelPiece* GetPiece(unsigned int i) { return pieces[i]; }
	LocalModelPiece* GetRoot() { return GetPiece(0); }

	void Draw() const { pieces[0]->Draw(); }
	void DrawLOD(unsigned int lod) const { if (lod <= lodCount) pieces[0]->DrawLOD(lod); }
	void SetLODCount(unsigned int count);

	//! raw forms, the piecenum must be valid
	void ApplyRawPieceTransform(int piecenum) const;
	float3 GetRawPiecePos(int piecenum) const;
	CMatrix44f GetRawPieceMatrix(int piecenum) const;
	float3 GetRawPieceDirection(int piecenum) const;
	void GetRawEmitDirPos(int piecenum, float3& pos, float3& dir) const;


	int type;  //! MODELTYPE_*
	unsigned int lodCount;

	std::vector<LocalModelPiece*> pieces;
};


#endif /* _3DMODEL_H */
