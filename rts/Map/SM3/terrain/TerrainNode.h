/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TERRAIN_NODE_H_
#define _TERRAIN_NODE_H_

#include "TerrainVertexBuffer.h"

class Frustum;
struct Sm3VisibilityNode;

namespace terrain
{
	struct int2 { 
		int2(int X,int Y) : x(X),y(Y) {}
		int2() { x=y=0; }
		int x,y; 
	};

	class Camera;
	struct Heightmap;
	struct QuadMap;
	struct IndexTable;
	class QuadRenderData;
	class QuadNormals;
	struct RenderSetupCollection;
	struct IShaderSetup;
	class TerrainTexture;

//-----------------------------------------------------------------------
// Terrain Quadtree Node
//-----------------------------------------------------------------------

	static const int QUAD_W=16;
	static const int VERTC = QUAD_W+1;
	static const int MAX_INDICES=((QUAD_W+1)*(QUAD_W+1)*2*3);
	static const int NUM_VERTICES=((QUAD_W+1)*(QUAD_W+1));

	const float SquareSize=8.0f;
	const float VBufMinDetail=0.25f; // at lod < 0.25f, the vertex buffer is deleted

	const float AREA_TEST_RANGE=0.05f;

	class TQuad
	{
	public:
		TQuad();
		~TQuad();
		bool isLeaf () { return childs[0]==0; }
		// sqStart=start in square coordinates
		// hmStart=start pixel on hm
		void Build (Heightmap *hm, int2 sqStart, int2 hmStart, int2 quadPos, int w, int depth);
		void Draw (IndexTable *indexTable, bool onlyPositions, int lodState);
		bool InFrustum (Frustum *f);
		float CalcLod (const Vector3& campos);
		void CollectNodes(std::vector<TQuad*>& quads);
		void FreeCachedTexture();

		TQuad* FindSmallestContainingQuad2D(const Vector3& pos, float range, int maxlevel);

		int GetVertexSize ();

		// Possible objects linked to this quad, for visibility determination
		std::vector<Sm3VisibilityNode*> nodeLinks;

		TQuad *parent;
		TQuad *childs[4];
		Vector3 start, end;  // quad node bounding box
		int2 qmPos; // Quad map coordinates
		int2 hmPos;
		int2 sqPos; // square position (position on highest detail heightmap)
		int depth, width;
		RenderSetupCollection *textureSetup;
		GLuint cacheTexture;
		float maxLodValue; 		 /** maximum of the LOD values calculated for each render contexts
			 used for determining if the QuadRenderData can be marked as free (see RenderDataManager::FreeUnused) */

		enum DrawState {
			NoDraw, Parent, Queued, Culled
		} drawState;

		QuadNormals *normalData;
		QuadRenderData *renderData;
	};

	// Each render context has a QuadRenderInfo for every quad they have to render
	struct QuadRenderInfo
	{
		int lodState;
		TQuad *quad;
	};

	class RenderContext
	{
	public:
		Camera *cam;
		bool needsTexturing; // contexts like shadow buffers don't need texturing
		bool needsNormalMap;

		std::vector<QuadRenderInfo> quads;
	};

	// renderdata for a visible quad
	class QuadRenderData
	{
	public:
		QuadRenderData();
		~QuadRenderData();

		uint GetDataSize();

		// seems silly yeah, but I use it to set breakpoints in.
		TQuad* GetQuad() { return quad; }
		void SetQuad(TQuad *q) { quad=q; }

		// renderdata: normalmap + vertex buffer
		// normalmap for detail preservation
		GLuint normalMap;
		uint normalMapW, normalMapTexWidth;

		VertexBuffer vertexBuffer;
		int vertexSize;

		int index;
		bool used;
	protected:
		TQuad *quad;
	};

	// Manager for QuadRenderData
	class RenderDataManager {
	public:
		RenderDataManager(Heightmap *roothm, QuadMap *rootqm);
		~RenderDataManager();

		void Free (QuadRenderData *qrd);
		void FreeUnused ();
		void PruneFreeList (int maxFreeRD=0); /// allow the list of free renderdata to be maxFreeRD or lower

		void InitializeNode (TQuad *q);
		void InitializeNodeNormalMap (TQuad *q, int cfgNormalMapLevel);

		void UpdateRect(int sx,int sy,int w,int h);

		void ClearStat () {
			normalDataAllocates=renderDataAllocates=0;
		}

		int normalDataAllocates; // performance statistic
		int renderDataAllocates;

		int QuadRenderDataCount () { return qrd.size(); }

	protected:
		QuadRenderData* Allocate();

		std::vector<QuadRenderData*> qrd;
		std::vector<QuadRenderData*> freeRD;
		Heightmap *roothm;
		QuadMap* rootQMap;
	};

//-----------------------------------------------------------------------
// Heightmap
//-----------------------------------------------------------------------
	struct Heightmap {
		Heightmap ();
		~Heightmap ();

