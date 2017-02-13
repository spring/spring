/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BasicWater.h"
#include "ISky.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "System/Log/ILog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBasicWater::CBasicWater()
{
	CBitmap waterTexBM;
	if (!waterTexBM.Load(mapInfo->water.texture)) {
		LOG_L(L_WARNING, "[%s] could not read water texture from file \"%s\"", __FUNCTION__, mapInfo->water.texture.c_str());

		// fallback
		waterTexBM.AllocDummy(SColor(0,0,255,255));
	}

	// create mipmapped texture
	textureID = waterTexBM.CreateMipMapTexture();
	displistID = GenWaterQuadsList(waterTexBM.xsize, waterTexBM.ysize);
}

CBasicWater::~CBasicWater()
{
	glDeleteTextures(1, &textureID);
	glDeleteLists(displistID, 1);
}

unsigned int CBasicWater::GenWaterQuadsList(unsigned int textureWidth, unsigned int textureHeight) const
{
	unsigned int listID = glGenLists(1);

	glNewList(listID, GL_COMPILE);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(0);
	glColor4f(0.7f, 0.7f, 0.7f, 0.5f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glBegin(GL_QUADS);

	const float mapSizeX = mapDims.mapx * SQUARE_SIZE;
	const float mapSizeY = mapDims.mapy * SQUARE_SIZE;

	// Calculate number of times texture should repeat over the map,
	// taking aspect ratio into account.
	float repeatX = 65536.0f / mapDims.mapx;
	float repeatY = 65536.0f / mapDims.mapy * textureWidth / textureHeight;

	// Use better repeat setting of 1 repeat per 4096 mapx/mapy for the new
	// ocean.jpg while retaining backward compatibility with old maps relying
	// on 1 repeat per 1024 mapx/mapy. (changed 16/05/2007)
	if (mapInfo->water.texture == "bitmaps/ocean.jpg") {
		repeatX /= 4;
		repeatY /= 4;
	}

	repeatX = (mapInfo->water.repeatX != 0 ? mapInfo->water.repeatX : repeatX) / 16;
	repeatY = (mapInfo->water.repeatY != 0 ? mapInfo->water.repeatY : repeatY) / 16;

	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++) {
			glTexCoord2f(x       * repeatX,  y      * repeatY); glVertex3f(x       * mapSizeX/16, 0,  y      * mapSizeY/16);
			glTexCoord2f(x       * repeatX, (y + 1) * repeatY); glVertex3f(x       * mapSizeX/16, 0, (y + 1) * mapSizeY/16);
			glTexCoord2f((x + 1) * repeatX, (y + 1) * repeatY); glVertex3f((x + 1) * mapSizeX/16, 0, (y + 1) * mapSizeY/16);
			glTexCoord2f((x + 1) * repeatX,  y      * repeatY); glVertex3f((x + 1) * mapSizeX/16, 0,  y      * mapSizeY/16);
		}
	}

	glEnd();
	glDisable(GL_TEXTURE_2D);
	glDepthMask(1);
	glEndList();

	return listID;
}



void CBasicWater::Draw()
{
	if (!mapInfo->water.forceRendering && !readMap->HasVisibleWater())
		return;

	glPushAttrib(GL_FOG_BIT);
	sky->SetupFog();
	glCallList(displistID);
	glPopAttrib();
}
