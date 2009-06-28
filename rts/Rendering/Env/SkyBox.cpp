#include "StdAfx.h"
#include "mmgr.h"

#include "SkyBox.h"
#include "float3.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "GlobalUnsynced.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Exceptions.h"

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
	if (fogStart>0.99f) gu->drawFog = false;
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

	float3 v1 = -camera->CalcPixelDir(0,0);
	float3 v2 = -camera->CalcPixelDir(gu->viewSizeX,0);
	float3 v3 = -camera->CalcPixelDir(gu->viewSizeX,gu->viewSizeY);
	float3 v4 = -camera->CalcPixelDir(0,gu->viewSizeY);

	glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0,1,0,1);

	glBegin(GL_QUADS);
		glTexCoord3f(v1.x,v1.y,v1.z);
		glVertex2f(0.0f,1.0f);

		glTexCoord3f(v2.x,v2.y,v2.z);
		glVertex2f(1.0f,1.0f);

		glTexCoord3f(v3.x,v3.y,v3.z);
		glVertex2f(1.0f,0.0f);

		glTexCoord3f(v4.x,v4.y,v4.z);
		glVertex2f(0.0f,0.0f);
	glEnd();

	//glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

	glDisable(GL_TEXTURE_CUBE_MAP_ARB);

	glDepthMask(1);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	if (gu->drawFog) {
		glFogfv(GL_FOG_COLOR,mapInfo->atmosphere.fogColor);
		glFogi(GL_FOG_MODE,GL_LINEAR);
		glFogf(GL_FOG_START,gu->viewRange*fogStart);
		glFogf(GL_FOG_END,gu->viewRange);
		glFogf(GL_FOG_DENSITY,1.00f);
		glEnable(GL_FOG);
	}
}

void CSkyBox::Update()
{
}

void CSkyBox::DrawSun(void)
{
}
