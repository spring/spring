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
	void UpdateSpecularTexture();

	unsigned int GetEnvReflectionTextureID() const { return envReflectionTexID; }
	unsigned int GetSkyReflectionTextureID() const { return skyReflectionTexID; }
	unsigned int GetSpecularTextureID() const { return specularTexID; }
	unsigned int GetReflectionTextureSize() const { return reflTexSize; }
	unsigned int GetSpecularTextureSize() const { return specTexSize; }

private:
	void CreateReflectionFace(unsigned int, const float3&, bool);
	inline void CreateSpecularFacePart(unsigned int, int, const float3&, const float3&, const float3&, int, unsigned char *);
	void CreateSpecularFace(unsigned int, int, const float3&, const float3&, const float3&);
	void UpdateSpecularFace(unsigned int, int, const float3&, const float3&, const float3&, int, unsigned char *);

	unsigned int envReflectionTexID; // sky and map
	unsigned int skyReflectionTexID; // sky only
	unsigned int specularTexID;

	unsigned int reflTexSize;
	unsigned int specTexSize;

	unsigned int currReflectionFace;
	bool mapSkyReflections;

	FBO reflectionCubeFBO;
	float specExp;
	int specularUpdateIter;
	unsigned char* specTexBuf;
};

extern CubeMapHandler *cubeMapHandler;

#endif
