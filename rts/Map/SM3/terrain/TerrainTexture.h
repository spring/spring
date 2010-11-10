/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TERRAIN_TEXTURE_H_
#define _TERRAIN_TEXTURE_H_

#include "TerrainNode.h"
#include "Textures.h"

class TdfParser;

namespace terrain {

	struct ShadowMapParams;
	class TQuad;
	struct Heightmap;
	class Lightmap;

//-----------------------------------------------------------------------
// terrain texturing system
//-----------------------------------------------------------------------

	enum VertexDataAttribs
	{
		VRT_Basic = 0,
		VRT_TangentSpaceMatrix = 1, // vertex program needs tangent space matrix (world space -> tangent space)
		VRT_Normal = 2,             // vertex program needs normal (world space)
	};

	struct IShaderSetup
	{
		IShaderSetup () {}
		virtual ~IShaderSetup() {}
		virtual uint GetVertexDataRequirements () { return VRT_Basic; }
		virtual std::string GetDebugDesc () { return std::string();} 

		virtual void BindTSM(Vector3* buf, uint vertexSize) {}
		virtual void UnbindTSM() {}

		virtual void GetTextureUnits(BaseTexture *tex, int& imageUnit, int& coordUnit) {}
	};

	// Helper class to determine how to implement the shader stages (with multipass or multitexturing)
	struct TextureUsage
	{
		std::vector<BaseTexture*> texUnits;
		std::vector<BaseTexture*> coordUnits;

		int AddTextureCoordRead (int maxCoords, BaseTexture *texture); // -1 if failed
		int AddTextureRead (int maxUnits, BaseTexture *texture); // -1 if failed
	};

	class ShaderDef
	{
	public:
		ShaderDef() { hasLighting=useShadowMapping=false; specularExponent = 16.0f; }
		void Parse(const TdfParser& tdf, bool needNormalMap);
		void Optimize(ShaderDef* dst);
		void Output();
		void GetTextureUsage(TextureUsage* tu);
		
		enum StageOp
		{
			Add,
			Mul,
			Alpha,
			Blend
		};

		struct Stage {
			Stage() { source=0; operation=Mul; }

			std::string sourceName; // owned by stage
			BaseTexture *source; // not owned
			StageOp operation;
		};

		std::vector<Stage> normalMapStages;
		std::vector<Stage> stages;
		bool hasLighting, useShadowMapping;
		float specularExponent;

		static void OptimizeStages(std::vector<Stage>& src, std::vector<Stage>& dst);
		static void LoadStages(int numStages, const char *stagename, const TdfParser& tdf, std::vector<Stage>& stages);
	};

	struct NodeSetupParams
	{
		const std::vector<Blendmap*>* blendmaps;
		const std::vector<TiledTexture*>* textures;
		const Vector3* wsLightDir, *wsEyePos;
		int pass;

		ShadowMapParams *shadowMapParams;
	};

	enum PassBlendop
	{
		Pass_Mul, 
		Pass_Add, 
		Pass_Replace,         // color = src
		Pass_Interpolate,      // interpolate dest and src using src alpha
		Pass_MulColor
	};


	struct RenderPass 
	{ // a renderpass
		RenderPass() { shaderSetup=0; depthWrite=false; operation=Pass_Replace; invertAlpha=false;}
		IShaderSetup * shaderSetup; 
		PassBlendop operation;
		bool depthWrite;
		bool invertAlpha; // in case of Pass_Interpolate
	};

	struct RenderSetup
	{
		~RenderSetup ();
		void Clear();
		void DebugOutput();

		std::vector <RenderPass> passes;
		ShaderDef shaderDef;
	};

	struct RenderSetupCollection
	{
		RenderSetupCollection () { sortkey=0;currentShaderSetup=0; vertexDataReq=0; }
		~RenderSetupCollection ();

		std::vector <RenderSetup *> renderSetup;

		// nodesetup sorting key, terrain nodes are sorted by this key, to minimize state changes
		uint sortkey;
		uint vertexDataReq; // vertex data requirements, calculated by OR'ing all return values of GetVertexDataRequirements()
		IShaderSetup* currentShaderSetup;
	};

