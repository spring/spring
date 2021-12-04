/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADOW_HANDLER_H
#define SHADOW_HANDLER_H

#include <array>

#include "Rendering/GL/FBO.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

namespace Shader {
	struct IProgramObject;
}

class CCamera;
class CShadowHandler
{
public:
	CShadowHandler(): shadowMapFBO(true) {}

	void Init();
	void Kill();
	void Reload(const char* argv);

	void SetupShadowTexSampler(unsigned int texUnit) const;
	void SetupShadowTexSamplerRaw() const;
	void ResetShadowTexSampler(unsigned int texUnit) const;
	void ResetShadowTexSamplerRaw() const;
	void CreateShadows();

	enum ShadowGenerationBits {
		SHADOWGEN_BIT_NONE  = 0,
		SHADOWGEN_BIT_MAP   = 2,
		SHADOWGEN_BIT_MODEL = 4,
		SHADOWGEN_BIT_PROJ  = 8,
		SHADOWGEN_BIT_TREE  = 16,
	};
	enum ShadowMapSizes {
		MIN_SHADOWMAP_SIZE =   512,
		DEF_SHADOWMAP_SIZE =  2048,
		MAX_SHADOWMAP_SIZE = 16384,
	};

	enum ShadowGenProgram {
		SHADOWGEN_PROGRAM_MODEL      = 0,
		SHADOWGEN_PROGRAM_MAP        = 1,
		SHADOWGEN_PROGRAM_TREE       = 2,
		SHADOWGEN_PROGRAM_PROJECTILE = 3,
		SHADOWGEN_PROGRAM_PARTICLE   = 4,
		SHADOWGEN_PROGRAM_LAST       = 5,
	};

	enum ShadowMatrixType {
		SHADOWMAT_TYPE_CULLING = 0,
		SHADOWMAT_TYPE_DRAWING = 1,
	};

	Shader::IProgramObject* GetCurrentShadowGenProg() { return (GetShadowGenProg(ShadowGenProgram(currentShadowPass))); }
	Shader::IProgramObject* GetShadowGenProg(ShadowGenProgram p) { return shadowGenProgs[p]; }

	const CMatrix44f& GetShadowViewMatrix   (unsigned int idx = SHADOWMAT_TYPE_DRAWING) const { return  viewMatrix[idx];      }
	const CMatrix44f& GetShadowProjMatrix   (unsigned int idx = SHADOWMAT_TYPE_DRAWING) const { return  projMatrix[idx];      }
	const      float* GetShadowViewMatrixRaw(unsigned int idx = SHADOWMAT_TYPE_DRAWING) const { return &viewMatrix[idx].m[0]; }
	const      float* GetShadowProjMatrixRaw(unsigned int idx = SHADOWMAT_TYPE_DRAWING) const { return &projMatrix[idx].m[0]; }

	const float4& GetShadowParams() const { return shadowProjParams; }

	unsigned int GetShadowTextureID() const { return shadowTexture; }
	unsigned int GetColorTextureID() const { return dummyColorTexture; }
	unsigned int GetCurrentPass() const { return currentShadowPass; }

	static bool ShadowsInitialized() { return firstInit; }
	static bool ShadowsSupported() { return shadowsSupported; }

	bool ShadowsLoaded() const { return shadowsLoaded; }
	bool InShadowPass() const { return inShadowPass; }

private:
	void DrawShadowPasses();
	void FreeTextures();

	bool InitDepthTarget();
	bool WorkaroundUnsupportedFboRenderTargets();

	void LoadProjectionMatrix(const CCamera* shadowCam);
	void LoadShadowGenShaders();

	void SetShadowMatrix(CCamera* playerCam);
	void SetShadowCamera(CCamera* shadowCam);

	float4 GetShadowProjectionScales(CCamera*, const CMatrix44f&);
	float3 CalcShadowProjectionPos(CCamera*, float3*) const;

	float GetOrthoProjectedFrustumRadius(CCamera*, const CMatrix44f&, float3&) const;

public:
	int shadowConfig = 0;
	int shadowMapSize = 0;
	int shadowGenBits = 0;

private:
	unsigned int shadowTexture = 0;
	unsigned int dummyColorTexture = 0;
	unsigned int currentShadowPass = SHADOWGEN_PROGRAM_LAST;

	bool shadowsLoaded = false;
	bool inShadowPass = false;

	static bool firstInit;
	static bool shadowsSupported;

	// these project geometry into light-space
	// to write the (FBO) depth-buffer texture
	std::array<Shader::IProgramObject*, SHADOWGEN_PROGRAM_LAST> shadowGenProgs;

	float3 projMidPos;
	float3 sunProjDir;

	float4 shadowProjScales;
	/// .xy := scale, .zw := bias (unused)
	float4 shadowProjParams = {0.5f, 0.5f, 0.5f, 0.5f};

	// culling and drawing versions of both matrices
	CMatrix44f projMatrix[2];
	CMatrix44f viewMatrix[2];
	CMatrix44f biasMatrix = {OnesVector * 0.5f,  RgtVector * 0.5f, UpVector * 0.5f, FwdVector * 0.5f};

	FBO shadowMapFBO;
};

extern CShadowHandler shadowHandler;

#endif /* SHADOW_HANDLER_H */
