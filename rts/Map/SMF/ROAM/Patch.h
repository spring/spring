/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATCH_H
#define PATCH_H

#include "Rendering/GL/myGL.h"
#include "System/Rectangle.h"


class CSMFGroundDrawer;


// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128

// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (12)


/**
 * Patch render mode
 * way indices/vertices are send to the GPU
 */
enum RenderMode {
	VBO = 1,
	DL  = 2,
	VA  = 3
};


//
// TriTreeNode Struct
// Store the triangle tree data, but no coordinates!
//
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

	bool HasChildren() const {
		// All non-leaf nodes have both children, so just check for one
		return !!RightChild;
	}

	TriTreeNode* LeftChild;
	TriTreeNode* RightChild;

	TriTreeNode* BaseNeighbor;
	TriTreeNode* LeftNeighbor;
	TriTreeNode* RightNeighbor;
};

//
// Patch Class
// Store information needed at the Patch level
//
class Patch
{
public:
	Patch();
	void Init(CSMFGroundDrawer* drawer, int worldX, int worldZ, const float* hMap, int mx); //FIXME move this into the ctor
	~Patch();

	friend class CRoamMeshDrawer;

	void Reset();
	
	TriTreeNode* GetBaseLeft()
	{
		return &m_BaseLeft;
	}
	TriTreeNode* GetBaseRight()
	{
		return &m_BaseRight;
	}
	char IsDirty()
	{
		return m_isDirty;
	}
	bool IsVisible()
	{
		return m_isVisible;
	}
	int GetTriCount()
	{
		return indices.size() / 3;
	}

	void UpdateVisibility();
	void UpdateHeightMap(const SRectangle& rect = SRectangle(0,0,PATCH_SIZE,PATCH_SIZE));

	void Tessellate(const float3& campos, int viewradius);
	void ComputeVariance();

	int Render();
	void DrawTriArray();

	void SetSquareTexture() const;

public:
	static void SwitchRenderMode(int mode = -1);

private:
	// The recursive half of the Patch Class
	void Split(TriTreeNode* tri);
	void RecursTessellate(TriTreeNode* const& tri, const int& leftX, const int& leftY, const int& rightX, const int& rightY, const int& apexX, const int& apexY, const int& node);
	void RecursRender(TriTreeNode* const& tri, const int& leftX, const int& leftY, const int& rightX, const int& rightY, const int& apexX, const int& apexY, int maxdepth);
	float RecursComputeVariance(const int& leftX, const int& leftY, const float& leftZ, const int& rightX, const int& rightY, const float& rightZ, const int& apexX, const int& apexY, const float& apexZ, const int& node);

protected:
	static RenderMode renderMode;

	CSMFGroundDrawer* smfGroundDrawer;

	const float* m_HeightMap; //< Pointer to height map to use
	const float* heightData;

	float m_VarianceLeft[1 << (VARIANCE_DEPTH)];  //< Left variance tree
	float m_VarianceRight[1 << (VARIANCE_DEPTH)]; //< Right variance tree
	float* m_CurrentVariance;  //< Which varience we are currently using. [Only valid during the Tessellate and ComputeVariance passes]

	bool m_isVisible; //< Is this patch visible in the current frame?
	bool m_isDirty; //< Does the Varience Tree need to be recalculated for this Patch?

	TriTreeNode m_BaseLeft;  //< Left base triangle tree node
	TriTreeNode m_BaseRight; //< Right base triangle tree node

	float varianceMaxLimit;
	float camDistLODFactor; //< defines the LOD falloff in camera distance

	int mapx;
	int m_WorldX, m_WorldY; //< World coordinate offset of this patch.
	//float minHeight, maxHeight;

	std::vector<float> vertices; // Why yes, this IS a mind bogglingly wasteful thing to do: TODO: remove this for both the Displaylist and the VBO implementations (only really needed for vertexarrays)
	std::vector<unsigned int> indices;

	GLuint triList;
	GLuint vertexBuffer;
	GLuint vertexIndexBuffer;
};

#endif
