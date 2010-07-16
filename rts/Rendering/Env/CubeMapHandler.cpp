/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/Camera.h"
#include "Game/Game.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "System/ConfigHandler.h"

static char cameraMemBuf[sizeof(CCamera)];

CubeMapHandler* CubeMapHandler::GetInstance() {
	static CubeMapHandler cmh;
	return &cmh;
}

CubeMapHandler::CubeMapHandler() {
	envReflectionTexID = 0;
	skyReflectionTexID = 0;
	specularTexID = 0;

	reflTexSize = 0;
	specTexSize = 0;

	currReflectionFace = 0;
	mapSkyReflections = false;
}

bool CubeMapHandler::Init() {
	specTexSize = configHandler->Get("CubeTexSizeSpecular", 128);
	reflTexSize = configHandler->Get("CubeTexSizeReflection", 128);

	mapSkyReflections = !(mapInfo->smf.skyReflectModTexName.empty());

	const float specExp = configHandler->Get("CubeTexSpecularExponent", 100.0f);

	{
		glGenTextures(1, &specularTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, specTexSize, float3( 1,  1,  1), float3( 0, 0, -2), float3(0, -2,  0), specExp);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, specTexSize, float3(-1,  1, -1), float3( 0, 0,  2), float3(0, -2,  0), specExp);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, specTexSize, float3(-1,  1, -1), float3( 2, 0,  0), float3(0,  0,  2), specExp);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, specTexSize, float3(-1, -1,  1), float3( 2, 0,  0), float3(0,  0, -2), specExp);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, specTexSize, float3(-1,  1,  1), float3( 2, 0,  0), float3(0, -2,  0), specExp);
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, specTexSize, float3( 1,  1, -1), float3(-2, 0,  0), float3(0, -2,  0), specExp);
	}

	{
		glGenTextures(1, &envReflectionTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, envReflectionTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	if (mapSkyReflections) {
		glGenTextures(1, &skyReflectionTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, skyReflectionTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}


	if (reflectionCubeFBO.IsValid()) {
		reflectionCubeFBO.Bind();
		reflectionCubeFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT, reflTexSize, reflTexSize);
		reflectionCubeFBO.Unbind();
	}

	if (!reflectionCubeFBO.IsValid()) {
		Free();
		return false;
	}

	return true;
}

void CubeMapHandler::Free() {
	if (specularTexID != 0) {
		glDeleteTextures(1, &specularTexID);
		specularTexID = 0;
	}
	if (envReflectionTexID != 0) {
		glDeleteTextures(1, &envReflectionTexID);
		envReflectionTexID = 0;
	}
	if (skyReflectionTexID != 0) {
		glDeleteTextures(1, &skyReflectionTexID);
		skyReflectionTexID = 0;
	}
}



void CubeMapHandler::UpdateReflectionTexture(void)
{
	if (!unitDrawer->advShading) {
		return;
	}

	switch (currReflectionFace++) {
		case 0: {
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, float3( 1.0f, 0.0f, 0.0f), false);
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, float3(-1.0f, 0.0f, 0.0f), false);

			if (mapSkyReflections) {
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, float3( 1.0f, 0.0f, 0.0f), true);
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, float3(-1.0f, 0.0f, 0.0f), true);
			}
		} break;
		case 1: {
		} break;
		case 2: {
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,  UpVector, false);
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, -UpVector, false);

			if (mapSkyReflections) {
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,  UpVector, true);
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, -UpVector, true);
			}
		} break;
		case 3: {
		} break;
		case 4: {
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, float3(0.0f, 0.0f,  1.0f), false);
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, float3(0.0f, 0.0f, -1.0f), false);

			if (mapSkyReflections) {
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, float3(0.0f, 0.0f,  1.0f), true);
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, float3(0.0f, 0.0f, -1.0f), true);
			}
		} break;
		case 5: {
			currReflectionFace = 0;
		} break;
		default: {
			currReflectionFace = 0;
		} break;
	}
}

void CubeMapHandler::CreateReflectionFace(unsigned int glType, const float3& camDir, bool skyOnly)
{
	reflectionCubeFBO.Bind();
	reflectionCubeFBO.AttachTexture((skyOnly? skyReflectionTexID: envReflectionTexID), glType);

	glPushAttrib(GL_FOG_BIT);
	glViewport(0, 0, reflTexSize, reflTexSize);
	glClear(GL_DEPTH_BUFFER_BIT);

	new (cameraMemBuf) CCamera(*camera); // anti-crash workaround for multithreading

	game->SetDrawMode(CGame::gameReflectionDraw);

	camera->SetFov(90.0f);
	camera->forward = camDir;
	camera->up = -UpVector;

	if (camera->forward.y ==  1.0f) { camera->up = float3(0.0f, 0.0f,  1.0f); }
	if (camera->forward.y == -1.0f) { camera->up = float3(0.0f, 0.0f, -1.0f); }

	camera->pos.y = ground->GetHeight(camera->pos.x, camera->pos.z) + 50.0f;
	camera->Update(false, false);

	sky->Draw();

	if (!skyOnly) {
		readmap->GetGroundDrawer()->Draw(false, true);
	}

	//! we do this later to save render context switches (this is one of the slowest opengl operations!)
	// reflectionCubeFBO.Unbind();
	// glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glPopAttrib();

	game->SetDrawMode(CGame::gameNormalDraw);

	camera->~CCamera();
	new (camera) CCamera(*(CCamera*) cameraMemBuf);
	camera->Update(false);
}


void CubeMapHandler::CreateSpecularFace(
	unsigned int texType,
	int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif,
	float specExponent)
{
	unsigned char* buf = new unsigned char[size * size * 4];

	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			const float3 dir = (cdir + (xdif * (x + 0.5f)) / size + (ydif * (y + 0.5f)) / size).Normalize();
			const float dot = std::max(0.0f, dir.dot(mapInfo->light.sunDir));
			const float spec = std::min(1.0f, pow(dot, specExponent) + pow(dot, 3.0f) * 0.25f);

			buf[(y * size + x) * 4 + 0] = (unsigned char) (mapInfo->light.unitSpecularColor.x * spec * 255);
			buf[(y * size + x) * 4 + 1] = (unsigned char) (mapInfo->light.unitSpecularColor.y * spec * 255);
			buf[(y * size + x) * 4 + 2] = (unsigned char) (mapInfo->light.unitSpecularColor.z * spec * 255);
			buf[(y * size + x) * 4 + 3] = 255;
		}
	}

	//! note: no mipmaps, cubemap linear filtering is broken
	glTexImage2D(texType, 0, GL_RGBA8, size, size, 0, GL_RGBA,GL_UNSIGNED_BYTE, buf);
	delete[] buf;
}
