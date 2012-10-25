/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include "System/Matrix44f.h"

enum ModelType {
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_OBJ   = 2,
	MODELTYPE_ASS   = 3, // Model loaded by Assimp library
	MODELTYPE_OTHER = 4  // For future use. Still used in some parts of code.
};

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;
struct aiScene;

typedef std::map<std::string, S3DModelPiece*> ModelPieceMap;


/**
 * S3DModel
 * A 3D model definition. Holds the vertices data, texture data, piece tree & default geometric data of those.
 * The S3DModel is static and shouldn't change once created, instead LocalModel is used by each agent.
 */

struct S3DModelPiece {
	S3DModelPiece()
		: model(NULL)
		, parent(NULL)
		, colvol(NULL)
		, isEmpty(true)
		, dispListID(0)
		, type(MODELTYPE_OTHER)
	{
	}

	virtual ~S3DModelPiece();
	virtual void DrawForList() const = 0;
	virtual int GetVertexCount() const { return 0; }
	virtual int GetNormalCount() const { return 0; }
	virtual int GetTxCoorCount() const { return 0; }
	virtual void SetMinMaxExtends() {}
	virtual void SetVertexTangents() {}
	virtual const float3& GetVertexPos(const int) const = 0;
	virtual const float3& GetNormal(const int) const = 0;
	virtual float3 GetPosOffset() const { return ZeroVector; }
	virtual void Shatter(float, int, int, const float3&, const float3&) const {}
	void DrawStatic() const;

	void SetCollisionVolume(CollisionVolume* cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return colvol; }
	      CollisionVolume* GetCollisionVolume()       { return colvol; }

	unsigned int GetChildCount() const { return childs.size(); }
	S3DModelPiece* GetChild(unsigned int i) const { return childs[i]; }

public:
	std::string name;
	std::string parentName;
	std::vector<S3DModelPiece*> childs;

	S3DModel* model;
	S3DModelPiece* parent;
	CollisionVolume* colvol;

	bool isEmpty;
	unsigned int dispListID;

	ModelType type;

	float3 mins;
	float3 maxs;
	float3 offset;    ///< @see parent
	float3 goffset;   ///< @see root
	float3 rot;
	float3 scale;
};


struct S3DModel
{
	S3DModel()
		: tex1("default.png")
		, tex2("")
		, id(-1)
		, type(MODELTYPE_OTHER)
		, textureType(-1)
		, flipTexY(false)
		, invertTexAlpha(false)
		, radius(0.0f)
		, height(0.0f)
		, mins(10000.0f, 10000.0f, 10000.0f)
		, maxs(-10000.0f, -10000.0f, -10000.0f)
		, relMidPos(ZeroVector)
		, numPieces(0)
		, rootPiece(NULL)
	{
	}

	S3DModelPiece* GetRootPiece() const { return rootPiece; }
	void SetRootPiece(S3DModelPiece* p) { rootPiece = p; }
	void DrawStatic() const { rootPiece->DrawStatic(); }
	S3DModelPiece* FindPiece(const std::string& name) const;

public:
	std::string name;
	std::string tex1;
	std::string tex2;

	int id;                 // unsynced ID, starting with 1
	ModelType type;
	int textureType;        // FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or OBJ)
	bool flipTexY;          // Turn both textures upside down before use
	bool invertTexAlpha;    // Invert teamcolor alpha channel in S3O texture 1

	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	int numPieces;
	S3DModelPiece* rootPiece;   // The piece at the base of the model hierarchy
	ModelPieceMap pieces;       // Lookup table for pieces by name
};



/**
 * LocalModel
 * Instance of S3DModel. Container for the geometric properties & piece visibility status of the agent's instance of a 3d model.
 */

struct LocalModelPiece
{
	LocalModelPiece(const S3DModelPiece* piece);
	~LocalModelPiece();

