/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATCH_H
#define PATCH_H

#include "Rendering/GL/myGL.h"


class CSMFGroundDrawer;


// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (12)

/**
 * Patch render mode
 * way indices/vertices are send to the GPU
 */
enum RenderMode {
	VA,
	DL,
	VBO
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
	~Patch();

	TriTreeNode* GetBaseLeft()
	{
		return &m_BaseLeft;
	}
	TriTreeNode* GetBaseRight()
	{
		return &m_BaseRight;
	}
	char isDirty()
	{
		return m_VarianceDirty;
	}
	bool isVisibile()
	{
		return m_isVisible;
	}
	int GetTriCount()
	{
		return indices.size() / 3;
	}

	void UpdateVisibility();

	void SetSquareTexture() const;
	
	void UpdateHeightMap();

	// The static half of the Patch Class
	void Init(CSMFGroundDrawer* drawer, int worldX, int worldZ, const float* hMap, int mx, float maxH, float minH);
	void Reset();
	void Tessellate(const float3& campos, int viewradius);
	
	int Render(bool waterdrawn);
	void ComputeVariance();

	// The recursive half of the Patch Class
	void Split(TriTreeNode* tri);
	void RecursTessellate(TriTreeNode* tri, int leftX, int leftY,
			int rightX, int rightY, int apexX, int apexY, int node);
	void RecursRender(TriTreeNode* tri, int leftX, int leftY, int rightX,
		int rightY, int apexX, int apexY, bool dir, int maxdepth, bool waterdrawn);
	unsigned char RecursComputeVariance(int leftX, int leftY,
		float leftZ, int rightX, int rightY, float rightZ,
		int apexX, int apexY, float apexZ, int node);
	void DrawTriArray();

public:
	static void SetRenderMode(RenderMode mode) {
		renderMode = mode;
	}
	static void ToggleRenderMode();

protected:
	static RenderMode renderMode;

	CSMFGroundDrawer* drawer;

	const float* m_HeightMap; //< Pointer to height map to use

	float m_VarianceLeft[1 << (VARIANCE_DEPTH)];  //< Left variance tree
	float m_VarianceRight[1 << (VARIANCE_DEPTH)]; //< Right variance tree

	float* m_CurrentVariance;  //< Which varience we are currently using. [Only valid during the Tessellate and ComputeVariance passes]

	bool m_isVisible; //< Is this patch visible in the current frame?

	TriTreeNode m_BaseLeft;  //< Left base triangle tree node
	TriTreeNode m_BaseRight; //< Right base triangle tree node

	// Some encapsulation functions & extras
	float distfromcam;

	int mapx;
	float maxh, minh;

	std::vector<float> vertices; // Why yes, this IS a mind bogglingly wasteful thing to do: TODO: remove this for both the Displaylist and the VBO implementations (only really needed for vertexarrays)
	std::vector<unsigned int> indices;


	GLuint triList;
	GLuint vertexBuffer;
	GLuint vertexIndexBuffer;

public:
	const float* heightData;
	int m_WorldX, m_WorldY; //< World coordinate offset of this patch.
	unsigned char m_VarianceDirty; //< Does the Varience Tree need to be recalculated for this Patch? FIXME why not bool?
};

#endif
