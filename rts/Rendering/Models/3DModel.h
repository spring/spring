/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _3DMODEL_H
#define _3DMODEL_H

#include <algorithm>
#include <array>
#include <vector>
#include <string>

#include "Lua/LuaObjectMaterial.h"
#include "Rendering/GL/VBO.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Matrix44f.h"
#include "System/type2.h"
#include "System/creg/creg_cond.h"


#define MAX_MODEL_OBJECTS  2048
#define NUM_MODEL_TEXTURES    2
#define NUM_MODEL_UVCHANNS    2

static constexpr float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static constexpr float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

enum ModelType {
	MODELTYPE_3DO    = 0,
	MODELTYPE_S3O    = 1,
	MODELTYPE_ASS    = 2, // Assimp
	MODELTYPE_OTHER  = 3  // count
};

struct CollisionVolume;
struct S3DModel;
struct S3DModelPiece;
struct LocalModel;
struct LocalModelPiece;




struct SVertexData {
	SVertexData() = default;
	SVertexData(const float3& p, const float3& n, const float3& s, const float3& t, const float2& uv0, const float2& uv1, uint32_t i) {
		pos = p;
		normal = n;
		sTangent = s;
		tTangent = t;
		texCoords[0] = uv0;
		texCoords[1] = uv1;
		pieceIndex = i;
	}

	float3 pos;
	float3 normal = UpVector;
	float3 sTangent;
	float3 tTangent;

	// TODO:
	//   with pieceIndex this struct is no longer 64 bytes in size which ATI's prefer
	//   support an arbitrary number of channels, would be easy but overkill (for now)
	float2 texCoords[NUM_MODEL_UVCHANNS];

	uint32_t pieceIndex = 0;
};



struct S3DModelPiecePart {
public:
	struct RenderData {
		float3 dir;
		size_t vboOffset = 0;
		size_t indexCount = 0;
	};

	static constexpr int SHATTER_MAX_PARTS  = 10;
	static constexpr int SHATTER_VARIATIONS =  2;

	std::vector<RenderData> renderData;
};



/**
 * S3DModel
 * A 3D model definition. Holds geometry (vertices/normals) and texture data as well as the piece tree.
 * The S3DModel is static and shouldn't change once created, instead a LocalModel is used by each agent.
 */

struct S3DModelPiece {
	S3DModelPiece() = default;

	// runs during global deinit, can not Clear() since that touches OpenGL
	virtual ~S3DModelPiece() { assert(shatterIndices.vboId == 0); }

	virtual void Clear() {
		name.clear();
		children.clear();

		for (S3DModelPiecePart& p: shatterParts) {
			p.renderData.clear();
		}

		parent = nullptr;
		colvol = {};

		bposeMatrix.LoadIdentity();
		bakedMatrix.LoadIdentity();

		offset = ZeroVector;
		goffset = ZeroVector;
		scales = OnesVector;

		mins = DEF_MIN_SIZE;
		maxs = DEF_MAX_SIZE;

		vboStartElem = 0;
		vboStartIndx = 0;

		shatterIndices.Release();
		// Release() does not virginize, be explicit here in case of reload
		shatterIndices = {};

		hasBakedMat = false;
		dummyPadding = false;
	}

	virtual float3 GetEmitPos() const;
	virtual float3 GetEmitDir() const;

	// internal use
	virtual unsigned int GetVertexCount() const = 0;
	virtual unsigned int GetVertexDrawIndexCount() const = 0;
	virtual const float3& GetVertexPos(const int) const = 0;
	virtual const float3& GetNormal(const int) const = 0;

	void SetGlobalOffset(const CMatrix44f& m) { goffset = (m.Mul(offset) + ((HasParent())? parent->goffset: ZeroVector)); }

	void BindShatterIndexBuffer() const { shatterIndices.Bind(GL_ELEMENT_ARRAY_BUFFER); }
	void UnbindShatterIndexBuffer() const { shatterIndices.Unbind(); }

	const VBO& GetShatterIndexBuffer() const { return shatterIndices; }

public:
	virtual const std::vector<SVertexData>& GetVertexElements() const = 0;
	virtual const std::vector<unsigned>& GetVertexIndices() const = 0;

public:
	void CreateShatterPieces();
	void Shatter(const S3DModel*, int, float, const float3&, const float3&, const CMatrix44f&) const;

	void SetBindPoseMatrix(const CMatrix44f& m) {
		bposeMatrix = m * ComposeTransform(offset, ZeroVector, scales);

		for (S3DModelPiece* c: children) {
			c->SetBindPoseMatrix(bposeMatrix);
		}
	}
	void SetBakedMatrix(const CMatrix44f& m) {
		bakedMatrix = m;
		hasBakedMat = !m.IsIdentity();
		assert(m.IsOrthoNormal());
	}

