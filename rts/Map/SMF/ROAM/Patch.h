/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATCH_H
#define PATCH_H

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Game/Camera.h"
#include "System/Rectangle.h"
#include "System/type2.h"

#include <array>
#include <vector>


class CSMFGroundDrawer;
class CCamera;


// how many heightmap pixels a patch consists of
#define PATCH_SIZE 128

// depth of variance tree; should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (12)

// how many TriTreeNodes should be reserved per pool
// (2M is a reasonable baseline for most large maps)
// if a 32bit TriTreeNode struct is 28 bytes, the total ram usage of LOAM is 2M*28*2 = 104 MB
#define NEW_POOL_SIZE (1 << 21)
// debug (simulates fast pool exhaustion)
// #define NEW_POOL_SIZE (1 << 2)

class Patch; //declare it so that tritreenode can store its parent
// stores the triangle-tree structure, but no coordinates
struct TriTreeNode
{
	static TriTreeNode dummyNode;

	// all non-leaf nodes have both children, so just check for one
	bool IsLeaf() const { assert(!IsDummy()); return (LeftChild == &dummyNode); }
	bool IsBranch() const { assert(!IsDummy()); return (LeftChild != &dummyNode); }
	bool IsDummy() const { return (this == &dummyNode); }

	void Reset() {
		 LeftChild = &dummyNode;
		RightChild = &dummyNode;

		 BaseNeighbor = &dummyNode;
		 LeftNeighbor = &dummyNode;
		RightNeighbor = &dummyNode;
	}

	TriTreeNode*  LeftChild = &dummyNode;
	TriTreeNode* RightChild = &dummyNode;

	TriTreeNode*  BaseNeighbor = &dummyNode;
	TriTreeNode*  LeftNeighbor = &dummyNode;
	TriTreeNode* RightNeighbor = &dummyNode;

	Patch* parentPatch = NULL; //triangles know their parent patch so they know of a neighbour's Split() func caused changes to them
};



// maintains a pool of TriTreeNodes, so we can (re)construct triangle-trees
// without dynamically (de)allocating nodes (note that InitPools() actually
// creates a pool for each worker thread to avoid locking)
class CTriNodePool
{
public:
	static void InitPools(bool shadowPass, size_t newPoolSize = NEW_POOL_SIZE);
	static void ResetAll(bool shadowPass);

	inline static CTriNodePool* GetPool(bool shadowPass);

public:
	void Resize(size_t poolSize);
	void Reset() { nextTriNodeIdx = 0; }
	bool Allocate(TriTreeNode*& left, TriTreeNode*& right);

	bool ValidNode(const TriTreeNode* n) const { return (n >= &tris.front() && n <= &tris.back()); }
	bool OutOfNodes() const { return (nextTriNodeIdx >= tris.size()); }

	size_t getPoolSize() { return tris.size(); }
	size_t getNextTriNodeIdx() { return nextTriNodeIdx; }
private:
	std::vector<TriTreeNode> tris;

	// index of next free TriTreeNode
	size_t nextTriNodeIdx = 0;
};



// stores information needed at the Patch level
class Patch
{
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

	void UpdateHeightMap(const SRectangle& rect = SRectangle(0, 0, PATCH_SIZE, PATCH_SIZE));

	float3 lastCameraPosition ; //the last camera position this patch was tesselated from

	//this specifies the manhattan distance from the camera during the last tesselation
	//note that this can only become lower, as we can only increase tesselation levels while maintaining no cracks
	float camDistanceLastTesselation;

	// create an approximate mesh

	bool Tessellate(const float3& camPos, int viewRadius, bool shadowPass);
	// compute the variance tree for each of the binary triangles in this patch
	void ComputeVariance();

	void GenerateIndices();
	void UploadIndices();
	void GenerateBorderVertices();

	void Draw();
	void DrawBorder();
	void SetSquareTexture() const;

public:
	static void UpdateVisibility(CCamera* cam, std::vector<Patch>& patches, const int numPatchesX);

protected:
	void UploadVertices();
	void UploadBorderVertices();

private:
	// split a single triangle and link it into the mesh; will correctly force-split diamonds
	bool Split(TriTreeNode* tri);
	// tessellate patch; will continue to split until the variance metric is met
	void RecursTessellate(TriTreeNode* tri, const int2 left, const int2 right, const int2 apex, const int treeIdx, const int curNodeIdx);
	void RecursGenIndices(const TriTreeNode* tri, const int2 left, const int2 right, const int2 apex);

	// computes variance over the entire tree; does not examine node relationships
	float RecursComputeVariance(
		const   int2 left,
		const   int2 rght,
		const   int2 apex,
		const float3 hgts,
		const    int treeIdx,
		const    int curNodeIdx
	);

	void RecursGenBorderVertices(
		const TriTreeNode* tri,
		const int2 left,
		const int2 rght,
		const int2 apex,
		const int2 depth
	);

	float GetHeight(int2 pos);

private:
	CSMFGroundDrawer* smfGroundDrawer = nullptr;

	// pool used during Tessellate; each invoked Split allocates from this
	CTriNodePool* curTriPool = nullptr;
	float3 midPos;
	// does the variance-tree need to be recalculated for this Patch?
	bool isDirty = true;
	bool isTesselated = false;
	// Did the tesselation tree change from what we have stored in the VBO?
	bool isChanged = false;

	float varianceMaxLimit = std::numeric_limits<float>::max();
	float camDistLODFactor = 1.0f; // defines the LOD falloff in camera distance

	// world-coordinate offsets of this patch
	int2 coors = {-1, -1};


	TriTreeNode baseLeft;  // left base-triangle tree node
	TriTreeNode baseRight; // right base-triangle tree node

	std::array<float, 1 << VARIANCE_DEPTH> varianceTrees[2];
	// TODO: remove, map+update buffer instead
	std::array<float, 3 * (PATCH_SIZE + 1) * (PATCH_SIZE + 1)> vertices;
	std::vector<VA_TYPE_C> borderVertices;
	std::vector<unsigned int> indices;

	// frame on which this patch was last visible, per pass
	// NOTE:
	//   shadow-mesh patches are only ever viewed by one camera
	//   normal-mesh patches can be viewed by *multiple* types!
	std::array<unsigned int, CCamera::CAMTYPE_VISCUL> lastDrawFrames = {};


	// [0] := inner, [1] := border
	GLuint vertexArrays[2] = {0, 0};
	GLuint vertexBuffers[2] = {0, 0};
	GLuint indexBuffer = 0;
};

#endif
