/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDRAWER_STATE_H
#define UNITDRAWER_STATE_H

#include <array>
#include "Map/MapDrawPassTypes.h"
#include "System/type2.h"
#include "System/Matrix44f.h"

struct float4;
class CUnitDrawer;
class CCamera;
struct ISkyLight;

class LuaMatShader;
namespace Shader {
	struct IProgramObject;
}

enum {
	DRAWER_STATE_NOP = 0, // no-op path
	DRAWER_STATE_SSP = 1, // standard-shader path
	DRAWER_STATE_LUA = 2, // custom-shader path
	DRAWER_STATE_SEL = 3, // selected path
	DRAWER_STATE_CNT = 4,
};


struct IUnitDrawerState {
public:
	static IUnitDrawerState* GetInstance(bool nopState, bool luaState);
	static void FreeInstance(IUnitDrawerState* state) { delete state; }

	static const CMatrix44f& GetDummyPieceMatrixRef(size_t idx = 0);
	static const CMatrix44f* GetDummyPieceMatrixPtr(size_t idx = 0);

	static float4 GetTeamColor(int team, float alpha);

	IUnitDrawerState() { modelShaders.fill(nullptr); }
	virtual ~IUnitDrawerState() {}

	virtual bool Init(const CUnitDrawer*) { return false; }
	virtual void Kill() {}

	virtual bool CanEnable(const CUnitDrawer*) const { return false; }
	virtual bool CanDrawAlpha() const { return false; }
	virtual bool CanDrawDeferred() const { return false; }

	virtual void Enable(const CUnitDrawer*, bool, bool) = 0;
	virtual void Disable(const CUnitDrawer*, bool) = 0;

	virtual void EnableTextures() const = 0;
	virtual void DisableTextures() const = 0;
	virtual void EnableShaders(const CUnitDrawer*) = 0;
	virtual void DisableShaders(const CUnitDrawer*) = 0;

	virtual void SetSkyLight(const ISkyLight*) const = 0;
	virtual void SetAlphaTest(const float4& params) const = 0;
	virtual void SetTeamColor(int team, const float2 alpha) const = 0;
	virtual void SetNanoColor(const float4& color) const = 0;
	virtual void SetMatrices(const CMatrix44f& modelMat, const std::vector<CMatrix44f>& pieceMats) const = 0;
	virtual void SetMatrices(const CMatrix44f& modelMat, const CMatrix44f* pieceMats, size_t numPieceMats) const = 0;
	virtual void SetWaterClipPlane(const DrawPass::e& drawPass) const = 0; // water
	virtual void SetBuildClipPlanes(const float4&, const float4&) const = 0; // nano-frames

	void SetActiveShader(unsigned int shadowed, unsigned int deferred) {
		// shadowed=1 --> shader 1 (deferred=0) or 3 (deferred=1)
		// shadowed=0 --> shader 0 (deferred=0) or 2 (deferred=1)
		modelShaders[MODEL_SHADER_ACTIVE] = modelShaders[shadowed + deferred * 2];
	}

	const Shader::IProgramObject* GetActiveShader() const { return modelShaders[MODEL_SHADER_ACTIVE]; }
	      Shader::IProgramObject* GetActiveShader()       { return modelShaders[MODEL_SHADER_ACTIVE]; }

	enum ModelShaderProgram {
		MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with self-shadowing
		MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
		MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW (GLSL-only)

		MODEL_SHADER_ACTIVE            = 4, ///< currently active model shader
		MODEL_SHADER_COUNT             = 5,
	};

protected:
	// shared ARB and GLSL state managers
	void EnableCommon(const CUnitDrawer*, bool);
	void DisableCommon(const CUnitDrawer*, bool);
	void EnableTexturesCommon() const;
	void DisableTexturesCommon() const;

protected:
	std::array<Shader::IProgramObject*, MODEL_SHADER_COUNT> modelShaders;
};




struct UnitDrawerStateNOP: public IUnitDrawerState {
public:
	bool Init(const CUnitDrawer*) override { return true; }
	void Kill() override {}

