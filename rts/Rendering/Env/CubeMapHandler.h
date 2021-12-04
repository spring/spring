/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CUBEMAP_HANDLER_HDR
#define CUBEMAP_HANDLER_HDR

#include "Rendering/GL/FBO.h"

class CubeMapHandler {
public:
	CubeMapHandler(): reflectionCubeFBO(true) {}

	bool Init();
	void Free();

	void UpdateReflectionTexture();

	unsigned int GetEnvReflectionTextureID() const { return envReflectionTexID; }
	unsigned int GetSkyReflectionTextureID() const { return skyReflectionTexID; }
	unsigned int GetReflectionTextureSize() const { return reflectionTexSize; }

private:
	void CreateReflectionFace(unsigned int, bool);

	unsigned int envReflectionTexID; // sky and map
	unsigned int skyReflectionTexID; // sky only

	unsigned int reflectionTexSize;
	unsigned int currReflectionFace;

	bool mapSkyReflections;
	bool generateMipMaps;

	FBO reflectionCubeFBO;

	const float3 faceDirs[6][3] = {
		{ RgtVector,  FwdVector,   UpVector}, // fwd = +x, right = +z, up = +y
		{-RgtVector, -FwdVector,   UpVector}, // fwd = -x
		{  UpVector,  RgtVector, -FwdVector}, // fwd = +y
		{ -UpVector,  RgtVector,  FwdVector}, // fwd = -y
		{ FwdVector, -RgtVector,   UpVector}, // fwd = +z
		{-FwdVector,  RgtVector,   UpVector}, // fwd = -z
	};
	const float4 faceColors[6] = {
		{1.0f, 0.0f, 0.0f, 1.0f}, // red
		{0.0f, 1.0f, 0.0f, 1.0f}, // green
		{0.0f, 0.0f, 1.0f, 1.0f}, // blue
		{1.0f, 1.0f, 0.0f, 1.0f}, // yellow
		{1.0f, 0.0f, 1.0f, 1.0f}, // purple
		{0.0f, 1.0f, 1.0f, 1.0f}, // cyan
	};
};

extern CubeMapHandler cubeMapHandler;

#endif
