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
	void CreateReflectionFace(unsigned int, const float3&, bool);

	unsigned int envReflectionTexID; // sky and map
	unsigned int skyReflectionTexID; // sky only

	unsigned int reflectionTexSize;
	unsigned int currReflectionFace;

	bool mapSkyReflections;

	FBO reflectionCubeFBO;
};

extern CubeMapHandler cubeMapHandler;

#endif
