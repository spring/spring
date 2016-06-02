/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATCH_H
#define PATCH_H

#include "Rendering/GL/myGL.h"
#include "System/Rectangle.h"
#include "System/type2.h"
#include <vector>


class CSMFGroundDrawer;
class CCamera;


// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128

// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (12)

// How many TriTreeNodes should be allocated?
#define POOL_SIZE      (500000)


/**
 * TriTreeNode Struct
 * Store the triangle tree data, but no coordinates!
 */
struct TriTreeNode
{
	TriTreeNode()
		: LeftChild(nullptr)
		, RightChild(nullptr)
		, BaseNeighbor(nullptr)
		, LeftNeighbor(nullptr)
		, RightNeighbor(nullptr)
	{}

	bool IsLeaf() const {
		// All non-leaf nodes have both children, so just check for one
		return (LeftChild == nullptr);
	}

	bool IsBranch() const {
		// All non-leaf nodes have both children, so just check for one
		return (RightChild != nullptr);
	}

	TriTreeNode* LeftChild;
	TriTreeNode* RightChild;

	TriTreeNode* BaseNeighbor;
	TriTreeNode* LeftNeighbor;
	TriTreeNode* RightNeighbor;
};


/**
 * CTriNodePool class
 * Allocs a pool of TriTreeNodes, so we can reconstruct the whole tree w/o to dealloc the old nodes.
 * InitPools() creates for each worker thread its own pool to avoid locking.
 */
class CTriNodePool
{
public:
	static void InitPools(bool shadowPass, size_t newPoolSize = POOL_SIZE);
	static void FreePools(bool shadowPass);
	static void ResetAll(bool shadowPass);
	inline static CTriNodePool* GetPool(bool shadowPass);

public:
	CTriNodePool(const size_t poolSize);

	void Reset();
	void Allocate(TriTreeNode*& left, TriTreeNode*& right);

	bool OutOfNodes() const {
		return (m_NextTriNode >= pool.size());
	}

private:
	std::vector<TriTreeNode> pool;

	size_t m_NextTriNode; //< Index to next free TriTreeNode
};




/**
 * Patch Class
 * Store information needed at the Patch level
 */
class Patch
{
public:
	enum RenderMode {
		VBO = 1,
		DL  = 2,
		VA  = 3
	};

public:
	friend class CRoamMeshDrawer;
	friend class CPatchInViewChecker;

	Patch();
	~Patch();

	void Init(CSMFGroundDrawer* drawer, int worldX, int worldZ); //FIXME move this into the ctor
	void Reset();

	TriTreeNode* GetBaseLeft()  { return &baseLeft;  }
	TriTreeNode* GetBaseRight() { return &baseRight; }

	bool IsVisible(const CCamera*) const;
	char IsDirty() const { return isDirty; }
	int GetTriCount() const { return (indices.size() / 3); }

	void UpdateHeightMap(const SRectangle& rect = SRectangle(0, 0, PATCH_SIZE, PATCH_SIZE));

	bool Tessellate(const float3& campos, int viewradius, bool shadowPass);
	void ComputeVariance();

	void GenerateIndices();
	void Upload();
	void Draw();
	void DrawBorder();
	void SetSquareTexture() const;

public:
	static void SwitchRenderMode(int mode = -1);

	void UpdateVisibility(CCamera* cam);
	static void UpdateVisibility(CCamera* cam, std::vector<Patch>& patches, const int numPatchesX);

protected:
	void VBOUploadVertices();

private:
	// recursive functions
	void Split(TriTreeNode* tri);
	void RecursTessellate(TriTreeNode* tri, const int2 left, const int2 right, const int2 apex, const int node);
	void RecursRender(const TriTreeNode* tri, const int2 left, const int2 right, const int2 apex);

	float RecursComputeVariance(
		const   int2 left,
		const   int2 rght,
		const   int2 apex,
		const float3 hgts,
		const    int node
	);

	void RecursBorderRender(
		CVertexArray* va,
		const TriTreeNode* tri,
		const int2 left,
		const int2 rght,
		const int2 apex,
		int depth,
		bool leftChild
	);

	float GetHeight(int2 pos);

	void GenerateBorderIndices(CVertexArray* va);

private:
	static RenderMode renderMode;

	CSMFGroundDrawer* smfGroundDrawer;

	//< Which variance we are currently using. [Only valid during the Tessellate and ComputeVariance passes]
	float* currentVariance;
	CTriNodePool* currentPool;

	//< Does the Variance Tree need to be recalculated for this Patch?
	bool isDirty;
	bool vboVerticesUploaded;

	float varianceMaxLimit;
	float camDistLODFactor; //< defines the LOD falloff in camera distance

	//< World coordinate offsets of this patch.
	int2 coors;


	TriTreeNode baseLeft;  //< Left base triangle tree node
	TriTreeNode baseRight; //< Right base triangle tree node

	std::vector<float> varianceLeft;  //< Left variance tree
	std::vector<float> varianceRight; //< Right variance tree

	// TODO: remove for both the Displaylist and the VBO implementations (only really needed for VA's)
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

	//< frame on which this patch was last visible, per pass
	std::vector<unsigned int> lastDrawFrames;


	GLuint triList;
	GLuint vertexBuffer;
	GLuint vertexIndexBuffer;
};

#endif
