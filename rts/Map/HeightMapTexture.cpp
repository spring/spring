/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "HeightMapTexture.h"

#include "ReadMap.h"
#include "ConfigHandler.h"


HeightMapTexture heightMapTexture;


HeightMapTexture::HeightMapTexture()
{
	init = false;

	texID = 0;

	xSize = 0;
	ySize = 0;
}


HeightMapTexture::~HeightMapTexture()
{
}


void HeightMapTexture::Init()
{
	if (init || (readmap == NULL)) {
		return;
	}
	init = true;

	if (!configHandler->Get("HeightMapTexture", 1)) {
		return;
	}

	if (!GLEW_ARB_texture_float ||
	    !GLEW_ARB_texture_non_power_of_two) {
		return;
	}

	xSize = (gs->mapx + 1);
	ySize = (gs->mapy + 1);
	
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	const float* heightMap = readmap->GetHeightmap();

	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB,
		xSize, ySize, 0,
		GL_LUMINANCE, GL_FLOAT, heightMap);

	glBindTexture(GL_TEXTURE_2D, 0);
}


void HeightMapTexture::Kill()
{
	glDeleteTextures(1, &texID);
	texID = 0;
	xSize = 0;
	ySize = 0;
}


void HeightMapTexture::UpdateArea(int x0, int z0, int x1, int z1)
{
	if (texID == 0) {
		return;
	}
	const float* heightMap = readmap->GetHeightmap();

	const int sizeX = x1 - x0 + 1;
	const int sizeZ = z1 - z0 + 1;

	pbo.Bind();
	pbo.Resize( sizeX * sizeZ * sizeof(float) );
	float* buf = (float*)pbo.MapBuffer();
		for (int z = 0; z < sizeZ; z++) {
			void* dst = buf + z * sizeX;
			const void* src = heightMap + x0 + (z + z0) * xSize;
			memcpy(dst, src, sizeX * sizeof(float));
		}
	pbo.UnmapBuffer();

	glBindTexture(GL_TEXTURE_2D, texID);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		x0, z0, sizeX, sizeZ,
		GL_LUMINANCE, GL_FLOAT, pbo.GetPtr());
	pbo.Unbind();
}