	bool CanEnable(const CUnitDrawer*) const override { return true; }
	bool CanDrawAlpha() const override { return false; }
	bool CanDrawDeferred() const  override { return false; }

	void Enable(const CUnitDrawer*, bool, bool) override {}
	void Disable(const CUnitDrawer*, bool) override {}

	void EnableTextures() const override {}
	void DisableTextures() const override {}
	void EnableShaders(const CUnitDrawer*) override {}
	void DisableShaders(const CUnitDrawer*) override {}

	void SetSkyLight(const ISkyLight*) const override {}
	void SetAlphaTest(const float4& params) const override {}
	void SetTeamColor(int team, const float2 alpha) const override {}
	void SetNanoColor(const float4& color) const override {}
	void SetMatrices(const CMatrix44f& modelMat, const std::vector<CMatrix44f>& pieceMats) const override {}
	void SetMatrices(const CMatrix44f& modelMat, const CMatrix44f* pieceMats, size_t numPieceMats) const override {}
	void SetWaterClipPlane(const DrawPass::e& drawPass) const override {}
	void SetBuildClipPlanes(const float4&, const float4&) const override {}
};


// does nothing by itself; LuaObjectDrawer handles custom uniforms
// this exists to ensure UnitDrawerStateGLSL::SetMatrices will not
// be invoked (via *Drawer::Draw*Trans) during a custom bin-pass
struct UnitDrawerStateLUA: public IUnitDrawerState {
public:
	void Enable(const CUnitDrawer* ud, bool, bool) override { EnableShaders(ud); }
	void Disable(const CUnitDrawer* ud, bool) override { DisableShaders(ud); }

	void EnableTextures() const override {}
	void DisableTextures() const override {}
	void EnableShaders(const CUnitDrawer*) override {}
	void DisableShaders(const CUnitDrawer*) override {}

	void SetSkyLight(const ISkyLight*) const override {}
	void SetAlphaTest(const float4& params) const override {}
	void SetTeamColor(int team, const float2 alpha) const override {} // handled via LuaObjectDrawer::SetObjectTeamColor
	void SetNanoColor(const float4& color) const override {}
	void SetMatrices(const CMatrix44f& modelMat, const std::vector<CMatrix44f>& pieceMats) const override { SetMatrices(modelMat, pieceMats.data(), pieceMats.size()); }
	void SetMatrices(const CMatrix44f& modelMat, const CMatrix44f* pieceMats, size_t numPieceMats) const override {} // handled via LuaObjectDrawer::SetObjectMatrices
	void SetWaterClipPlane(const DrawPass::e& drawPass) const override {}
	void SetBuildClipPlanes(const float4&, const float4&) const override {}
};


struct UnitDrawerStateGLSL: public IUnitDrawerState {
public:
	bool Init(const CUnitDrawer*) override;
	void Kill() override;

	bool CanEnable(const CUnitDrawer*) const override { return true; }
	bool CanDrawAlpha() const override { return true; }
	bool CanDrawDeferred() const  override { return true; }

	void Enable(const CUnitDrawer*, bool, bool) override;
	void Disable(const CUnitDrawer*, bool) override;

	void EnableTextures() const override;
	void DisableTextures() const override;
	void EnableShaders(const CUnitDrawer*) override;
	void DisableShaders(const CUnitDrawer*) override;

	void SetSkyLight(const ISkyLight*) const override;
	void SetAlphaTest(const float4& params) const override;
	void SetTeamColor(int team, const float2 alpha) const override;
	void SetNanoColor(const float4& color) const override;
	void SetMatrices(const CMatrix44f& modelMat, const std::vector<CMatrix44f>& pieceMats) const override { SetMatrices(modelMat, pieceMats.data(), pieceMats.size()); }
	void SetMatrices(const CMatrix44f& modelMat, const CMatrix44f* pieceMats, size_t numPieceMats) const override;
	void SetWaterClipPlane(const DrawPass::e& drawPass) const override;
	void SetBuildClipPlanes(const float4&, const float4&) const override;
};

#endif

