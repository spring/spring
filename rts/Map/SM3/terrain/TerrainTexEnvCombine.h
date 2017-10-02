/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TERRAIN_TEX_ENV_COMBINE_H_
#define _TERRAIN_TEX_ENV_COMBINE_H_

#include "TerrainTexture.h" // for IShaderSetup

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
			Interp, ///< interpolate:  color = prev.RGB * prev.Alpha + current.RGB * ( 1-prev.Alpha )
			InsertAlpha, ///< use the previous color and insert an alpha channel
			AlphaToRGB, ///< use the texture alpha as RGB
			Dot3, ///< dot product, requires GL_ARB_texture_env_dot3
		};
		enum Source {
			None, TextureSrc, ColorSrc
		};

		Operation operation;
		Source source;
		BaseTexture* srcTexture;

		TexEnvStage();
	};

	/// each TQuad has an instance of this class
	struct NodeTexEnvSetup : public IShaderSetup
	{
		NodeTexEnvSetup()
			: IShaderSetup()
			, usedTexUnits(0)
			{}

		std::string GetDebugDesc();
		uint GetVertexDataRequirements();
		void GetTextureUnits(BaseTexture* tex, int &imageUnit, int& coordUnit);

		int usedTexUnits;
		std::vector <TexEnvStage> stages;
	};

	/// Texenv setup
	class TexEnvSetupHandler : public ITexShaderHandler
	{
	public:
		TexEnvSetupHandler();

		void SetTexCoordGen(float* tgv);

		// ITexShaderHandler interface
		void BeginTexturing();
		void EndTexturing();
		void BeginPass(const std::vector<Blendmap*>& blendmaps, const std::vector<TiledTexture*>& textures, int pass);
		void EndPass() {}
		void BuildNodeSetup(ShaderDef* shaderDef, RenderSetup* renderSetup);
		bool SetupShader(IShaderSetup* shadercfg, NodeSetupParams& params);

		int MaxTextureUnits();
		int MaxTextureCoords();

	protected:
		int maxtu;

		// Used when enabling texture units without actual texture
		GLuint whiteTexture;
		NodeTexEnvSetup* lastShader;

		NodeTexEnvSetup* curSetup;
	};

};

#endif // _TERRAIN_TEX_ENV_COMBINE_H_