	// base class for the FragmentProgramHandler (fragment programs) and TexEnvSetupHandler (texture env combine)
	struct ITexShaderHandler
	{
		ITexShaderHandler () {}
		virtual ~ITexShaderHandler() {}

		virtual bool SetupShader (IShaderSetup *shader, NodeSetupParams& params) = 0;// returns false if it doesn't have to be drawn

		virtual void BeginPass (const std::vector<Blendmap*>& blendmaps, const std::vector<TiledTexture*>& textures, int pass) = 0;
		virtual void EndPass () = 0;

		virtual void EndTexturing () {}
		virtual void BeginTexturing () {}

		// true for FragmentProgram, false for TexEnvCombine
		virtual int MaxTextureUnits () = 0;
		virtual int MaxTextureCoords () = 0;

		// Construction
		virtual void BuildNodeSetup (ShaderDef* shaderDef, RenderSetup *rs) = 0;
		virtual void BeginBuild() {}
		virtual void EndBuild() {}
	};

	struct QuadMap;
	struct ILoadCallback;
	struct Config;
	struct IFontRenderer;
	struct LightingInfo;

	class TerrainTexture
	{
	public:
		TerrainTexture();
		~TerrainTexture();

		void Load (const TdfParser *parser, Heightmap *heightmap, TQuad *quadTree, const std::vector<QuadMap*>& qmaps, Config *cfg, ILoadCallback *cb, LightingInfo* li);
		void ReloadShaders (TQuad *quadtree, Config *cfg);

		int NumPasses () const;

		void BeginTexturing ();
		void EndTexturing ();

		void BeginPass(int p);
		void EndPass();

		bool SetupNode (TQuad *q, int pass);

		void DebugPrint (IFontRenderer *fr);
		void DebugEvent (const std::string& event);

		void SetShaderParams (const Vector3& lv, const Vector3& eyePos); // world space light vector
		void SetShadowMapParams (const ShadowMapParams *smp);
		
		int debugShader;
	protected:
		// Calculate blending factors for textures with blending defined by terrain properties
		void GenerateBlendmap (Blendmap* bm, Heightmap *heightmap, int level);
		void InstantiateShaders (Config *cfg, ILoadCallback *cb);
		BaseTexture* LoadImageSource(const std::string& texName, const std::string& basepath, Heightmap *heightmap, ILoadCallback *cb, Config *cfg, bool isBumpmap=false);

		bool SetupShading (RenderPass *p, int passIndex);

		struct GenerateInfo;
		void CreateTexProg (TQuad *node, GenerateInfo* gi);
		void ReloadTexProg (TQuad *node, TerrainTexture::GenerateInfo *gi);

		// Calculate sorting key based on current blendmap optimization state (Blendmap::curAreaResult)
		uint CalcBlendmapSortKey (); 

		int heightmapW;
		int blendmapLOD; // lod level from which blendmaps are generated
		bool useBumpMaps;
		float optimizeEpsilon; // epsilon value used for texturing optimization, read from map
		DetailBumpmap detailBumpmap;

		// nearby shader, possibly tangent space bumpmapping
		// far away shader, world space bumpmapping based on heightmap

		struct ShaderDesc
		{
			ShaderDesc() { index=0; }
			ShaderDef def;
			int index;
		};

		ShaderDesc unlitShader;
		ShaderDesc farShader;
		ShaderDesc nearShader;
		std::vector<ShaderDesc*> shaders;

		RenderSetup *currentRenderSetup;

		ITexShaderHandler* shaderHandler;
		Vector3 wsLightDir, wsEyePos;

		std::vector <Blendmap *> blendMaps;
		std::vector <TiledTexture *> textures;

		std::vector <RenderSetupCollection*> texNodeSetup;
		int maxPasses; // the highest number of passes a RenderSetupCollection requires (some nodes might need less passes than this)

		//only valid during loading
		const TdfParser *tdfParser;

		Lightmap *lightmap;
		ShaderDef shaderDef;

		TiledTexture *flatBumpmap;
		ShadowMapParams *shadowMapParams;
	};
	void SetTexGen (float scale);
};

#endif // _TERRAIN_TEXTURE_H_

