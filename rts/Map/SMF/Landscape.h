
#ifndef LANDSCAPE_H
#define LANDSCAPE_H

#include "Patch.h"
#include "SMFGroundDrawer.h"
#include "Map/BaseGroundDrawer.h"

class CSMFGroundDrawer;

// ---------------------------------------------------------------------
// Various Pre-Defined map sizes & their #define counterparts:



// How many TriTreeNodes should be allocated?
#define POOL_SIZE (500000)

// How many heightmap pixels a patch consists of
#define PATCH_SIZE 128

//useful stuff:
#define SQR(x) ((x) * (x))
#define MAX(a,b) ((a < b) ? (b) : (a))
#define MIN(a,b) ((a > b) ? (b) : (a))
#define DEG2RAD(a) (((a) * M_PI) / 180.0f)
#define M_PI (3.14159265358979323846f)



//
// Landscape Class
// Holds all the information to render an entire landscape.
//
class Landscape
{
protected:
	const float* m_HeightMap;                //< HeightMap of the Landscape

	static int m_NextTriNode;                //< Index to next free TriTreeNode
	static TriTreeNode m_TriPool[POOL_SIZE]; //< Pool of TriTree nodes for splitting

	static int GetNextTriNode() { return m_NextTriNode; }
	static void SetNextTriNode( int nNextNode ) { m_NextTriNode = nNextNode; }

public:
	Patch* m_Patches; //[NUM_PATCHES_PER_SIDE][NUM_PATCHES_PER_SIDE];  //< Array of patches

	static TriTreeNode* AllocateTri();
	int h, w;
	float* minhpatch; //< min and maximum heights for each patch for view testing.
	float* maxhpatch;
	const float* heightData;

	virtual void Init(const float* hMap, int bx, int by);
	virtual void Reset();
	virtual void Tessellate(float cx, float cy, float cz, int viewradius);
	virtual int Render(CSMFGroundDrawer* parent, bool changed, bool shadows, bool waterdrawn);
	virtual void Explosion(float x, float y, float z, float radius);
	int updateCount;
};


#endif
