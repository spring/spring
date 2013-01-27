/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/Camera.h"
#include "Game/Game.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "System/Config/ConfigHandler.h"

CONFIG(int, CubeTexSizeSpecular).defaultValue(128).minimumValue(1);
CONFIG(int, CubeTexSizeReflection).defaultValue(128).minimumValue(1);

static char cameraMemBuf[sizeof(CCamera)];

CubeMapHandler* cubeMapHandler = NULL;

CubeMapHandler::CubeMapHandler() {
	envReflectionTexID = 0;
	skyReflectionTexID = 0;
	specularTexID = 0;

	reflTexSize = 0;
	specTexSize = 0;

	currReflectionFace = 0;
	specularTexIter = 0;
	mapSkyReflections = false;

	specTexBuf = NULL;
}

bool CubeMapHandler::Init() {
	specTexSize = configHandler->GetInt("CubeTexSizeSpecular");
	reflTexSize = configHandler->GetInt("CubeTexSizeReflection");
	specTexBuf = new unsigned char[specTexSize * 4];

	mapSkyReflections = !(mapInfo->smf.skyReflectModTexName.empty());

	{
		glGenTextures(1, &specularTexID);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTexID);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, specTexSize, float3( 1,  1,  1), float3( 0, 0, -2), float3(0, -2,  0));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, specTexSize, float3(-1,  1, -1), float3( 0, 0,  2), float3(0, -2,  0));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, specTexSize, float3(-1,  1, -1), float3( 2, 0,  0), float3(0,  0,  2));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, specTexSize, float3(-1, -1,  1), float3( 2, 0,  0), float3(0,  0, -2));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, specTexSize, float3(-1,  1,  1), float3( 2, 0,  0), float3(0, -2,  0));
		CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, specTexSize, float3( 1,  1, -1), float3(-2, 0,  0), float3(0, -2,  0));
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

	delete [] specTexBuf;
}



void CubeMapHandler::UpdateReflectionTexture()
{
	if (!unitDrawer->advShading) {
		return;
	}

	switch (currReflectionFace++) {
		case 0: {
			reflectionCubeFBO.Bind();
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, float3(0.0f, 0.0f, -1.0f), false);
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,  UpVector, false);

			if (mapSkyReflections) {
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, float3( 1.0f, 0.0f, 0.0f), true);
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, float3(-1.0f, 0.0f, 0.0f), true);
			}
		} break;
		case 1: {} break;
		case 2: {} break;
		case 3: {
			reflectionCubeFBO.Bind();
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, float3( 1.0f, 0.0f, 0.0f), false);
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, float3(-1.0f, 0.0f, 0.0f), false);

			if (mapSkyReflections) {
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,  UpVector, true);
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, -UpVector, true);
			}
		} break;
		case 4: {} break;
		case 5: {} break;
		case 6: {
			reflectionCubeFBO.Bind();
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, float3(0.0f, 0.0f,  1.0f), false);
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, -UpVector, false);

			if (mapSkyReflections) {
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, float3(0.0f, 0.0f,  1.0f), true);
				CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, float3(0.0f, 0.0f, -1.0f), true);
			}
		} break;
		case 7: {} break;
		case 8: {
			currReflectionFace = 0;
		} break;
		default: {
			currReflectionFace = 0;
		} break;
	}
}

