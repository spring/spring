#include "StdAfx.h"
// HeightMapTexture.cpp: implementation of the HeightMapTexture class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "HeightMapTexture.h"

#include "ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "ConfigHandler.h"

/******************************************************************************/
/******************************************************************************/


HeightMapTexture heightMapTexture;


HeightMapTexture::HeightMapTexture()
{
	init = false;

	texID = 0;

	xSize = 0;
	ySize = 0;
}


HeightMapTexture::~HeightMapTexture()
{
}


void HeightMapTexture::Init()
{
	if (init || (readmap == NULL)) {
		return;
	}
	init = true;

	if (!configHandler.Get("HeightMapTexture", 1)) {
		return;
	}

	if (!GLEW_ARB_texture_float ||
	    !GLEW_ARB_texture_non_power_of_two) {
		return;
	}

	xSize = (gs->mapx + 1);
	ySize = (gs->mapy + 1);
	
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	const float* heightMap = readmap->GetHeightmap();

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
}


void HeightMapTexture::UpdateArea(int x0, int z0, int x1, int z1)
{
	if (texID == 0) {
		return;
	}
	const float* heightMap = readmap->GetHeightmap();
	glBindTexture(GL_TEXTURE_2D, texID);

	for (int z = z0; z <= z1; z++) {
		glTexSubImage2D(GL_TEXTURE_2D, 0,
										x0, z, (x1 - x0 + 1), 1,
										GL_LUMINANCE, GL_FLOAT, heightMap + (x0 + (z * xSize)));
	}
	

	glBindTexture(GL_TEXTURE_2D, 0);
}


/******************************************************************************/
/******************************************************************************/
