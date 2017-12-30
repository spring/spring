/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PboInfoTexture.h"

CPboInfoTexture::~CPboInfoTexture()
{
	glDeleteTextures(1, &texture);
	infoTexPBO.Release();
}

