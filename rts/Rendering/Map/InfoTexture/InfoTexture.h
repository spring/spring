/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _INFO_TEXTURE_H
#define _INFO_TEXTURE_H

#include "Rendering/GL/myGL.h"
#include "System/type2.h"
#include <string>


class CInfoTexture
{
public:
	CInfoTexture();
	CInfoTexture(const std::string& name, GLuint texture, int2 texSize);
	virtual ~CInfoTexture() {}

public:
	virtual GLuint GetTexture() { return texture; }

	int2 GetTexSize()     const { return texSize; }
	int  GetTexChannels() const { return texChannels; }

	const std::string& GetName() const { return name; }

protected:
	friend class IInfoTextureHandler;

	GLuint texture;
	std::string name;
	int2 texSize;
	int texChannels;
};

class CDummyInfoTexture: public CInfoTexture {
public:
	CDummyInfoTexture(): CInfoTexture("dummy", 0, int2(0, 0)) {}
};

#endif // _INFO_TEXTURE_H