	CMatrix44f ComposeTransform(const float3& t, const float3& r, const float3& s) const {
		CMatrix44f m;

		// NOTE:
		//   ORDER MATTERS (T(baked + script) * R(baked) * R(script) * S(baked))
		//   translating + rotating + scaling is faster than matrix-multiplying
		//   m is identity so m.SetPos(t)==m.Translate(t) but with fewer instrs
		m.SetPos(t);

		if (hasBakedMat)
			m *= bakedMatrix;

		// default Spring rotation-order [YPR=Y,X,Z]
		m.RotateEulerYXZ(-r);
		m.Scale(s);
		return m;
	}

	void SetCollisionVolume(const CollisionVolume& cv) { colvol = cv; }
	const CollisionVolume* GetCollisionVolume() const { return &colvol; }
	      CollisionVolume* GetCollisionVolume()       { return &colvol; }

	bool HasParent() const { return (parent != nullptr); }
	bool HasGeometryData() const { return (GetVertexDrawIndexCount() >= 3); }

private:
	void CreateShatterPiece(int pieceNum);

public:
	std::string name;
	std::vector<S3DModelPiece*> children;
	std::array<S3DModelPiecePart, S3DModelPiecePart::SHATTER_VARIATIONS> shatterParts;

	S3DModelPiece* parent = nullptr;
	CollisionVolume colvol;

	CMatrix44f bposeMatrix;      /// bind-pose transform, including baked rots
	CMatrix44f bakedMatrix;      /// baked local-space rotations

	float3 offset;               /// local (piece-space) offset wrt. parent piece
	float3 goffset;              /// global (model-space) offset wrt. root piece
	float3 scales = OnesVector;  /// baked uniform scaling factors (assimp-only)

	float3 mins = DEF_MIN_SIZE;
	float3 maxs = DEF_MAX_SIZE;

	unsigned int vboStartElem = 0;
	unsigned int vboStartIndx = 0;

protected:
	VBO shatterIndices;

	bool hasBakedMat = false;
	bool dummyPadding = false;
};



struct S3DModel
{
	S3DModel()
		: id(-1)
		, numPieces(0)
		, textureType(-1)

		, type(MODELTYPE_OTHER)

		, radius(0.0f)
		, height(0.0f)

		, mins(DEF_MIN_SIZE)
		, maxs(DEF_MAX_SIZE)
		, relMidPos(ZeroVector)
	{
	}

	S3DModel(const S3DModel& m) = delete;
	S3DModel(S3DModel&& m) { *this = std::move(m); }
	~S3DModel() { DeleteBuffers(); }

	S3DModel& operator = (const S3DModel& m) = delete;
	S3DModel& operator = (S3DModel&& m) {
		name    = std::move(m.name   );
		texs[0] = std::move(m.texs[0]);
		texs[1] = std::move(m.texs[1]);

		id = m.id;
		numPieces = m.numPieces;
		textureType = m.textureType;

		vertexArray = m.vertexArray; m.vertexArray = 0;
		elemsBuffer = m.elemsBuffer; m.elemsBuffer = 0;
		indcsBuffer = m.indcsBuffer; m.indcsBuffer = 0;
		vboNumVerts = m.vboNumVerts;
		vboNumIndcs = m.vboNumIndcs;

		type = m.type;

		radius = m.radius;
		height = m.height;

		mins = m.mins;
		maxs = m.maxs;
		relMidPos = m.relMidPos;

		pieceObjects = std::move(m.pieceObjects);
		pieceMatrices = std::move(m.pieceMatrices);
		return *this;
	}

	S3DModelPiece* GetPiece(size_t i) const { assert(i < pieceObjects.size()); return pieceObjects[i]; }
	S3DModelPiece* GetRootPiece() const { return (GetPiece(0)); }

	void AddPiece(S3DModelPiece* p) { pieceObjects.push_back(p); }
	void DeletePieces() {
		assert(!pieceObjects.empty());

		// NOTE: actual piece memory is owned by parser pools
		pieceObjects.clear();
	}

	void Draw() const;
	void DrawPiece(const S3DModelPiece* omp) const;
	void DrawPieceRec(const S3DModelPiece* omp) const;

	void DeleteBuffers();
	void UploadBuffers();
	void EnableAttribs() const;
	void DisableAttribs() const;

