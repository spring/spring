/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _PATH_TEXTURE_H
#define _PATH_TEXTURE_H

#include "PboInfoTexture.h"
#include "Rendering/GL/FBO.h"
#include "System/Misc/SpringTime.h"


class CPathTexture : public CPboInfoTexture
{
public:
	CPathTexture();

public:
	void Update();
	bool IsUpdateNeeded();

	GLuint GetTexture();

private:
	int updateFrame;
	int updateProcess;
	unsigned int lastSelectedPathType;
	spring_time lastUsage;
	FBO fbo;
};

#endif // _PATH_TEXTURE_H
