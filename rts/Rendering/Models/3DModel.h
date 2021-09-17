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
#include "System/SafeUtil.h"
#include "System/creg/creg_cond.h"

constexpr int MAX_MODEL_OBJECTS = 2048;
constexpr int AVG_MODEL_PIECES = 16; // as it used to be
constexpr int NUM_MODEL_TEXTURES = 2;
constexpr int NUM_MODEL_UVCHANNS = 2;

static constexpr float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static constexpr float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

enum ModelType {
	MODELTYPE_3DO    = 0,
	MODELTYPE_S3O    = 1,
	MODELTYPE_ASS    = 2, // Assimp
	MODELTYPE_CNT    = 3  // count
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
		size_t vboOffset;
		size_t indexCount;
	};

	static const int SHATTER_MAX_PARTS  = 10;
	static const int SHATTER_VARIATIONS = 2;

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
	virtual ~S3DModelPiece() { assert(vboShatterIndices.GetIdRaw() == 0); }

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

		DeleteDispList();
		DeleteBuffers();

		hasBakedMat = false;
	}

	virtual float3 GetEmitPos() const;
	virtual float3 GetEmitDir() const;

	// internal use
	uint32_t GetVertexCount() const { return vertices.size();  }
	uint32_t GetVertexDrawIndexCount() const { return indices.size(); }
	virtual const float3& GetVertexPos(const int) const = 0;
	virtual const float3& GetNormal(const int) const = 0;

	virtual void PostProcessGeometry();
	void UploadToVBO();

	void MeshOptimize();

	void BindVertexAttribVBOs() const;
	void UnbindVertexAttribVBOs() const;

	void BindIndexVBO() const;
	void UnbindIndexVBO() const;

	void DrawElements(GLuint prim = GL_TRIANGLES) const;

	void BindShatterIndexVBO() const { vboShatterIndices.Bind(GL_ELEMENT_ARRAY_BUFFER); }
	void UnbindShatterIndexVBO() const { vboShatterIndices.Unbind(); }

	const VBO& GetShatterIndexVBO() const { return vboShatterIndices; }

protected:
	virtual void DrawForList() const = 0;