	void BindVertexArray() const { glBindVertexArray(vertexArray); }
	void BindElemsBuffer() const { glBindBuffer(GL_ARRAY_BUFFER, elemsBuffer); }
	void BindIndcsBuffer() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indcsBuffer); }
	void UnbindVertexArray() const { glBindVertexArray(0); }
	void UnbindElemsBuffer() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }
	void UnbindIndcsBuffer() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

	// minor hack for piece-projectiles, saves a uniform
	// void SetPieceMatrixWeight(size_t i, float w) const { const_cast<CMatrix44f&>(pieceMatrices[i])[15] = w; }
	void SetPieceMatrices();
	void FlattenPieceTree(S3DModelPiece* root);
	void FlattenPieceTreeRec(S3DModelPiece* piece);

	// default values set by parsers; radius is also cached in WorldObject::drawRadius (used by projectiles)
	float CalcDrawRadius() const { return ((maxs - mins).Length() * 0.5f); }
	float CalcDrawHeight() const { return (maxs.y - mins.y); }
	float GetDrawRadius() const { return radius; }
	float GetDrawHeight() const { return height; }

	float3 CalcDrawMidPos() const { return ((maxs + mins) * 0.5f); }
	float3 GetDrawMidPos() const { return relMidPos; }

	const std::vector<CMatrix44f>& GetPieceMatrices() const { return pieceMatrices; }

	bool UploadedBuffers() const { return (vertexArray != 0); }

public:
	std::string name;
	std::string texs[NUM_MODEL_TEXTURES];

	// flattened tree; pieceObjects[0] is the root
	std::vector<S3DModelPiece*> pieceObjects;
	// static bind-pose matrices
	std::vector<CMatrix44f> pieceMatrices;

	int id;                     /// unsynced ID, starting with 1
	int numPieces;
	int textureType;            /// FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or ASSIMP)

	unsigned int vertexArray = 0;
	unsigned int elemsBuffer = 0;
	unsigned int indcsBuffer = 0;
	unsigned int vboNumVerts = 0;
	unsigned int vboNumIndcs = 0;

	ModelType type;

	float radius;
	float height;

	float3 mins;
	float3 maxs;
	float3 relMidPos;
};



/**
 * LocalModel
 * Instance of S3DModel. Container for the geometric properties & piece visibility status of the agent's instance of a 3d model.
 */

struct LocalModelPiece
{
	CR_DECLARE_STRUCT(LocalModelPiece)

	LocalModelPiece(): dirty(true) {}
	LocalModelPiece(const S3DModelPiece* piece);

	void AddChild(LocalModelPiece* c) { children.push_back(c); }
	void RemoveChild(LocalModelPiece* c) { children.erase(std::find(children.begin(), children.end(), c)); }
	void SetParent(LocalModelPiece* p) { parent = p; }

	void SetLModelPieceIndex(unsigned int idx) { lmodelPieceIndex = idx; }
	void SetScriptPieceIndex(unsigned int idx) { scriptPieceIndex = idx; }
	unsigned int GetLModelPieceIndex() const { return lmodelPieceIndex; }
	unsigned int GetScriptPieceIndex() const { return scriptPieceIndex; }


	// on-demand functions
	void UpdateChildMatricesRec(bool updateChildMatrices) const;
	void UpdateParentMatricesRec() const;

	CMatrix44f CalcPieceSpaceMatrixRaw(const float3& p, const float3& r, const float3& s) const { return (original->ComposeTransform(p, r, s)); }
	CMatrix44f CalcPieceSpaceMatrix(const float3& p, const float3& r, const float3& s) const {
		if (blockScriptAnims)
			return pieceSpaceMat;
		return (CalcPieceSpaceMatrixRaw(p, r, s));
	}

	// note: actually OBJECT_TO_WORLD but transform is the same
	float3 GetAbsolutePos() const { return (GetModelSpaceMatrix().GetPos() * WORLD_TO_OBJECT_SPACE); }

	bool GetEmitDirPos(float3& emitPos, float3& emitDir) const;


	void SetDirty();
	void SetPosOrRot(const float3& src, float3& dst); // anim-script only
	void SetPosition(const float3& p) { SetPosOrRot(p, pos); } // anim-script only
	void SetRotation(const float3& r) { SetPosOrRot(r, rot); } // anim-script only

	bool SetPieceSpaceMatrix(const CMatrix44f& mat) {
		if ((blockScriptAnims = (mat.GetX() != ZeroVector))) {
			pieceSpaceMat = mat;

			// neither of these are used outside of animation scripts, and
			// GetEulerAngles wants a matrix created by PYR rotation while
			// <rot> is YPR
			// pos = mat.GetPos();
			// rot = mat.GetEulerAnglesLftHand();
			return true;
		}

		return false;
	}

	const float3& GetPosition() const { return pos; }
	const float3& GetRotation() const { return rot; }
	const float3& GetDirection() const { return dir; }

	const CMatrix44f& GetPieceSpaceMatrix() const { if (dirty) UpdateParentMatricesRec(); return pieceSpaceMat; }
	const CMatrix44f& GetModelSpaceMatrix() const { if (dirty) UpdateParentMatricesRec(); return modelSpaceMat; }

