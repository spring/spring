/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <string>

#include "System/type2.h"
#include "System/float4.h"
#include "System/SafeUtil.h"

class CCamera;
struct ISkyLight;
class CMatrix44f;

namespace Shader {
	struct IProgramObject;
}

enum ModelShaderProgram {
	MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
	MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with    self-shadowing
	MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
	MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW   (GLSL-only)
	MODEL_SHADER_COUNT             = 4,
};

enum ShaderCameraModes {
	NORMAL_CAMERA = 0,
	REFLCT_CAMERA = 1,
	REFRAC_CAMERA = 2,
};

enum ShaderMatrixModes {
	NORMAL_MATMODE = 0,
	STATIC_MATMODE = 1,
	 ARRAY_MATMODE = 2, //future
};

enum ShaderShadingModes {
	NORMAL_SHADING = 0,
	SKIP_SHADING = 1,
};

class IModelDrawerState {
public:
	template<typename T>
	static void InitInstance(int t) {
		if (modelDrawerStates[t] == nullptr)
			modelDrawerStates[t] = new T{};
	}
	static void KillInstance(int t) {
		spring::SafeDelete(modelDrawerStates[t]);
	}
public:
	IModelDrawerState();
	virtual ~IModelDrawerState() {}

	virtual bool CanEnable() const = 0;
	virtual bool CanDrawDeferred() const { return false; }
	virtual bool IsLegacy() const = 0;

	bool IsValid() const;

	virtual void Enable(bool deferredPass, bool alphaPass) const = 0;
	virtual void Disable(bool deferredPass) const = 0;

	virtual void EnableTextures() const = 0;
	virtual void DisableTextures() const = 0;

	//virtual void UpdateCurrentShaderSky(const ISkyLight*) const = 0;

	// alpha.x := alpha-value
	// alpha.y := alpha-pass (true or false)
	virtual bool SetTeamColor(int team, float alpha = 1.0f) const;
	virtual void SetNanoColor(const float4& color) const = 0;

	void SetColorMultiplier(float a = 1.0f) const { SetColorMultiplier(1.0f, 1.0f, 1.0f, a); };
	virtual void SetColorMultiplier(float r, float g, float b, float a) const {
		assert(false);  //doesn't make sense, except in GL4, overridden below
	};
	virtual ShaderCameraModes SetCameraMode(ShaderCameraModes scm_ = ShaderCameraModes::NORMAL_CAMERA) const {
		assert(false);  //doesn't make sense, except in GL4, overridden below
		std::swap(scm, scm_);
		return scm_;
	};
	virtual ShaderMatrixModes SetMatrixMode(ShaderMatrixModes smm_ = ShaderMatrixModes::NORMAL_MATMODE) const {
		assert(false);  //doesn't make sense, except in GL4, overridden below
		std::swap(smm, smm_);
		return smm_;
	};
	virtual ShaderShadingModes SetShadingMode(ShaderShadingModes ssm_ = ShaderShadingModes::NORMAL_SHADING) const {
		assert(false);  //doesn't make sense, except in GL4, overridden below
		std::swap(ssm, ssm_);
		return ssm_;
	};
	virtual void SetStaticModelMatrix(const CMatrix44f& mat) const {
		assert(false);  //doesn't make sense, except in GL4, overridden below
	};
	virtual void SetClipPlane(uint8_t idx, const float4& cp = {0.0f,  0.0f, 0.0f, 1.0f}) const {
		assert(false);  //doesn't make sense, except in GL4, overridden below
	};

	void SetActiveShader(bool shadowed, bool deferred) const {
		// shadowed=1 --> shader 1 (deferred=0) or 3 (deferred=1)
		// shadowed=0 --> shader 0 (deferred=0) or 2 (deferred=1)
		modelShader = modelShaders[shadowed + deferred * 2];
	}
	Shader::IProgramObject* ActiveShader() { return modelShader; }
public:
	void SetupOpaqueDrawing(bool deferredPass) const;
	void ResetOpaqueDrawing(bool deferredPass) const;
	void SetupAlphaDrawing(bool deferredPass) const;
	void ResetAlphaDrawing(bool deferredPass) const;
public:
	inline static std::array<IModelDrawerState*, MODEL_SHADER_COUNT> modelDrawerStates = {};
public:
	/// <summary>
	/// .x := regular unit alpha
	/// .y := ghosted unit alpha (out of radar)
	/// .z := ghosted unit alpha (inside radar)
	/// .w := AI-temp unit alpha
	/// </summary>
	inline static float4 alphaValues = {};
protected:
	mutable ShaderCameraModes  scm = ShaderCameraModes::NORMAL_CAMERA;
	mutable ShaderShadingModes ssm = ShaderShadingModes::NORMAL_SHADING;
	mutable ShaderMatrixModes  smm = ShaderMatrixModes::NORMAL_MATMODE;

	std::array<Shader::IProgramObject*, MODEL_SHADER_COUNT> modelShaders;
	mutable Shader::IProgramObject* modelShader = nullptr;
};


class CModelDrawerStateLegacy : public IModelDrawerState {
public:
	// caps functions
	bool IsLegacy() const override { return true; }
protected:
	inline static const std::string PO_CLASS = "[ModelDrawer]";
};

class CModelDrawerStateFFP final : public CModelDrawerStateLegacy {
public:
	CModelDrawerStateFFP();
	~CModelDrawerStateFFP() override;
public:
	// caps functions
	bool CanEnable() const override { return true; }
	bool CanDrawDeferred() const override { return false; }

	bool SetTeamColor(int team, float alpha) const override;

	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
private:
	void SetNanoColor(const float4& color) const override;

	void EnableTextures() const override;
	void DisableTextures() const override;
private:
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();
};

class CModelDrawerStateGLSL final : public CModelDrawerStateLegacy {
public:
	CModelDrawerStateGLSL();
	~CModelDrawerStateGLSL() override;
public:
	// caps functions
	bool CanEnable() const override;
	bool CanDrawDeferred() const override;

	bool SetTeamColor(int team, float alpha) const override;

	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
private:
	void SetNanoColor(const float4& color) const override;

	void EnableTextures() const override;
	void DisableTextures() const override;
};

class CModelDrawerStateGL4 final : public IModelDrawerState {
public:
	CModelDrawerStateGL4();
	~CModelDrawerStateGL4() override;
public:
public:
	// caps functions
	bool IsLegacy() const override { return false; }

	bool CanEnable() const override;
	bool CanDrawDeferred() const override;

	bool SetTeamColor(int team, float alpha) const override;

	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;

	void SetColorMultiplier(float r, float g, float b, float a) const override;

	ShaderCameraModes SetCameraMode(ShaderCameraModes sdm_) const override;
	ShaderMatrixModes SetMatrixMode(ShaderMatrixModes smm_) const override;
	ShaderShadingModes SetShadingMode(ShaderShadingModes sm) const override;
	void SetStaticModelMatrix(const CMatrix44f& mat) const override;
	void SetClipPlane(uint8_t idx, const float4& cp = { 0.0f,  0.0f, 0.0f, 1.0f }) const override;
private:
	void SetNanoColor(const float4& color) const override;

	void EnableTextures() const override;
	void DisableTextures() const override;
private:
	inline static const std::string PO_CLASS = "[ModelDrawer-GL4]";
};

