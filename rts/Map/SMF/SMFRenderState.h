/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMF_RENDERSTATE_H
#define SMF_RENDERSTATE_H

#include "Map/MapDrawPassTypes.h"

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

private:
	Shader::IProgramObject* smfShaderBaseARB;   // default (V+F) SMF ARB shader
	Shader::IProgramObject* smfShaderReflARB;   // shader (V+F) for the DynamicWater reflection pass
	Shader::IProgramObject* smfShaderRefrARB;   // shader (V+F) for the DynamicWater refraction pass
	Shader::IProgramObject* smfShaderCurrARB;   // currently active ARB shader
};


struct SMFRenderStateGLSL: public ISMFRenderState {
public:
	bool Init(const CSMFGroundDrawer* smfGroundDrawer);
	void Kill();
	bool CanEnable(const CSMFGroundDrawer* smfGroundDrawer) const;
	void Enable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void Disable(const CSMFGroundDrawer* smfGroundDrawer, const DrawPass::e& drawPass);
	void SetSquareTexGen(const int sqx, const int sqy) const;
	void SetCurrentShader(const DrawPass::e& drawPass) {}
	void UpdateCurrentShader(const ISkyLight* skyLight) const;

private:
	enum {
		SMF_SHADOWS_OPT_BIT = (1 << 0),
		SMF_INFOTEX_OPT_BIT = (1 << 1),
		SMF_MAX_OPT_BIT_VAL = (1 << 2),
	};

	Shader::IProgramObject* smfShadersGLSL[SMF_MAX_OPT_BIT_VAL];
	Shader::IProgramObject* smfShaderGLSL; // currently active GLSL shader
};

#endif

