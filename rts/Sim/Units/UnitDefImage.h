/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DEF_IMAGE
#define UNIT_DEF_IMAGE

#include "creg/creg_cond.h"
#include "Rendering/GL/myGL.h"

struct UnitDefImage
{
	CR_DECLARE_STRUCT(UnitDefImage);
	UnitDefImage() { imageSizeX = -1; imageSizeY = -1; }
	void Free() {
		if (textureOwner) {
			glDeleteTextures(1, &textureID);
		}
	}

	int imageSizeX;
	int imageSizeY;
	GLuint textureID;
	bool textureOwner;
};

#endif // UNIT_DEF_IMAGE
