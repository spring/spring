/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Metal.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"



CMetalTexture::CMetalTexture()
: CPboInfoTexture("metal")
, CEventClient("[CMetalTexture]", 271990, false)
, metalMapChanged(true)
{
	eventHandler.AddClient(this);
	texSize = int2(mapDims.hmapx, mapDims.hmapy);
	texChannels = 1;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, texSize.x, texSize.y);
}


void CMetalTexture::Update()
{
	assert(metalMap.GetSizeX() == texSize.x && metalMap.GetSizeZ() == texSize.y);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RED, GL_UNSIGNED_BYTE, metalMap.GetDistributionMap());

	metalMapChanged = false;
}

