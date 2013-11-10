/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMF_RENDERSTATE_H
#define SMF_RENDERSTATE_H

#include "Map/MapDrawPassTypes.h"
#include <stddef.h> //required for NULL

class CSMFGroundDrawer;
struct ISkyLight;

namespace Shader {
	struct IProgramObject;
};


struct ISMFRenderState {
public:
	static ISMFRenderState* GetInstance(bool haveARB, bool haveGLSL);
	static void FreeInstance(ISMFRenderState* state) { delete state; }

	virtual ~ISMFRenderState() {}
	virtual bool Init(const CSMFGroundDrawer* smfGroundDrawer) = 0;
	virtual void Kill() = 0;
	virtual bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const = 0;
	virtual bool CanDrawDeferred() const { return false; }
	virtual void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass) = 0;
	virtual void SetSquareTexGen(const int sqx, const int sqy) const = 0;
	virtual void SetCurrentShader(const DrawPass::e& drawPass) = 0;
	virtual void UpdateCurrentShader(const ISkyLight* skyLight) const = 0;
};




struct SMFRenderStateFFP: public ISMFRenderState {
public:
	bool Init(const CSMFGroundDrawer* smfGroundDrawer) { return false; }
	void Kill() {}
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void SetSquareTexGen(const int sqx, const int sqy) const;
	void SetCurrentShader(const DrawPass::e& drawPass) {}
	void UpdateCurrentShader(const ISkyLight* skyLight) const {}
};


struct SMFRenderStateARB: public ISMFRenderState {
public:
	bool Init(const CSMFGroundDrawer* smfGroundDrawer);
	void Kill();
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
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
	bool Init(const CSMFGroundDrawer* smfGroundDrawer);
	void Kill();
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
	bool CanDrawDeferred() const { return true; }
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
};

#endif