	const CollisionVolume* GetCollisionVolume() const { return &colvol; }
	      CollisionVolume* GetCollisionVolume()       { return &colvol; }

private:
	float3 pos; // translation relative to parent LMP, *INITIALLY* equal to original->offset
	float3 rot; // orientation relative to parent LMP, in radians (updated by scripts)
	float3 dir; // cached copy of original->GetEmitDir()

	mutable CMatrix44f pieceSpaceMat; // transform relative to parent LMP (SYNCED), combines <pos> and <rot>
	mutable CMatrix44f modelSpaceMat; // transform relative to root LMP (SYNCED), chained pieceSpaceMat's

	CollisionVolume colvol;

	mutable bool dirty;

public:
	bool scriptSetVisible; // TODO: add (visibility) maxradius!
	bool blockScriptAnims; // if true, Set{Position,Rotation} are ignored for this piece

	unsigned int lmodelPieceIndex; // index of this piece into LocalModel::pieces
	unsigned int scriptPieceIndex; // index of this piece into UnitScript::pieces

	const S3DModelPiece* original;
	LocalModelPiece* parent;

	std::vector<LocalModelPiece*> children;
};


struct LocalModel
{
	CR_DECLARE_STRUCT(LocalModel)

	LocalModel() {}
	~LocalModel() { pieces.clear(); }


	bool HasPiece(unsigned int i) const { return (i < pieces.size()); }
	bool Initialized() const { return (!pieces.empty()); }

	const LocalModelPiece* GetPiece(unsigned int i)  const { assert(HasPiece(i)); return &pieces[i]; }
	      LocalModelPiece* GetPiece(unsigned int i)        { assert(HasPiece(i)); return &pieces[i]; }

	const LocalModelPiece* GetRoot() const { return (GetPiece(0)); }
	const CollisionVolume* GetBoundingVolume() const { return &boundingVolume; }

	const LuaObjectMaterialData* GetLuaMaterialData() const { return &luaMaterialData; }
	      LuaObjectMaterialData* GetLuaMaterialData()       { return &luaMaterialData; }

	const float3 GetRelMidPos() const { return (boundingVolume.GetOffsets()); }

	// raw forms, the piece-index must be valid
	const float3 GetRawPiecePos(int pieceIdx) const { return pieces[pieceIdx].GetAbsolutePos(); }
	const CMatrix44f& GetRawPieceMatrix(int pieceIdx) const { return pieces[pieceIdx].GetModelSpaceMatrix(); }
	// const CMatrix44f& GetRawPieceMatrix(int pieceIdx) const { return matrices[pieceIdx]; }
	const std::vector<CMatrix44f>& GetPieceMatrices() const { return matrices; }

	// used by all SolidObject's; accounts for piece movement
	float GetDrawRadius() const { return (boundingVolume.GetBoundingRadius()); }


	void Draw() const;
	void DrawPiece(const LocalModelPiece* lmp) const;

	void SetModel(const S3DModel* model, bool initialize = true);
	void SetLODCount(unsigned int lodCount) {
		assert(Initialized());
		luaMaterialData.SetLODCount(lodCount);
	}

	void UpdateBoundingVolume();
	void UpdatePieceMatrices() { UpdatePieceMatrices(pmuFrameNum + 1); }
	void UpdatePieceMatrices(unsigned int gsFrameNum);
	void UpdateVolumeAndMatrices(bool updateChildMatrices) {
		pieces[0].UpdateChildMatricesRec(updateChildMatrices);
		UpdateBoundingVolume();
		UpdatePieceMatrices();
	}

	void GetBoundingBoxVerts(std::array<float3, 8 + 2>& verts) const { GetBoundingBoxVerts(&verts[0]); }
	void GetBoundingBoxVerts(float3* verts) const {
		const float3 bbMins = GetRelMidPos() - boundingVolume.GetHScales();
		const float3 bbMaxs = GetRelMidPos() + boundingVolume.GetHScales();

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

public:
	std::vector<LocalModelPiece> pieces;
	// unsynced copies of pieces[i].modelSpaceMat
	std::vector<CMatrix44f> matrices;

private:
	// object-oriented box; accounts for piece movement
	CollisionVolume boundingVolume;

	// custom Lua-set material this model should be rendered with
	LuaObjectMaterialData luaMaterialData;

	// simframe at which unsynced piece-matrices were last updated
	unsigned int pmuFrameNum = -1u;
	// per-instance shallow copies of S3DModel::*
	unsigned int vertexArray = 0;
	unsigned int elemsBuffer = 0;
	unsigned int indcsBuffer = 0;
	unsigned int vboNumVerts = 0;
	unsigned int vboNumIndcs = 0;
};

#endif /* _3DMODEL_H */
