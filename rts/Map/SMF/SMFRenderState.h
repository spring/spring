/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMF_RENDERSTATE_H
#define SMF_RENDERSTATE_H

#include <cstring> // memset
#include "Map/MapDrawPassTypes.h"

class CSMFGroundDrawer;
struct ISkyLight;
struct LuaMapShaderData;

namespace Shader {
	struct IProgramObject;
}

enum {
	RENDER_STATE_FFP = 0, // fixed-function path
	RENDER_STATE_SSP = 1, // standard-shader path (ARB/GLSL)
	RENDER_STATE_LUA = 2, // Lua-shader path
	RENDER_STATE_SEL = 3, // selected path
	RENDER_STATE_CNT = 4,
};


struct ISMFRenderState {
public:
	static ISMFRenderState* GetInstance(bool haveARB, bool haveGLSL, bool luaShader);
	static void FreeInstance(ISMFRenderState* state) { delete state; }

	virtual ~ISMFRenderState() {}
	virtual bool Init(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) = 0;
	virtual void Kill() = 0;

	virtual bool HasValidShader(const DrawPass::e& drawPass) const = 0;
	virtual bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const = 0;
	virtual bool CanDrawDeferred() const = 0;

	virtual void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;

	virtual void SetSquareTexGen(const int sqx, const int sqy) const = 0;
	virtual void SetCurrentShader(const DrawPass::e& drawPass) = 0;
	virtual void UpdateCurrentShader(const ISkyLight* skyLight) const = 0;
};




struct SMFRenderStateFFP: public ISMFRenderState {
public:
	bool Init(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	) { return false; }
	void Kill() {}

	bool HasValidShader(const DrawPass::e& drawPass) const { return false; }
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
	bool CanDrawDeferred() const { return false; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);

	void SetSquareTexGen(const int sqx, const int sqy) const;
	void SetCurrentShader(const DrawPass::e& drawPass) {}
	void UpdateCurrentShader(const ISkyLight* skyLight) const {}
};


struct SMFRenderStateARB: public ISMFRenderState {
public:
	SMFRenderStateARB() { memset(&arbShaders[0], 0, sizeof(arbShaders)); }
	~SMFRenderStateARB() { Kill(); }

	bool Init(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	);
	void Kill();

	bool HasValidShader(const DrawPass::e& drawPass) const;
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
	bool CanDrawDeferred() const { return false; }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);

	void SetSquareTexGen(const int sqx, const int sqy) const;
	void SetCurrentShader(const DrawPass::e& drawPass);
	void UpdateCurrentShader(const ISkyLight* skyLight) const;

	enum {
		ARB_SHADER_DEFAULT = 0,
		ARB_SHADER_REFLECT = 1,
		ARB_SHADER_REFRACT = 2,
		ARB_SHADER_CURRENT = 3,
		ARB_SHADER_COUNT   = 4,
	};
private:
	// [0] := default (V+F) SMF ARB shader
	// [1] := shader (V+F) for the DynamicWater reflection pass
	// [2] := shader (V+F) for the DynamicWater refraction pass
	// [3] := currently active ARB shader {0, 1, 2}
	Shader::IProgramObject* arbShaders[ARB_SHADER_COUNT];
};


struct SMFRenderStateGLSL: public ISMFRenderState {
public:
	SMFRenderStateGLSL(bool lua): useLuaShaders(lua) { memset(&glslShaders[0], 0, sizeof(glslShaders)); }
	~SMFRenderStateGLSL() { Kill(); }

	bool Init(
		const CSMFGroundDrawer* smfGroundDrawer,
		const LuaMapShaderData* luaMapShaderData
	);
	void Kill();

	bool HasValidShader(const DrawPass::e& drawPass) const;
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
	bool CanDrawDeferred() const { return (HasValidShader(DrawPass::TerrainDeferred)); }

	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);

	void SetSquareTexGen(const int sqx, const int sqy) const;
	void SetCurrentShader(const DrawPass::e& drawPass);
	void UpdateCurrentShader(const ISkyLight* skyLight) const;

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
	Shader::IProgramObject* glslShaders[GLSL_SHADER_COUNT];

	// if true, shader programs for this state are Lua-defined
	bool useLuaShaders;
};

#endif

