/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <vector>
#include <string>

#include "Lua/LuaObjectMaterial.h"
#include "Rendering/GL/VBO.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Matrix44f.h"
#include "System/creg/creg_cond.h"

static const float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static const float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

enum ModelType {
	MODELTYPE_3DO   = 0,
	MODELTYPE_S3O   = 1,
	MODELTYPE_OBJ   = 2,
	MODELTYPE_ASS   = 3, // Model loaded by Assimp library
	MODELTYPE_OTHER = 4  // For future use. Still used in some parts of code.
};
enum AxisMappingType {
	AXIS_MAPPING_XYZ = 0,
	AXIS_MAPPING_ZXY = 1,
	AXIS_MAPPING_YZX = 2,
	AXIS_MAPPING_XZY = 3,
	AXIS_MAPPING_ZYX = 4,
	AXIS_MAPPING_YXZ = 5,
};

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;



/**
 * S3DModel
 * A 3D model definition. Holds geometry (vertices/normals) and texture data as well as the piece tree.
 * The S3DModel is static and shouldn't change once created, instead a LocalModel is used by each agent.
 */

struct S3DModelPiece {
	S3DModelPiece();
	virtual ~S3DModelPiece();

	virtual unsigned int CreateDrawForList() const;
	virtual void UploadGeometryVBOs() {}

	virtual unsigned int GetVertexCount() const { return 0; }
	virtual unsigned int GetNormalCount() const { return 0; }
	virtual unsigned int GetTxCoorCount() const { return 0; }

	virtual const float3& GetVertexPos(const int) const = 0;
	virtual const float3& GetNormal(const int) const = 0;

	virtual void Shatter(float, int, int, const float3&, const float3&) const {}

	CMatrix44f& ComposeRotation(CMatrix44f& m, const float3& r) const {
		// execute rotations in YPR order by default
		// note: translating + rotating is faster than
		// matrix-multiplying (but the branching hurts)
		switch (axisMapType) {
			case AXIS_MAPPING_XYZ: {
				if (r.y != 0.0f) { m.RotateY(r.y * rotAxisSigns.y); } // yaw
				if (r.x != 0.0f) { m.RotateX(r.x * rotAxisSigns.x); } // pitch
				if (r.z != 0.0f) { m.RotateZ(r.z * rotAxisSigns.z); } // roll
			} break;

			case AXIS_MAPPING_XZY: {
				if (r.y != 0.0f) { m.RotateZ(r.y * rotAxisSigns.y); } // yaw
				if (r.x != 0.0f) { m.RotateX(r.x * rotAxisSigns.x); } // pitch
				if (r.z != 0.0f) { m.RotateY(r.z * rotAxisSigns.z); } // roll
			} break;

			case AXIS_MAPPING_YXZ:
			case AXIS_MAPPING_YZX:
			case AXIS_MAPPING_ZXY:
			case AXIS_MAPPING_ZYX: {
				// FIXME: implement
			} break;
		}

		return m;
	}

	bool ComposeTransform(CMatrix44f& m, const float3& t, const float3& r, const float3& s) const {
		bool isIdentity = true;

		// NOTE: ORDER MATTERS (T(baked + script) * R(baked) * R(script) * S(baked))
		if (t != ZeroVector) { m = m.Translate(t);        isIdentity = false; }
		if (!hasIdentityRot) { m *= bakedRotMatrix;       isIdentity = false; }
		if (r != ZeroVector) { m = ComposeRotation(m, r); isIdentity = false; }
		if (s != OnesVector) { m = m.Scale(s);            isIdentity = false; }

		return isIdentity;
	}

	// draw piece and children statically (ie. without script-transforms)
	void DrawStatic() const;

	void SetCollisionVolume(const CollisionVolume& cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return &colvol; }
	      CollisionVolume* GetCollisionVolume()       { return &colvol; }

	unsigned int GetChildCount() const { return children.size(); }
	S3DModelPiece* GetChild(unsigned int i) const { return children[i]; }

	unsigned int GetDisplayListID() const { return dispListID; }
	void SetDisplayListID(unsigned int id) { dispListID = id; }

	bool HasGeometryData() const { return hasGeometryData; }
	bool HasIdentityRotation() const { return hasIdentityRot; }

	void SetHasGeometryData(bool b) { hasGeometryData = b; }
	void SetHasIdentityRotation(bool b) { hasIdentityRot = b; }

protected:
	virtual void DrawForList() const = 0;

public:
	std::string name;
	std::string parentName;

	std::vector<S3DModelPiece*> children;

	S3DModelPiece* parent;
	CollisionVolume colvol;

	AxisMappingType axisMapType;

