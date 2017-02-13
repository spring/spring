/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PboInfoTexture.h"


CPboInfoTexture::CPboInfoTexture(const std::string& _name)
{
	name        = _name;
	texChannels = 0;
	texture     = 0;
}


CPboInfoTexture::~CPboInfoTexture()
{
	glDeleteTextures(1, &texture);
}
