/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMF_RENDERSTATE_H
#define SMF_RENDERSTATE_H

#include <array>
#include "Map/MapDrawPassTypes.h"

class CSMFGroundDrawer;
struct float4;
struct ISkyLight;
struct LuaMapShaderData;

namespace Shader {
	struct IProgramObject;
}

enum {
	RENDER_STATE_NOP = 0, // no-op path
	RENDER_STATE_SSP = 1, // standard-shader path
	RENDER_STATE_LUA = 2, // Lua-shader path
	RENDER_STATE_SEL = 3, // selected path
	RENDER_STATE_CNT = 4,
};


struct ISMFRenderState {
public:
	static ISMFRenderState* GetInstance(bool nopState, bool luaShader);
	static void FreeInstance(ISMFRenderState* state) { delete state; }

	virtual ~ISMFRenderState() {}
	virtual bool Init(const CSMFGroundDrawer* smfGroundDrawer) = 0;
	virtual void Kill() = 0;
	virtual void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) = 0;

	virtual bool HasValidShader(const DrawPass::e& drawPass) const = 0;
	virtual bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const = 0;
	virtual bool CanDrawForward() const = 0;
	virtual bool CanDrawDeferred() const = 0;

	virtual void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;

	virtual void SetSquareTexGen(const int sqx, const int sqy, const int nsx, const int mip) const = 0;
	virtual void SetCurrentShader(const DrawPass::e& drawPass) = 0;
	virtual void SetSkyLight(const ISkyLight* skyLight) const = 0;
	virtual void SetAlphaTest(const float4& params) const = 0;
};




struct SMFRenderStateNOP: public ISMFRenderState {
	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override { return true; }
	void Kill() override {}
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override {}

	bool HasValidShader(const DrawPass::e& drawPass) const override { return false; }
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const override { return true; }
	bool CanDrawForward() const override { return false; }
	bool CanDrawDeferred() const  override { return false; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override {}

	void SetSquareTexGen(const int sqx, const int sqy, const int nsx, const int mip) const override {}
	void SetCurrentShader(const DrawPass::e& drawPass) override {}
	void SetSkyLight(const ISkyLight* skyLight) const override {}
	void SetAlphaTest(const float4& params) const override {}
};


struct SMFRenderStateGLSL: public ISMFRenderState {
public:
	SMFRenderStateGLSL(bool lua): useLuaShaders(lua) { glslShaders.fill(nullptr); }
	~SMFRenderStateGLSL() { glslShaders.fill(nullptr); }

	bool Init(const CSMFGroundDrawer* smfGroundDrawer) override;
	void Kill() override;
	void Update(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) override;

	bool HasValidShader(const DrawPass::e& drawPass) const override;
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const override { return true; }
	bool CanDrawForward() const override { return true; }
	bool CanDrawDeferred() const override { return true; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) override;

	void SetSquareTexGen(const int sqx, const int sqy, const int nsx, const int mip) const override;
	void SetCurrentShader(const DrawPass::e& drawPass) override;
	void SetSkyLight(const ISkyLight* skyLight) const override;
	void SetAlphaTest(const float4& params) const override;

	enum {
		GLSL_SHADER_STANDARD = 0,
		GLSL_SHADER_DEFERRED = 1,
		GLSL_SHADER_CURRENT  = 2,
		GLSL_SHADER_COUNT    = 3,
	};

private:
	// [0] := standard version
	// [1] := deferred version (not used unless AllowDeferredMapRendering)
	// [2] := currently active GLSL shader {0, 1}
	std::array<Shader::IProgramObject*, GLSL_SHADER_COUNT> glslShaders;

	// if true, shader programs for this state are Lua-defined
	bool useLuaShaders;
};

#endif

