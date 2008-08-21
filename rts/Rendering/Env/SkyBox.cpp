#include "StdAfx.h"
#include "mmgr.h"

#include "SkyBox.h"
#include "float3.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "GlobalStuff.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/UnitModels/UnitDrawer.h"

CSkyBox::CSkyBox(std::string texture)
{
	CBitmap btex;
	if (!btex.Load(texture))
		throw content_error("Could not load skybox texture from file " + texture);
	tex = btex.CreateTexture(0);

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
	//glTranslatef(camera->pos.x, camera->pos.y, camera->pos.z);
	//glCallList(displist);

	glDisable(GL_FOG);
	glColor3f(1,1,1);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_CUBE_MAP_ARB);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, tex);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);

	float3 v1 = camera->CalcPixelDir(0,0);
	float3 v2 = camera->CalcPixelDir(gu->viewSizeX,0);
	float3 v3 = camera->CalcPixelDir(gu->viewSizeX,gu->viewSizeY);
	float3 v4 = camera->CalcPixelDir(0,gu->viewSizeY);

	glBegin(GL_QUADS);
			
			glTexCoord3f(-v1.x,-v1.y,-v1.z);
			//glNormal3f(0,1,0);
			glVertexf3(camera->pos + v1*(NEAR_PLANE+5));

			glTexCoord3f(-v2.x,-v2.y,-v2.z);
			//glNormal3f(1,1,0);
			glVertexf3(camera->pos + v2*(NEAR_PLANE+5));

			glTexCoord3f(-v3.x,-v3.y,-v3.z);
			//glNormal3f(0,1,1);
			glVertexf3(camera->pos + v3*(NEAR_PLANE+5));

			glTexCoord3f(-v4.x,-v4.y,-v4.z);
			//glNormal3f(1,1,1);
			glVertexf3(camera->pos + v4*(NEAR_PLANE+5));
		glEnd();

	glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glDepthMask(1);
	glDisable(GL_ALPHA_TEST);
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
