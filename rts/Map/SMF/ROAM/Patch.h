/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATCH_H
#define PATCH_H

#include "Rendering/GL/myGL.h"
#include "System/Rectangle.h"
#include "System/Vec2.h"
#include <vector>


class CSMFGroundDrawer;
class CCamera;


// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128

// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (12)

// How many TriTreeNodes should be allocated?
#define POOL_SIZE      (500000)
#define MAX_POOL_SIZE (8000000)


/**
 * TriTreeNode Struct
 * Store the triangle tree data, but no coordinates!
 */
struct TriTreeNode
{
	TriTreeNode()
		: LeftChild(NULL)
		, RightChild(NULL)
		, BaseNeighbor(NULL)
		, LeftNeighbor(NULL)
		, RightNeighbor(NULL)
	{}

	bool IsLeaf() const {
		// All non-leaf nodes have both children, so just check for one
		return (LeftChild == NULL);
	}

	bool IsBranch() const {
		// All non-leaf nodes have both children, so just check for one
		return !!RightChild;
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
	static void InitPools(const size_t newPoolSize = POOL_SIZE);
	static void FreePools();
	static void ResetAll();
	inline static CTriNodePool* GetPool();

public:
	CTriNodePool(const size_t& poolSize) {
		pool.resize(poolSize);
		m_NextTriNode = 0;
	}

	void Reset();
	TriTreeNode* AllocateTri();

	bool RunOutOfNodes() const {
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
	Patch();
	~Patch();
	void Init(CSMFGroundDrawer* drawer, int worldX, int worldZ); //FIXME move this into the ctor

	friend class CRoamMeshDrawer;
	friend class CPatchInViewChecker;

	void Reset();

	TriTreeNode* GetBaseLeft()  { return &m_BaseLeft;  }
	TriTreeNode* GetBaseRight() { return &m_BaseRight; }
	char IsDirty()     const { return m_isDirty; }
	bool IsVisible()   const { return m_isVisible; }
	int  GetTriCount() const { return indices.size() / 3; }

	void UpdateHeightMap(const SRectangle& rect = SRectangle(0,0,PATCH_SIZE,PATCH_SIZE));

	bool Tessellate(const float3& campos, int viewradius);
	void ComputeVariance();

	void GenerateIndices();
	void Upload();
	void Draw();
	void DrawBorder();
	void SetSquareTexture() const;

public:
	static void SwitchRenderMode(int mode = -1);

	//void UpdateVisibility(CCamera*& cam);
	static void UpdateVisibility(CCamera*& cam, std::vector<Patch>& patches, const int numPatchesX);

protected:
	void VBOUploadVertices();

private:
	// The recursive half of the Patch Class
	void Split(TriTreeNode* tri);
	void RecursTessellate(TriTreeNode* const tri, const int2 left, const int2 right, const int2 apex, const int node);
	void RecursRender(TriTreeNode* const tri, const int2 left, const int2 right, const int2 apex);
	float RecursComputeVariance(const int leftX, const int leftY, const float leftZ, const int rightX, const int rightY, const float rightZ, const int apexX, const int apexY, const float apexZ, const int node);

	void RecursBorderRender(CVertexArray* va, TriTreeNode* const& tri, const int2& left, const int2& right, const int2& apex, int i, bool left_);
	void GenerateBorderIndices(CVertexArray* va);
protected:
	static RenderMode renderMode;

	CSMFGroundDrawer* smfGroundDrawer;

	const float* m_HeightMap; //< Pointer to height map to use
	const float* heightData;

	std::vector<float> m_VarianceLeft;  //< Left variance tree
	std::vector<float> m_VarianceRight; //< Right variance tree
	float* m_CurrentVariance;  //< Which varience we are currently using. [Only valid during the Tessellate and ComputeVariance passes]

	bool m_isVisible; //< Is this patch visible in the current frame?
	bool m_isDirty; //< Does the Varience Tree need to be recalculated for this Patch?

	TriTreeNode m_BaseLeft;  //< Left base triangle tree node
	TriTreeNode m_BaseRight; //< Right base triangle tree node

	float varianceMaxLimit;
	float camDistLODFactor; //< defines the LOD falloff in camera distance

	int m_WorldX, m_WorldY; //< World coordinate offset of this patch.
	//float minHeight, maxHeight;

	std::vector<float> vertices; // Why yes, this IS a mind bogglingly wasteful thing to do: TODO: remove this for both the Displaylist and the VBO implementations (only really needed for vertexarrays)
	std::vector<unsigned int> indices;

	bool vboVerticesUploaded;

	GLuint triList;
	GLuint vertexBuffer;
	GLuint vertexIndexBuffer;
};

#endif
