#ifndef UNIT_IMAGE
#define UNIT_IMAGE

#include "StdAfx.h"
#include "creg/creg.h"
#include "Rendering/GL/myGL.h"

struct UnitImage
{
	CR_DECLARE_STRUCT(UnitImage);
	UnitImage() { imageSizeX = -1; imageSizeY = -1; }

	std::string buildPicName;
	int imageSizeX;
	int imageSizeY;
	GLuint textureID;
};

#endif