		void Alloc (int W,int H);
		void LodScaleDown (Heightmap * dst);
		void FindMinMax (int2 st, int2 size, float& minH, float& maxH);
		Heightmap *CreateLowDetailHM ();
		void GenerateNormals ();
		void UpdateLower (int sx,int sy,int w,int h);
		Heightmap *GetLevel (int level); // level > 0 returns a high detail HM
										// level < 0 returns a lower detail HM

		float& at(int x,int y) { return data[y*w+x]; }
		uchar* GetNormal(int x,int y) { return &normalData[3*(y*w+x)]; }
		float HeightAt(int x,int y) { return data[y*w+x]; }// * scale + offset; }

		int w,h;
		float *data;
		//float scale, offset;
		float squareSize;

        uchar *normalData; // optional heightmap normals, stored as compressed vectors (3 bytes per normal)

		Heightmap *lowDetail, *highDetail; // geomipmap chain links
	};

//-----------------------------------------------------------------------
// Quad map - stores a 2D map of the quad nodes, 
//            for quick access of nabours
//-----------------------------------------------------------------------
	struct QuadMap 
	{
		QuadMap () {
			w = 0;
			map = 0;
			highDetail=lowDetail=0;
		}
		~QuadMap () {
			delete[] map;
		}

		void Alloc (int W);
		void Fill (TQuad *root);
		TQuad*& At (int x,int y) { return map[y*w+x]; }

		int w;
		TQuad **map;

		QuadMap *highDetail;
		QuadMap *lowDetail;
	};

	
	// Applies a "sobel" filter to the heightmap to find the slope in X and Z(Y in heightmap) direction
	inline void CalculateTangents (Heightmap *hm, int x, int y, Vector3& tangent, Vector3& binormal)
	{
		int xp = (x < hm->w-1) ? x+1 : x;
		int xm = (x > 0) ? x-1 : x;
		int yp = (y < hm->h-1) ? y+1 : y;
		int ym = (y > 0) ? y-1 : y;

		//X filter:
		//-1 0 1
		//-2 0 2
		//-1 0 1
		/*float dhdx = -hm->HeightAt (xm,ym) + hm->HeightAt (xp, ym);
		dhdx += 2.0f * (-hm->HeightAt (xm,y) + hm->HeightAt (xp, y));
		dhdx += -hm->HeightAt (xm,yp) + hm->HeightAt (xp, yp);
		dhdx *= 0.25f;*/

		int dhdx = int(-hm->at (xm,ym) + hm->at (xp, ym));
		dhdx += int(2 * (-hm->at (xm,y) + hm->at (xp, y)));
		dhdx += int(-hm->at (xm,yp) + hm->at (xp, yp));

		// 
		//Z filter:
		//-1 -2 -1
		//0  0  0
		//1  2  1
		//
		/*float dhdz = hm->HeightAt (xm, yp) + 2.0f * hm->HeightAt (x, yp) + hm->HeightAt (xp, yp);
		dhdz -= hm->HeightAt (xm, ym) + 2.0f * hm->HeightAt (x, ym) + hm->HeightAt (xp, ym);
		dhdz *= 0.25f;*/
		int dhdz = int(hm->at (xm, yp) + 2 * hm->at (x, yp) + hm->at (xp, yp));
		dhdz -= int(hm->at (xm, ym) + 2 * hm->at (x, ym) + hm->at (xp, ym));

		tangent = Vector3(hm->squareSize * 2.0f, /*hm->scale * */ dhdx * 0.25f, 0.0f);
		binormal = Vector3(0.0f, dhdz * /*hm->scale * */ 0.25f, hm->squareSize * 2.0f);

//		tangent.set (vdif * 2.0f, dhdx, 0.0f);
//		binormal.set (0.0f, dhdz, vdif * 2.0f);
	}


//-----------------------------------------------------------------------
// IndexTable
//    generates index buffers for the 16 different LOD configurations a quad can be in.
//-----------------------------------------------------------------------
#define NUM_TABLES 16

	// flags that determine if a specific quad side has lower res
#define up_bit 1    // x=0 y=-1
#define left_bit 2  // x=-1 y=0
#define right_bit 4 // x=1 y=0
#define down_bit 8  // x=0 y=1

	// table of precomputed indices
	struct IndexTable
	{
		typedef unsigned short index_t;
		IndexTable ();

		GLenum IndexType() { return GL_UNSIGNED_SHORT; }

		int size[NUM_TABLES];
		IndexBuffer buffers[NUM_TABLES];

		void Calculate (int sides);
	};

};

#endif // _TERRAIN_NODE_H_

