/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SkyBox.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"
#include "System/float3.h"
#include "System/Log/ILog.h"

#define LOG_SECTION_SKY_BOX "SkyBox"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SKY_BOX)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SKY_BOX


CSkyBox::CSkyBox(const std::string& texture)
{
#ifndef HEADLESS
	CBitmap btex;
	if (!btex.Load(texture) || btex.textype != GL_TEXTURE_CUBE_MAP) {
		LOG_L(L_WARNING, "could not load skybox texture from file %s", texture.c_str());
	} else {
		skyTex.SetRawTexID(btex.CreateTexture());
		skyTex.SetRawSize(int2(btex.xsize, btex.ysize));
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
#endif
	globalRendering->drawFog = (fogStart <= 0.99f);
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Draw()
{
	SCOPED_GMARKER("CSkyBox::Draw");

	if (!globalRendering->drawSky)
		return;

	glColor3f(1,1,1);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());

	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, 1, 0, 1);

	GLfloat verts[] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f
	};
	float3 texcoords[] = {
		-camera->CalcPixelDir(                         0,                          0),
		-camera->CalcPixelDir(globalRendering->viewSizeX,                          0),
		-camera->CalcPixelDir(globalRendering->viewSizeX, globalRendering->viewSizeY),
		-camera->CalcPixelDir(                         0, globalRendering->viewSizeY)
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(3, GL_FLOAT, 0, texcoords);
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	sky->SetupFog();
}
