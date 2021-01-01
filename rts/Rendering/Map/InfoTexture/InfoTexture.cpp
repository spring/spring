/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InfoTexture.h"


CInfoTexture::CInfoTexture()
: texture(0)
, texSize(0, 0)
, texChannels(0)
{
}


CInfoTexture::CInfoTexture(const std::string& _name, GLuint _texture, int2 _texSize)
: texture(_texture)
, name(_name)
, texSize(_texSize)
, texChannels(4)
{
}