	void AddChild(LocalModelPiece* c) { childs.push_back(c); }
	void SetParent(LocalModelPiece* p) { parent = p; }

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);

	void UpdateMatricesRec();

	bool GetEmitDirPos(float3& pos, float3& dir) const;
	float3 GetAbsolutePos() const;

	void SetPosition(const float3& p) { pos = p; ++numUpdatesSynced; }
	void SetRotation(const float3& r) { rot = r; ++numUpdatesSynced; }
	void SetDirection(const float3& d) { dir = d; } // unused

	const float3& GetPosition() const { return pos; }
	const float3& GetRotation() const { return rot; }
	const float3& GetDirection() const { return dir; }

	const CMatrix44f& GetPieceSpaceMatrix() const { return pieceSpaceMat; }
	const CMatrix44f& GetModelSpaceMatrix() const { return modelSpaceMat; }

	const CollisionVolume* GetCollisionVolume() const { return colvol; }
	      CollisionVolume* GetCollisionVolume()       { return colvol; }

private:
	float3 pos; // translation relative to parent LMP
	float3 rot; // orientation relative to parent LMP, in radians
	float3 dir; // direction from vertex[0] to vertex[1] (constant!)

	CMatrix44f pieceSpaceMat; // transform relative to parent LMP (SYNCED), combines <pos> and <rot>
	CMatrix44f modelSpaceMat; // transform relative to root LMP (SYNCED)

	CollisionVolume* colvol;

	unsigned numUpdatesSynced;
	unsigned lastMatrixUpdate;

public:
	bool scriptSetVisible;  // TODO: add (visibility) maxradius!
	bool identityTransform; // true IFF pieceSpaceMat (!) equals identity

	unsigned int dispListID;

	const S3DModelPiece* original;
	const LocalModelPiece* parent;

	std::vector<LocalModelPiece*> childs;
	std::vector<unsigned int> lodDispLists;
};


struct LocalModel
{
	LocalModel(const S3DModel* model)
		: original(model)
		, type(model->type)
		, lodCount(0)
		, dirtyPieces(model->numPieces)
	{
		assert(model->numPieces >= 1);
		pieces.reserve(model->numPieces);
		CreateLocalModelPieces(model->GetRootPiece());
		assert(pieces.size() == model->numPieces);
	}

	~LocalModel()
	{
		// delete the local piece copies
		for (std::vector<LocalModelPiece*>::iterator pi = pieces.begin(); pi != pieces.end(); ++pi) {
			delete *pi;
		}
		pieces.clear();
	}

	LocalModelPiece* GetPiece(unsigned int i) const { return pieces[i]; }
	LocalModelPiece* GetRoot() const { return GetPiece(0); }

	void Draw() const { DrawPieces(); }
	void DrawLOD(unsigned int lod) const {
		if (lod > lodCount)
			return;

		DrawPiecesLOD(lod);
	}

	void UpdatePieceMatrices() {
		if (dirtyPieces > 0) {
			pieces[0]->UpdateMatricesRec();
		}
		dirtyPieces = 0;
	}



	void DrawPieces() const;
	void DrawPiecesLOD(unsigned int lod) const;

	void SetLODCount(unsigned int count);
	void PieceUpdated(unsigned int pieceIdx) { dirtyPieces += 1; }

	void ReloadDisplayLists();

	// raw forms, the piece-index must be valid
	// NOTE:
	//   GetRawPieceDirection is only useful for special pieces (used for emit-sfx)
	//   it returns a direction in piece-space, NOT model-space as the "Raw" suggests
	void ApplyRawPieceTransformUnsynced(int pieceIdx) const;
	void GetRawEmitDirPos(int pieceIdx, float3& pos, float3& dir) const { pieces[pieceIdx]->GetEmitDirPos(pos, dir); }
	float3 GetRawPiecePos(int pieceIdx) const { return pieces[pieceIdx]->GetAbsolutePos(); }
	float3 GetRawPieceDirection(int pieceIdx) const { return pieces[pieceIdx]->GetDirection(); }
	CMatrix44f GetRawPieceMatrix(int pieceIdx) const { return pieces[pieceIdx]->GetModelSpaceMatrix(); }

private:
	LocalModelPiece* CreateLocalModelPieces(const S3DModelPiece* mpParent, size_t pieceNum = 0);

public:
	const S3DModel* original;

	ModelType type;

	// increased by UnitScript whenever a piece is transformed
	unsigned int dirtyPieces;
	unsigned int lodCount;

	std::vector<LocalModelPiece*> pieces;
};

#endif /* _3DMODEL_H */
