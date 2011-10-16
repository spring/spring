/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATCH_H
#define PATCH_H

#include "Rendering/GL/myGL.h"

class CVertexArray;
class CSMFGroundDrawer;

//--UNCOMMENT WHICH TRIANGLE PUSHING METHOD SHOULD BE USED!-----------------------

//#define ROAM_VA //vertex array rendering
#define ROAM_DL //static display list rendering
//#define ROAM_VBO //vertex buffer object rendering


// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (12)

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
protected:
	const float* m_HeightMap; // Pointer to height map to use

	float m_VarianceLeft[1 << (VARIANCE_DEPTH)]; // Left variance tree
	float m_VarianceRight[1 << (VARIANCE_DEPTH)]; // Right variance tree

	float* m_CurrentVariance; // Which varience we are currently using. [Only valid during the Tessellate and ComputeVariance passes]

	unsigned char m_isVisible; // Is this patch visible in the current frame?

	TriTreeNode m_BaseLeft; // Left base triangle tree node
	TriTreeNode m_BaseRight; // Right base triangle tree node

	int dbgmax;
	int ptrsf;

	CSMFGroundDrawer* m_parent;
	CVertexArray* m_Array;

public:
	float superfloat[128000]; //Why yes, this IS a mind bogglingly wasteful thing to do: TODO: remove this for both the Displaylist and the VBO implementations (only really needed for vertexarrays)
	const float* heightData;
	unsigned int lend;
	unsigned int rend;

	// Some encapsulation functions & extras
	float distfromcam;
	int m_WorldX, m_WorldY; // World coordinate offset of this patch.
	int mapx;
	int offx, offy;
	float maxh, minh;
	unsigned char m_VarianceDirty; // Does the Varience Tree need to be recalculated for this Patch?

#ifdef ROAM_DL
	GLuint triList;
#endif

#ifdef ROAM_VBO
	//roamvbo
	unsigned int superint[129*129*4];
	GLuint vertexBuffer;
	GLuint vertexIndexBuffer;
#endif

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
	int isVisibile()
	{
		return m_isVisible;
	}
	void SetVisibility();

	// The static half of the Patch Class
	virtual void Init(int worldX, int worldZ, const float* hMap, int mx, float maxH, float minH);
	virtual void Reset();
	virtual void Tessellate(float cx, float cy, float cz, int viewradius);
	
	virtual int Render(CSMFGroundDrawer* parent, int n, bool waterdrawn);
	virtual void ComputeVariance();

	// The recursive half of the Patch Class
	virtual void Split(TriTreeNode* tri);
	virtual void RecursTessellate(TriTreeNode* tri, int leftX, int leftY,
			int rightX, int rightY, int apexX, int apexY, int node);
	virtual void RecursRender(TriTreeNode* tri, int leftX, int leftY, int rightX,
		int rightY, int apexX, int apexY, int n,bool dir, int maxdepth, bool waterdrawn);
	virtual unsigned char RecursComputeVariance(int leftX, int leftY,
		float leftZ, int rightX, int rightY, float rightZ,
		int apexX, int apexY, float apexZ, int node);
	virtual void DrawTriArray(CSMFGroundDrawer* parent);
};

#endif
