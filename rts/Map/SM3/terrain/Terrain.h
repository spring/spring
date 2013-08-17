/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SM3_TERRAIN_H
#define SM3_TERRAIN_H

#include "TerrainBase.h"
#include "Map/SM3/Vector3.h"
#include "Map/SM3/Frustum.h"
#include "System/Matrix44f.h"

#include <vector>

class CSM3Map;
class TdfParser;

namespace terrain {

	// Terrain system internal structures
	class TQuad;

	struct Heightmap;
	struct QuadMap;
	struct IndexTable;

	class QuadRenderData;
	class TerrainTexture;
	class RenderDataManager;

	struct ShadowMapParams
	{
		float mid[2];
		float f_a, f_b;
		float shadowMatrix[16];
		unsigned shadowMap;
	};

	class Camera
	{
	public:
		Camera()
			: yaw(0.0f)
			, pitch(0.0f)
			, fov(80.0f)
			, aspect(1.0f)
		{
		}

		void Update();

		float yaw;
		float pitch;
		Vector3 pos;
		Vector3 front;
		Vector3 up;
		Vector3 right;
		CMatrix44f mat;

		float fov;
		float aspect;
	};

	// for rendering debug text
	struct IFontRenderer
	{
		virtual void printf(int x, int y, float size, const char* fmt, ...) = 0;
	protected:
		~IFontRenderer() {}
	};

	struct Config
	{
		Config();

		bool cacheTextures;
		int cacheTextureSize; // size of a single cache texture: 32/64/128/256/512

		bool useBumpMaps;
		bool terrainNormalMaps;
		bool useShadowMaps;  // do the terrain shaders need shadow map support

		float detailMod; // acceptable range is 0.25f - 4

		// (Only in case of terrainNormalMaps=true)
		// heightmap level from which detail normal maps are generated,
		// Normal maps are generated from heightmap[x+node.depth]
		// 0 disables normal map detail-preservation
		int normalMapLevel;

		float anisotropicFiltering; // level of anisotropic filtering - default is 0 (no anisotropy)

		bool useStaticShadow;
		bool forceFallbackTexturing; // only use GL_ARB_texture_env_combine even if shader GL extensions are available
		int maxLodLevel; // lower max lod usually requires less heavy texturing but more geometry
	};

	struct StaticLight
	{
		Vector3 color;
		Vector3 position;
		bool directional; // if true,  position is a direction vector
	};

	struct RenderStats
	{
		int tris;
		int passes;
		int cacheTextureSize;
		int renderDataSize;
	};

    class RenderContext;

	struct LightingInfo
	{
		std::vector<StaticLight> staticLights;
		Vector3 ambient;
	};

	class Terrain
	{
	public:
		Terrain();
		~Terrain();

		// Render contexts
        RenderContext* AddRenderContext(Camera* cam, bool needsTexturing);
		void RemoveRenderContext(RenderContext* ctx);
		void SetActiveContext(RenderContext* ctx); // set active rendering context / camera viewpoint

		void Load(const TdfParser& tdf, LightingInfo* li, ILoadCallback* cb);
		void LoadHeightMap(const TdfParser&, ILoadCallback*);
		void ReloadShaders();
		void Draw();
		void DrawAll(); // draw all terrain nodes, regardless of visibility or lod
		void Update(); // update lod+visibility, should be called when camera position has changed
		void DrawSimple(); // no texture/no lighting
		void DrawOverlayTexture(uint tex); // draw with single texture/no lighting

		TQuad* GetQuadTree() { return quadtree; }

		// Allow it to use any part of the current rendering buffer target for caching textures
		// should be called just before glClear (it doesn't need a cleared framebuffer)
		// Update() should be called before this, otherwise there could be uncached nodes when doing Draw()
		void CacheTextures();
		// Render a single node flat onto the framebuffer, used for minimap rendering and texture caching
		void RenderNodeFlat(int x,int y,int depth);

		void DebugEvent(const std::string& event);
		void DebugPrint(IFontRenderer* fr);

		// World space lighting vector - used for bumpmapping
		void SetShaderParams(Vector3 dir, Vector3 eyePos);

		void SetShadowMap(uint shadowTex);
		void SetShadowParams(ShadowMapParams* smp);

		// Heightmap interface, for dynamically changing heightmaps
		void GetHeightMapSynced(int x, int y, int w, int h, float* dest);
		void HeightMapUpdatedUnsynced(int x,int y,int w,int h);

		// these return the top-level heightmaps
		std::vector<float>& GetCornerHeightMapSynced();
		std::vector<float>& GetCornerHeightMapUnsynced();

		Vector3 TerrainSize();
		int GetHeightmapWidth() const;
		int GetHeightmapHeight() const;

		void CalcRenderStats(RenderStats& stats, RenderContext* ctx=0);

		Config config;

	protected:
		void ClearRenderQuads();

		void RenderNode(TQuad* q);

		Heightmap* heightmap; // list of heightmaps, starting from highest detail level
		Heightmap* lowdetailhm; // end of heightmap list, representing lowest detail level

		TQuad* quadtree;
		std::vector<QuadMap*> qmaps; // list of quadmaps, starting from lowest detail level
		std::vector<TQuad*> updatequads; // temporary list of quads that are being updated
		std::vector<TQuad*> culled;
		std::vector<RenderContext*> contexts;
		RenderContext* activeRC;
		RenderContext* curRC;
		Frustum frustum;
		IndexTable* indexTable;
		TerrainTexture* texturing;
		// settings read from config file
		//float hmScale;
		//float hmOffset;
		uint shadowMap;
		uint quadTreeDepth;

		RenderDataManager* renderDataManager;

		// Initial quad visibility and LOD estimation, registers to renderquads
		void QuadVisLod(TQuad* q);
		// Nabour and lod state calculation
		void UpdateLodFix(TQuad* q);
		void ForceQueue(TQuad* q);

		// Heightmap loading using DevIL
		Heightmap* LoadHeightmapFromImage(const std::string& file, ILoadCallback* cb);
		// RAW 16 bit heightmap loading
		Heightmap* LoadHeightmapFromRAW(const std::string& file, ILoadCallback* cb);

		bool IsShadowed(int x, int y);

		inline void CheckNabourLod(TQuad* q, int xOfs, int yOfs);
		inline void QueueLodFixQuad(TQuad* q);

		// debug variables
		TQuad* debugQuad;
		int nodeUpdateCount; // number of node updates since last frame
		bool logUpdates;

		struct VisNode
		{
			TQuad* quad;
			uint index;
		};

		std::vector<VisNode> visNodes;
	};

}

#endif // SM3_TERRAIN_H