	bool hasGeometryData;      /// if piece contains any geometry data
	bool hasIdentityRot;       /// if bakedRotMatrix is identity

	CMatrix44f bakedRotMatrix; /// baked local-space rotations (assimp-only)

	float3 offset;             /// local (piece-space) offset wrt. parent piece
	float3 goffset;            /// global (model-space) offset wrt. root piece
	float3 scales;             /// baked uniform scaling factors (assimp-only)
	float3 mins;
	float3 maxs;
	float3 rotAxisSigns;

protected:
	unsigned int dispListID;

	VBO vboIndices;
	VBO vboAttributes;
};



struct S3DModel
{
	S3DModel()
		: id(-1)
		, numPieces(0)
		, textureType(-1)

		, type(MODELTYPE_OTHER)

		, invertTexYAxis(false)
		, invertTexAlpha(false)

		, radius(0.0f)
		, height(0.0f)
		, drawRadius(0.0f)

		, mins(DEF_MIN_SIZE)
		, maxs(DEF_MAX_SIZE)
		, relMidPos(ZeroVector)

		, rootPiece(NULL)
	{
	}

	S3DModelPiece* GetRootPiece() const { return rootPiece; }

	void SetRootPiece(S3DModelPiece* p) { rootPiece = p; }
	void DrawStatic() const { rootPiece->DrawStatic(); }
	void DeletePieces(S3DModelPiece* piece);

public:
	std::string name;
	std::string tex1;
	std::string tex2;

	int id;                     /// unsynced ID, starting with 1
	int numPieces;
	int textureType;            /// FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or OBJ)

	ModelType type;

	bool invertTexYAxis;        /// Turn both textures upside down before use
	bool invertTexAlpha;        /// Invert teamcolor alpha channel in S3O texture 1

	float radius;
	float height;
	float drawRadius;

	float3 mins;
	float3 maxs;
	float3 relMidPos;

	S3DModelPiece* rootPiece;   /// The piece at the base of the model hierarchy
};



/**
 * LocalModel
 * Instance of S3DModel. Container for the geometric properties & piece visibility status of the agent's instance of a 3d model.
 */

struct LocalModelPiece
{
	CR_DECLARE_STRUCT(LocalModelPiece)

	LocalModelPiece() {}
	LocalModelPiece(const S3DModelPiece* piece);
	~LocalModelPiece() {}

	void AddChild(LocalModelPiece* c) { children.push_back(c); }
	void RemoveChild(LocalModelPiece* c) { children.erase(std::remove(children.begin(), children.end(), c), children.end()); }
	void SetParent(LocalModelPiece* p) { parent = p; }

	void SetLModelPieceIndex(unsigned int idx) { lmodelPieceIndex = idx; }
	void SetScriptPieceIndex(unsigned int idx) { scriptPieceIndex = idx; }
	unsigned int GetLModelPieceIndex() const { return lmodelPieceIndex; }
	unsigned int GetScriptPieceIndex() const { return scriptPieceIndex; }

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);

	bool UpdateMatrix();
	void UpdateMatricesRec(bool updateChildMatrices);

	// note: actually OBJECT_TO_WORLD but transform is the same
	float3 GetAbsolutePos() const { return (modelSpaceMat.GetPos() * WORLD_TO_OBJECT_SPACE); }

	bool GetEmitDirPos(float3& emitPos, float3& emitDir) const;

	void SetPosition(const float3& p) { pos = p; ++numUpdatesSynced; }
	void SetRotation(const float3& r) { rot = r; ++numUpdatesSynced; }
	void SetDirection(const float3& d) { dir = d; } // unused

	const float3& GetPosition() const { return pos; }
	const float3& GetRotation() const { return rot; }
	const float3& GetDirection() const { return dir; }

	const CMatrix44f& GetPieceSpaceMatrix() const { return pieceSpaceMat; }
	const CMatrix44f& GetModelSpaceMatrix() const { return modelSpaceMat; }

	const CollisionVolume* GetCollisionVolume() const { return &colvol; }
	      CollisionVolume* GetCollisionVolume()       { return &colvol; }

private:
	float3 pos; // translation relative to parent LMP, *INITIALLY* equal to original->offset
	float3 rot; // orientation relative to parent LMP, in radians (updated by scripts)
	float3 dir; // direction from vertex[0] to vertex[1] (constant!)

	CMatrix44f pieceSpaceMat; // transform relative to parent LMP (SYNCED), combines <pos> and <rot>
	CMatrix44f modelSpaceMat; // transform relative to root LMP (SYNCED)

	CollisionVolume colvol;

	unsigned numUpdatesSynced; // triggers UpdateMatrix (via UpdateMatricesRec) if != lastMatrixUpdate
	unsigned lastMatrixUpdate;

