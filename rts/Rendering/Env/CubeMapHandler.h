/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CUBEMAP_HANDLER_HDR
#define CUBEMAP_HANDLER_HDR

#include "Rendering/GL/FBO.h"

class CubeMapHandler {
public:
	CubeMapHandler();

	bool Init();
	void Free();

	void UpdateReflectionTexture();

	unsigned int GetReflectionTextureID() const { return reflectionTexID; }
	unsigned int GetSpecularTextureID() const { return specularTexID; }
	unsigned int GetReflectionTextureSize() const { return reflTexSize; }
	unsigned int GetSpecularTextureSize() const { return specTexSize; }

	static CubeMapHandler* GetInstance();

private:
	void CreateReflectionFace(unsigned int, const float3&);
	void CreateSpecularFace(unsigned int, int, const float3&, const float3&, const float3&, float);

	unsigned int reflectionTexID;
	unsigned int specularTexID;

	unsigned int reflTexSize;
	unsigned int specTexSize;

	unsigned int currReflectionFace;

	FBO reflectionCubeFBO;
};

#define cubeMapHandler (CubeMapHandler::GetInstance())

#endif