void CubeMapHandler::CreateReflectionFace(unsigned int glType, const float3& camDir, bool skyOnly)
{
	reflectionCubeFBO.AttachTexture((skyOnly? skyReflectionTexID: envReflectionTexID), glType);

	glPushAttrib(GL_FOG_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, reflTexSize, reflTexSize);

	if (skyOnly) {
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
	} else {
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	}

	new (cameraMemBuf) CCamera(*camera); // anti-crash workaround for multi-threading

	game->SetDrawMode(CGame::gameReflectionDraw);

	camera->SetFov(90.0f);
	camera->forward = camDir;
	camera->up = -UpVector;

	if (camera->forward.y ==  1.0f) { camera->up = float3(0.0f, 0.0f,  1.0f); }
	if (camera->forward.y == -1.0f) { camera->up = float3(0.0f, 0.0f, -1.0f); }

	camera->pos.y = ground->GetHeightAboveWater(camera->pos.x, camera->pos.z, false) + 50.0f;
	camera->Update(false);

	sky->Draw();

	if (!skyOnly) {
		readmap->GetGroundDrawer()->Draw(DrawPass::UnitReflection);
	}

	// NOTE we do this later to save render context switches (this is one of the slowest OpenGL operations!)
	// reflectionCubeFBO.Unbind();
	// glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glPopAttrib();

	game->SetDrawMode(CGame::gameNormalDraw);

	camera->~CCamera();
	new (camera) CCamera(*reinterpret_cast<CCamera*>(cameraMemBuf));
	reinterpret_cast<CCamera*>(cameraMemBuf)->~CCamera();
	camera->Update();
}


void CubeMapHandler::UpdateSpecularTexture() {
	if (!unitDrawer->advShading) {
		return;
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTexID);

	int specularTexRow = specularTexIter / 3; //FIXME WTF

	switch (specularTexIter % 3) {
		case 0: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, specTexSize, float3( 1,  1,  1), float3( 0, 0, -2), float3(0, -2,  0), specularTexRow, specTexBuf);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, specTexSize, float3(-1,  1, -1), float3( 0, 0,  2), float3(0, -2,  0), specularTexRow, specTexBuf);
			break;
		}
		case 1: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, specTexSize, float3(-1,  1, -1), float3( 2, 0,  0), float3(0,  0,  2), specularTexRow, specTexBuf);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, specTexSize, float3(-1, -1,  1), float3( 2, 0,  0), float3(0,  0, -2), specularTexRow, specTexBuf);
			break;
		}
		case 2: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, specTexSize, float3(-1,  1,  1), float3( 2, 0,  0), float3(0, -2,  0), specularTexRow, specTexBuf);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, specTexSize, float3( 1,  1, -1), float3(-2, 0,  0), float3(0, -2,  0), specularTexRow, specTexBuf);
			break;
		}
	}

	// update one face of one row per frame
	++specularTexIter;
	specularTexIter = specularTexIter % (specTexSize * 3);
}

void CubeMapHandler::CreateSpecularFacePart(
	unsigned int texType,
	unsigned int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif,
	unsigned int y,
	unsigned char* buf)
{
	// TODO move to a shader
	for (int x = 0; x < size; ++x) {
		const float3 dir = (cdir + (xdif * (x + 0.5f)) / size + (ydif * (y + 0.5f)) / size).Normalize();
		const float dot  = std::max(0.0f, dir.dot(sky->GetLight()->GetLightDir()));
		const float spec = std::min(1.0f, math::pow(dot, mapInfo->light.specularExponent) + math::pow(dot, 3.0f) * 0.25f);

		buf[x * 4 + 0] = (mapInfo->light.unitSpecularColor.x * spec * 255);
		buf[x * 4 + 1] = (mapInfo->light.unitSpecularColor.y * spec * 255);
		buf[x * 4 + 2] = (mapInfo->light.unitSpecularColor.z * spec * 255);
		buf[x * 4 + 3] = 255;
	}
}

void CubeMapHandler::CreateSpecularFace(
	unsigned int texType,
	unsigned int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif)
{
	std::vector<unsigned char> buf(size * size * 4, 0);

	for (int y = 0; y < size; ++y) {
		CreateSpecularFacePart(texType, size, cdir, xdif, ydif, y, &buf[y * size * 4]);
	}

	//! note: no mipmaps, cubemap linear filtering is broken
	glTexImage2D(texType, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, &buf[0]);
}

void CubeMapHandler::UpdateSpecularFace(
	unsigned int texType,
	unsigned int size,
	const float3& cdir,
	const float3& xdif,
	const float3& ydif,
	unsigned int y,
	unsigned char* buf)
{
	CreateSpecularFacePart(texType, size, cdir, xdif, ydif, y, buf);

	glTexSubImage2D(texType, 0, 0, y, size, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
}

