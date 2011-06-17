/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADOW_HANDLER_H
#define SHADOW_HANDLER_H

#include <vector>

#include "Rendering/GL/FBO.h"
#include "System/float4.h"
#include "System/Matrix44f.h"

namespace Shader {
	struct IProgramObject;
};

class CShadowHandler
{
public:
	CShadowHandler();
	~CShadowHandler();

	void CreateShadows();
	void CalcMinMaxView();

	const float4& GetShadowParams() const { return shadowProjCenter; }

	enum ShadowGenProgram {
		SHADOWGEN_PROGRAM_MODEL      = 0,
		SHADOWGEN_PROGRAM_MAP        = 1,
		SHADOWGEN_PROGRAM_TREE_NEAR  = 2,
		SHADOWGEN_PROGRAM_TREE_FAR   = 3,
		SHADOWGEN_PROGRAM_PROJECTILE = 4,
		SHADOWGEN_PROGRAM_LAST       = 5,
	};

	Shader::IProgramObject* GetShadowGenProg(ShadowGenProgram p) {
		return shadowGenProgs[p];
	}

protected:
	void GetFrustumSide(float3& side, bool upside);
	bool InitDepthTarget();
	void DrawShadowPasses();
	void LoadShadowGenShaderProgs();
	void SetShadowMapSizeFactors();

public:
	int shadowMapSize;
	unsigned int shadowTexture;
	unsigned int dummyColorTexture;

	static bool shadowsSupported;

	bool shadowsLoaded;
	bool inShadowPass;
	bool drawTerrainShadow;

	float3 centerPos;
	float3 sunDirX;
	float3 sunDirY;
	float3 sunDirZ;

	CMatrix44f shadowMatrix;

protected:
	struct fline {
		float base;
		float dir;
		int left;
		float minz;
		float maxz;
	};
	std::vector<fline> left;
	FBO fb;

	static bool firstInstance;

	//! these project geometry into light-space
	//! to write the (FBO) depth-buffer texture
	std::vector<Shader::IProgramObject*> shadowGenProgs;

	/// x1, x2, y1, y2
	float4 shadowProjMinMax;
	/// xmid, ymid, p17, p18
	float4 shadowProjCenter;
};

extern CShadowHandler* shadowHandler;

#endif /* SHADOW_HANDLER_H */
