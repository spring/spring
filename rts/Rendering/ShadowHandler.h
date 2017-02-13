/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADOW_HANDLER_H
#define SHADOW_HANDLER_H

#include <vector>

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
	CShadowHandler() { Init(); }
	~CShadowHandler() { Kill(); }

	void Reload(const char* argv);

	void SetupShadowTexSampler(unsigned int texUnit, bool enable = false) const;
	void SetupShadowTexSamplerRaw() const;
	void ResetShadowTexSampler(unsigned int texUnit, bool disable = false) const;
	void ResetShadowTexSamplerRaw() const;
	void CreateShadows();

	enum ShadowGenerationBits {
		SHADOWGEN_BIT_NONE  = 0,
		SHADOWGEN_BIT_MAP   = 2,
		SHADOWGEN_BIT_MODEL = 4,
		SHADOWGEN_BIT_PROJ  = 8,
		SHADOWGEN_BIT_TREE  = 16,
	};
	enum ShadowProjectionMode {
		SHADOWPROMODE_MAP_CENTER = 0, // use center of map-geometry as projection target (constant res.)
		SHADOWPROMODE_CAM_CENTER = 1, // use center of camera-frustum as projection target (variable res.)
		SHADOWPROMODE_MIX_CAMMAP = 2, // use whichever mode maximizes resolution this frame
	};
	enum ShadowMapSizes {
		MIN_SHADOWMAP_SIZE =   512,
		DEF_SHADOWMAP_SIZE =  2048,
		MAX_SHADOWMAP_SIZE = 16384,
	};

	enum ShadowGenProgram {
		SHADOWGEN_PROGRAM_MODEL      = 0,
		SHADOWGEN_PROGRAM_MAP        = 1,
		SHADOWGEN_PROGRAM_TREE_NEAR  = 2,
		SHADOWGEN_PROGRAM_TREE_FAR   = 3,
		SHADOWGEN_PROGRAM_PROJECTILE = 4,
		SHADOWGEN_PROGRAM_LAST       = 5,
	};

	enum ShadowMatrixType {
		SHADOWMAT_TYPE_CULLING = 0,
		SHADOWMAT_TYPE_DRAWING = 1,
	};

	Shader::IProgramObject* GetShadowGenProg(ShadowGenProgram p) {
		return shadowGenProgs[p];
	}

	const CMatrix44f& GetShadowMatrix   (unsigned int idx = SHADOWMAT_TYPE_DRAWING) const { return  viewMatrix[idx];      }
	const      float* GetShadowMatrixRaw(unsigned int idx = SHADOWMAT_TYPE_DRAWING) const { return &viewMatrix[idx].m[0]; }

	const float4& GetShadowParams() const { return shadowTexProjCenter; }

	unsigned int GetShadowTextureID() const { return shadowTexture; }
	unsigned int GetColorTextureID() const { return dummyColorTexture; }

	static bool ShadowsInitialized() { return firstInit; }
	static bool ShadowsSupported() { return shadowsSupported; }

	bool ShadowsLoaded() const { return shadowsLoaded; }
	bool InShadowPass() const { return inShadowPass; }

private:
	void Init();
	void Kill();
	void FreeTextures();

	bool InitDepthTarget();
	bool WorkaroundUnsupportedFboRenderTargets();

	void DrawShadowPasses();
	void LoadShadowGenShaderProgs();

	void SetShadowMapSizeFactors();
	void SetShadowMatrix(CCamera* playerCam, CCamera* shadowCam);
	void SetShadowCamera(CCamera* shadowCam);

	float4 GetShadowProjectionScales(CCamera*, const float3&);

	float GetOrthoProjectedMapRadius(const float3&, float3&);
	float GetOrthoProjectedFrustumRadius(CCamera*, float3&);

public:
	int shadowConfig;
	int shadowMapSize;
	int shadowGenBits;
	int shadowProMode;

private:
	unsigned int shadowTexture;
	unsigned int dummyColorTexture;

	bool shadowsLoaded;
	bool inShadowPass;

	static bool firstInit;
	static bool shadowsSupported;

	// these project geometry into light-space
	// to write the (FBO) depth-buffer texture
	std::vector<Shader::IProgramObject*> shadowGenProgs;

	float3 projMidPos[2 + 1];
	float3 sunProjDir;

	float4 shadowProjScales;
	/// frustum bounding-rectangle corners; x1, x2, y1, y2
	float4 shadowProjMinMax;
	/// xmid, ymid, p17, p18
	float4 shadowTexProjCenter;

	// culling and drawing versions of both matrices
	CMatrix44f projMatrix[2];
	CMatrix44f viewMatrix[2];

	FBO fb;
};

extern CShadowHandler* shadowHandler;

#endif /* SHADOW_HANDLER_H */
