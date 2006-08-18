/*---------------------------------------------------------------------
 Terrain Renderer using texture splatting and geomipmapping

 Copyright (2006) Jelmer Cnossen
 This code is released under GPL license (See LICENSE.html for info)
---------------------------------------------------------------------*/

namespace terrain {

//-----------------------------------------------------------------------
// texture environment programs - texture setup using GL_TEXTURE_ENV_COMBINE
//-----------------------------------------------------------------------

	struct Blendmap;
	struct TiledTexture;

	struct TexEnvStage
	{
		enum Operation {
			Replace,
			Mul,
			Add,
			Sub,
			Interp,   // interpolate:  color = prev.RGB * prev.Alpha + current.RGB * ( 1-prev.Alpha )
			InsertAlpha, // use the previous color and insert an alpha channel
			AlphaToRGB, // use the texture alpha as RGB
			Dot3,   // dot product, requires GL_ARB_texture_env_dot3
		};
		enum Source {
			None, TextureSrc, ColorSrc
		};

		Operation operation;
		Source source;
		BaseTexture *srcTexture;

		TexEnvStage ();
	};

	// each TQuad has an instance of this class
	struct NodeTexEnvSetup : public IShaderSetup
	{
		NodeTexEnvSetup () {
			usedTexUnits=0;
		}
		int usedTexUnits;
		std::vector <TexEnvStage> stages;

		std::string GetDebugDesc ();
		uint GetVertexDataRequirements ();
		void GetTextureUnits(BaseTexture* tex, int &imageUnit, int& coordUnit);
	};

	// Texenv setup
	class TexEnvSetupHandler : public ITexShaderHandler
	{
	public:
		TexEnvSetupHandler ();

		void SetTexCoordGen (float *tgv);

		// ITexShaderHandler interface
		void BeginTexturing();
		void EndTexturing();
		void BeginPass (const std::vector<Blendmap*>& blendmaps, const std::vector<TiledTexture*>& textures);
		void EndPass() {}
		void BuildNodeSetup (ShaderDef *shaderDef, RenderSetup *renderSetup);
		bool SetupShader (IShaderSetup *shadercfg, NodeSetupParams& params);

		int MaxTextureUnits ();
		int MaxTextureCoords ();
	protected:
		int maxtu;
		bool hasDot3;
		// Used when enabling texture units without actual texture
		uint whiteTexture;
		NodeTexEnvSetup *lastShader;

		NodeTexEnvSetup* curSetup;
	};

};
