/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "HeightMapTexture.h"

#include "ReadMap.h"
#include "System/EventHandler.h"
#include "System/Rectangle.h"
#include "System/Config/ConfigHandler.h"

#include <cstring>

CONFIG(bool, HeightMapTexture).defaultValue(true);

HeightMapTexture* heightMapTexture = NULL;
HeightMapTexture::HeightMapTexture()
	: CEventClient("[HeightMapTexture]", 2718965, false)
{
	texID = 0;
	xSize = 0;
	ySize = 0;

	eventHandler.AddClient(this);
	Init();
}


HeightMapTexture::~HeightMapTexture()
{
	Kill();
	eventHandler.RemoveClient(this);
}


void HeightMapTexture::Init()
{
	if (readMap == nullptr)
		return;

	if (!configHandler->GetBool("HeightMapTexture"))
		return;

	xSize = mapDims.mapxp1;
	ySize = mapDims.mapyp1;

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	const float* heightMap = readMap->GetCornerHeightMapUnsynced();

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
	pbo.Release();
}


void HeightMapTexture::UnsyncedHeightMapUpdate(const SRectangle& rect)
{
	if (texID == 0) {
		return;
	}
	const float* heightMap = readMap->GetCornerHeightMapUnsynced();

	const int sizeX = rect.x2 - rect.x1 + 1;
	const int sizeZ = rect.z2 - rect.z1 + 1;

	pbo.Bind();
	pbo.New(sizeX * sizeZ * sizeof(float));

	{
		float* buf = (float*) pbo.MapBuffer();
		for (int z = 0; z < sizeZ; z++) {
			const void* src = heightMap + rect.x1 + (z + rect.z1) * xSize;
			      void* dst = buf + z * sizeX;

			memcpy(dst, src, sizeX * sizeof(float));
		}
		pbo.UnmapBuffer();
	}

	glBindTexture(GL_TEXTURE_2D, texID);
	glTexSubImage2D(GL_TEXTURE_2D, 0,
		rect.x1, rect.z1, sizeX, sizeZ,
		GL_LUMINANCE, GL_FLOAT, pbo.GetPtr());

	pbo.Invalidate();
	pbo.Unbind();
}
