/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TERRAIN_TEXTURE_GLSL_H_
#define _TERRAIN_TEXTURE_GLSL_H_

#include "TerrainTexture.h"

namespace terrain 
{
	struct Blendmap;
	struct TiledTexture;
	class BufferTexture;

	struct GLSLBaseShader : public IShaderSetup
	{
		virtual void Setup(NodeSetupParams& params) = 0;
		virtual void Cleanup() = 0;
	};

	struct SimpleCopyShader 
	{
		SimpleCopyShader(BufferTexture* src);
		~SimpleCopyShader();

		void Setup();
		void Cleanup();

		BufferTexture* source;
		GLhandleARB vertexShader;
		GLhandleARB fragmentShader;
		GLhandleARB program;
	};

	struct SimpleCopyNodeShader : public GLSLBaseShader
	{
		SimpleCopyNodeShader(SimpleCopyShader* scs) :
			shader(scs) {}

		void Setup(NodeSetupParams& params) { shader->Setup(); }
		void Cleanup() { shader->Cleanup(); }

		SimpleCopyShader* shader;
	};

	struct NodeGLSLShader : public GLSLBaseShader
	{
		NodeGLSLShader();
		~NodeGLSLShader();
		
		std::string GetDebugDesc();
		uint GetVertexDataRequirements();
		void GetTextureUnits(BaseTexture* tex, int& imageUnit, int& coordUnit);

		void BindTSM(Vector3* buf, uint vertexSize);
		void UnbindTSM();

		std::vector<BaseTexture*> texUnits; // texture image units
		std::vector<BaseTexture*> texCoordGen; // textures that are used for texture coordinate generation

		GLhandleARB vertexShader;
		GLhandleARB fragmentShader;
		GLhandleARB program;

		GLint tsmAttrib; // attribute ID of tangent space matrix
		GLint wsLightDirLocation, wsEyePosLocation; // location for wsLightDir
		uint vertBufReq;
		std::string debugstr;

		GLint shadowMapLocation;
		GLint shadowMatrixLocation;
		GLint shadowParamsLocation;

		BufferTexture* renderBuffer; // 0 for normal display

		void Setup(NodeSetupParams& params);
		void Cleanup();
	};

	class GLSLShaderHandler : public ITexShaderHandler
	{
	public:
		GLSLShaderHandler();
		~GLSLShaderHandler();

		// ITexShaderHandler interface
		void BeginPass(const std::vector<Blendmap*>& blendmaps, const std::vector<TiledTexture*>& textures, int pass);
		void EndPass() {}

		void BeginTexturing();
		void EndTexturing();

		void BuildNodeSetup(ShaderDef* shaderDef, RenderSetup* renderSetup);
		bool SetupShader(IShaderSetup* shadercfg, NodeSetupParams& params);

		void BeginBuild();
		void EndBuild();

		int MaxTextureUnits();
		int MaxTextureCoords();
	protected:
		GLSLBaseShader* curShader;

		BufferTexture* buffer;
		std::vector<RenderSetup*> renderSetups;

		SimpleCopyShader* scShader;
	};
};

#endif // _TERRAIN_TEXTURE_GLSL_H_

