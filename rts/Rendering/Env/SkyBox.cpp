/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "SkyBox.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"
#include "System/float3.h"
#include "System/GlobalUnsynced.h"

CSkyBox::CSkyBox(std::string texture)
{
	CBitmap btex;
	if (!btex.Load(texture))
		throw content_error("Could not load skybox texture from file " + texture);
	tex = btex.CreateTexture(true);

	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	cloudDensity = mapInfo->atmosphere.cloudDensity;
	cloudColor = mapInfo->atmosphere.cloudColor;
	skyColor = mapInfo->atmosphere.skyColor;
	sunColor = mapInfo->atmosphere.sunColor;
	fogStart = mapInfo->atmosphere.fogStart;
	if (fogStart>0.99f) globalRendering->drawFog = false;
}

CSkyBox::~CSkyBox(void)
{
}

void CSkyBox::Draw()
{
	glColor3f(1,1,1);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0,1,0,1);

	GLfloat verts[] = {0.f, 1.f,
			   1.f, 1.f,
			   1.f, 0.f,
			   0.f, 0.f
	};
	float3 texcoords[] = {
		-camera->CalcPixelDir(0,0),
		-camera->CalcPixelDir(globalRendering->viewSizeX,0),
		-camera->CalcPixelDir(globalRendering->viewSizeX,globalRendering->viewSizeY),
		-camera->CalcPixelDir(0,globalRendering->viewSizeY)
	};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(3, GL_FLOAT, 0, texcoords);
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	//glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glFogf(GL_FOG_START,globalRendering->viewRange*fogStart);
	glFogf(GL_FOG_END,globalRendering->viewRange);
	glFogf(GL_FOG_DENSITY,1.00f);
	if (globalRendering->drawFog) {
		glEnable(GL_FOG);
	}
}

void CSkyBox::Update()
{
}

void CSkyBox::DrawSun(void)
{
}