public:
	bool scriptSetVisible;  // TODO: add (visibility) maxradius!
	bool identityTransform; // true IFF pieceSpaceMat (!) equals identity

	unsigned int lmodelPieceIndex; // index of this piece into LocalModel::pieces
	unsigned int scriptPieceIndex; // index of this piece into UnitScript::pieces
	unsigned int dispListID;

	const S3DModelPiece* original;
	LocalModelPiece* parent;

	std::vector<LocalModelPiece*> children;
	std::vector<unsigned int> lodDispLists;
};


struct LocalModel
{
	CR_DECLARE_STRUCT(LocalModel)

	LocalModel() {
		dirtyPieces = 0;
		bvFrameTime = 0;
		lodCount = 0;
	}
	~LocalModel() {
		pieces.clear();
	}


	bool HasPiece(unsigned int i) const { return (i < pieces.size()); }
	bool Initialized() const { return (!pieces.empty()); }

	const LocalModelPiece* GetPiece(unsigned int i)  const { assert(HasPiece(i)); return &pieces[i]; }
	      LocalModelPiece* GetPiece(unsigned int i)        { assert(HasPiece(i)); return &pieces[i]; }

	const LocalModelPiece* GetRoot() const { return (GetPiece(0)); }
	const CollisionVolume* GetBoundingVolume() const { return &boundingVolume; }

	const LuaObjectMaterialData* GetLuaMaterialData() const { return &luaMaterialData; }
	      LuaObjectMaterialData* GetLuaMaterialData()       { return &luaMaterialData; }

	// raw forms, the piece-index must be valid
	const float3 GetRawPiecePos(int pieceIdx) const { return pieces[pieceIdx].GetAbsolutePos(); }
	const CMatrix44f& GetRawPieceMatrix(int pieceIdx) const { return pieces[pieceIdx].GetModelSpaceMatrix(); }


	void Draw() const {
		if (luaMaterialData.Enabled()) {
			DrawLOD(luaMaterialData.GetCurrentLOD());
		} else {
			DrawNoLOD();
		}
	}

	void Update(unsigned int frameNum) {
		if (dirtyPieces > 0)
			pieces[0].UpdateMatricesRec(false);

		// has its own fixed schedule independent of dirtyPieces
		if ((frameNum - bvFrameTime) >= 15)
			UpdateBoundingVolume(frameNum);

		dirtyPieces = 0;
	}


	void DrawPieces() const;
	void DrawPiecesLOD(unsigned int lod) const;

	void SetModel(const S3DModel* model);
	void SetLODCount(unsigned int count);
	void PieceUpdated(unsigned int pieceIdx) { dirtyPieces += 1; }
	void UpdateBoundingVolume(unsigned int frameNum);


	void GetBoundingBoxVerts(std::vector<float3>& verts) const {
		verts.resize(8 + 2); GetBoundingBoxVerts(&verts[0]);
	}

	void GetBoundingBoxVerts(float3* verts) const {
		// bottom
		verts[0] = float3(bbMins.x,  bbMins.y,  bbMins.z);
		verts[1] = float3(bbMaxs.x,  bbMins.y,  bbMins.z);
		verts[2] = float3(bbMaxs.x,  bbMins.y,  bbMaxs.z);
		verts[3] = float3(bbMins.x,  bbMins.y,  bbMaxs.z);
		// top
		verts[4] = float3(bbMins.x,  bbMaxs.y,  bbMins.z);
		verts[5] = float3(bbMaxs.x,  bbMaxs.y,  bbMins.z);
		verts[6] = float3(bbMaxs.x,  bbMaxs.y,  bbMaxs.z);
		verts[7] = float3(bbMins.x,  bbMaxs.y,  bbMaxs.z);
		// extrema
		verts[8] = bbMins;
		verts[9] = bbMaxs;
	}


private:
	LocalModelPiece* CreateLocalModelPieces(const S3DModelPiece* mpParent);

	void DrawNoLOD() const { DrawPieces(); }
	void DrawLOD(unsigned int lod) const {
		if (lod > lodCount)
			return;

		DrawPiecesLOD(lod);
	}

public:
	// increased by UnitScript whenever a piece is transformed
	unsigned int dirtyPieces;
	unsigned int bvFrameTime;
	unsigned int lodCount;

	std::vector<LocalModelPiece> pieces;

private:
	// object-oriented box; accounts for piece movement
	CollisionVolume boundingVolume;

	// custom Lua-set material this model should be rendered with
	LuaObjectMaterialData luaMaterialData;

	// bounding-box extrema (local space)
	float3 bbMins;
	float3 bbMaxs;
};

#endif /* _3DMODEL_H */
