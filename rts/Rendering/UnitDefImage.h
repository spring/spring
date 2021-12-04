/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DEF_IMAGE
#define UNIT_DEF_IMAGE

#include "System/creg/creg_cond.h"
#include "Rendering/GL/myGL.h"

struct UnitDefImage
{
	CR_DECLARE_STRUCT(UnitDefImage)

	UnitDefImage(): imageSizeX(-1), imageSizeY(-1), textureID(0) {
	}

	bool Free() {
		if (textureID != 0) {
			glDeleteTextures(1, &textureID);
			textureID = 0;
			return true;
		}
		return false;
	}

	int imageSizeX;
	int imageSizeY;
	GLuint textureID;
};

#endif // UNIT_DEF_IMAGE