public:
	void DrawStatic() const;
	void CreateDispList();
	void DeleteDispList();
	void DeleteBuffers() {
		vboShatterIndices = {};
	}

	uint32_t GetDisplayListID() const { return dispListID; }

	void CreateShatterPieces();
	void Shatter(float, int, int, int, const float3, const float3, const CMatrix44f&) const;

	void SetPieceMatrix(const CMatrix44f& m) {
		bposeMatrix = m * ComposeTransform(offset, ZeroVector, scales);

		for (S3DModelPiece* c: children) {
			c->SetPieceMatrix(bposeMatrix);
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

	bool HasGeometryData() const { return (GetVertexDrawIndexCount() >= 3); }
	void SetParentModel(S3DModel* model_) { model = model_; }

	const std::vector<SVertexData>& GetVerticesVec() const { return vertices; };
	const std::vector<uint32_t>& GetIndicesVec() const { return indices; };
private:
	void CreateShatterPiecesVariation(const int num);

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

	uint32_t indxStart = 0u; //global VBO offset, size data
	uint32_t indxCount = 0u;
protected:
	uint32_t vboIndxStart = 0u;
	uint32_t vboVertStart = 0u;

	std::vector<SVertexData> vertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> indicesVBO; //used only to upload to VBO with shifted indices

	VBO vboShatterIndices;

	S3DModel* model;

	uint32_t dispListID;

	bool hasBakedMat;
public:
	friend class CAssParser;
};



struct S3DModel
{
	S3DModel()
		: id(-1)
		, numPieces(0)
		, textureType(-1)

		, vertVBO(nullptr)
		, indxVBO(nullptr)

		, indxStart(0u)
		, indxCount(0u)

		, curVertStartIndx(0u)
		, curIndxStartIndx(0u)

		, type(MODELTYPE_CNT)

		, radius(0.0f)
		, height(0.0f)

		, mins(DEF_MIN_SIZE)
		, maxs(DEF_MAX_SIZE)
		, relMidPos(ZeroVector)
	{

	}

	S3DModel(const S3DModel& m) = delete;
	S3DModel(S3DModel&& m) { *this = std::move(m); }

	S3DModel& operator = (const S3DModel& m) = delete;
	S3DModel& operator = (S3DModel&& m) noexcept {
		name    = std::move(m.name   );
		texs[0] = std::move(m.texs[0]);
		texs[1] = std::move(m.texs[1]);

		id = m.id;
		numPieces = m.numPieces;
		textureType = m.textureType;

		type = m.type;

		radius = m.radius;
		height = m.height;

		mins = m.mins;
		maxs = m.maxs;
		relMidPos = m.relMidPos;

		vertVBO = std::move(m.vertVBO);
		indxVBO = std::move(m.indxVBO);

		indxStart = m.indxStart;
		indxCount = m.indxCount;

		curVertStartIndx = m.curVertStartIndx;
		curIndxStartIndx = m.curIndxStartIndx;

		pieceObjects = std::move(m.pieceObjects);

		for (auto po : pieceObjects)
			po->SetParentModel(this);

		return *this;
	}

	S3DModelPiece* GetPiece(size_t i) const { assert(i < pieceObjects.size()); return pieceObjects[i]; }
	S3DModelPiece* GetRootPiece() const { return (GetPiece(0)); }

	void AddPiece(S3DModelPiece* p) { pieceObjects.push_back(p); }
	void DrawStatic() const {
		// draw pieces in their static bind-pose (ie. without script-transforms)
		for (const S3DModelPiece* pieceObj: pieceObjects) {
			pieceObj->DrawStatic();
		}
	}

	void CreateVBOs();

	void BindVertexAttribs() const;
	void UnbindVertexAttribs() const;

	void BindIndexVBO() const;
	void UnbindIndexVBO() const;

	void BindVertexVBO() const;
	void UnbindVertexVBO() const;

	void DrawElements(GLenum prim, uint32_t vboIndxStart, uint32_t vboIndxEnd) const;

	void UploadToVBO(const std::vector<SVertexData>& vertices, const std::vector<uint32_t>& indices, const uint32_t vertStart, const uint32_t indxStart) const;

	void SetPieceMatrices() { pieceObjects[0]->SetPieceMatrix(CMatrix44f()); }
	void DeletePieces() {
		assert(!pieceObjects.empty());

		// NOTE: actual piece memory is owned by parser pools
		pieceObjects.clear();
	}
	void FlattenPieceTree(S3DModelPiece* root) {
		assert(root != nullptr);

		pieceObjects.clear();
		pieceObjects.reserve(numPieces);

		std::vector<S3DModelPiece*> stack = {root};

		while (!stack.empty()) {
			S3DModelPiece* p = stack.back();

			stack.pop_back();
			pieceObjects.push_back(p);

			// add children in reverse for the correct DF traversal order
			for (size_t n = 0; n < p->children.size(); n++) {
				stack.push_back(p->children[p->children.size() - n - 1]);
			}
		}
	}

	// default values set by parsers; radius is also cached in WorldObject::drawRadius (used by projectiles)
	float CalcDrawRadius() const { return ((maxs - mins).Length() * 0.5f); }
	float CalcDrawHeight() const { return (maxs.y - mins.y); }
	float GetDrawRadius() const { return radius; }
	float GetDrawHeight() const { return height; }

	float3 CalcDrawMidPos() const { return ((maxs + mins) * 0.5f); }
	float3 GetDrawMidPos() const { return relMidPos; }

public:
	std::string name;
	std::string texs[NUM_MODEL_TEXTURES];

	// flattened tree; pieceObjects[0] is the root
	std::vector<S3DModelPiece*> pieceObjects;

	int id;                     /// unsynced ID, starting with 1
	int numPieces;
	int textureType;            /// FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or ASSIMP)

	std::unique_ptr<VBO> vertVBO;
	std::unique_ptr<VBO> indxVBO;

	uint32_t indxStart; //global VBO offset, size data
	uint32_t indxCount;

	uint32_t curVertStartIndx;
	uint32_t curIndxStartIndx;

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

	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void SetLODCount(unsigned int count);


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
	unsigned int dispListID;

	const S3DModelPiece* original;
	LocalModelPiece* parent;

	std::vector<LocalModelPiece*> children;
	std::vector<unsigned int> lodDispLists;
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

	// used by all SolidObject's; accounts for piece movement
	float GetDrawRadius() const { return (boundingVolume.GetBoundingRadius()); }


	void Draw() const {
		if (!luaMaterialData.Enabled()) {
			DrawPieces();
			return;
		}

		DrawPiecesLOD(luaMaterialData.GetCurrentLOD());
	}

	void SetModel(const S3DModel* model, bool initialize = true);
	void SetLODCount(unsigned int lodCount);
	void UpdateBoundingVolume();

	void GetBoundingBoxVerts(std::vector<float3>& verts) const {
		verts.resize(8 + 2); GetBoundingBoxVerts(&verts[0]);
	}

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

	void DrawPieces() const;
	void DrawPiecesLOD(unsigned int lod) const;

public:
	std::vector<LocalModelPiece> pieces;

private:
	// object-oriented box; accounts for piece movement
	CollisionVolume boundingVolume;

	// custom Lua-set material this model should be rendered with
	LuaObjectMaterialData luaMaterialData;
};

#endif /* _3DMODEL_H */
