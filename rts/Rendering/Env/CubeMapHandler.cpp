#include "Game/Camera.h"
#include "Game/Game.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "System/ConfigHandler.h"

CubeMapHandler* CubeMapHandler::GetInstance() {
	static CubeMapHandler cmh;
	return &cmh;
}

CubeMapHandler::CubeMapHandler() {
	reflectionTexID = 0;
	specularTexID = 0;

	reflTexSize = 0;
	specTexSize = 0;

	currReflectionFace = 0;
}

bool CubeMapHandler::Init() {
	specTexSize = configHandler->Get("CubeTexSizeSpecular", 128);
	reflTexSize = configHandler->Get("CubeTexSizeReflection", 128);

	glGenTextures(1, &specularTexID);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTexID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, specTexSize, float3( 1, 1, 1), float3( 0, 0,-2), float3(0,-2, 0), 100.0f);
	CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, specTexSize, float3(-1, 1,-1), float3( 0, 0, 2), float3(0,-2, 0), 100.0f);
	CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, specTexSize, float3(-1 ,1,-1), float3( 2, 0, 0), float3(0, 0, 2), 100.0f);
	CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, specTexSize, float3(-1,-1, 1), float3( 2, 0, 0), float3(0, 0,-2), 100.0f);
	CreateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, specTexSize, float3(-1, 1, 1), float3( 2, 0, 0), float3(0,-2, 0), 100.0f);
	CreateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, specTexSize, float3( 1, 1,-1), float3(-2, 0, 0), float3(0,-2, 0), 100.0f);


	glGenTextures(1, &reflectionTexID);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, reflectionTexID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA,GL_UNSIGNED_BYTE, 0);

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
	if (reflectionTexID != 0) {
		glDeleteTextures(1, &reflectionTexID);
		reflectionTexID = 0;
	}
}



void CubeMapHandler::UpdateReflectionTexture(void)
{
	if (!unitDrawer->advShading)
		return;

	switch (currReflectionFace++) {
		case 0:
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, float3( 1, 0, 0));
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, float3(-1, 0, 0));
			break;
		case 1:
			break;
		case 2:
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, float3(0,  1, 0));
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, float3(0, -1, 0));
			break;
		case 3:
			break;
		case 4:
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, float3(0, 0,  1));
			CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, float3(0, 0, -1));
			break;
		case 5:
			currReflectionFace = 0;
			break;
		default:
			currReflectionFace = 0;
			break;
	}
}

void CubeMapHandler::CreateReflectionFace(unsigned int glType, const float3& camDir)
{
	reflectionCubeFBO.Bind();
	reflectionCubeFBO.AttachTexture(reflectionTexID, glType);

	glPushAttrib(GL_FOG_BIT);
	glViewport(0, 0, reflTexSize, reflTexSize);
	glClear(GL_DEPTH_BUFFER_BIT);

	char realCam[sizeof(CCamera)];
	new (realCam) CCamera(*camera); // anti-crash workaround for multithreading

	game->SetDrawMode(CGame::reflectionDraw);

	camera->SetFov(90.0f);
	camera->forward = camDir;
	camera->up = -UpVector;

	if (camera->forward.y == 1.0f)
		camera->up = float3(0.0f, 0.0f, 1.0f);
	if (camera->forward.y == -1.0f)
		camera->up = float3(0.0f, 0.0f, -1.0f);

	camera->pos.y = ground->GetHeight(camera->pos.x, camera->pos.z) + 50.0f;
	camera->Update(false, false);

	sky->Draw();
	readmap->GetGroundDrawer()->Draw(false, true);

	//! we do this later to save render context switches (this is one of the slowest opengl operations!)
	// reflectionCubeFBO.Unbind();
	// glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
	glPopAttrib();

	game->SetDrawMode(CGame::normalDraw);

	camera->~CCamera();
	new (camera) CCamera(*(CCamera*) realCam);
	camera->Update(false);
}


void CubeMapHandler::CreateSpecularFace(
	unsigned int glType,
	int size,
	const float3& baseDir,
	const float3& xdif,
	const float3& ydif,
	float specExponent)
{
	unsigned char* buf = new unsigned char[size * size * 4];

	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			float3 vec = baseDir + (xdif * (x + 0.5f)) / size + (ydif * (y + 0.5f)) / size;
				vec.Normalize();
			float dot = vec.dot(mapInfo->light.sunDir);

			if (dot < 0.0f)
				dot = 0.0f;

			const float spec = std::min(1.f, pow(dot, specExponent) + pow(dot, 3) * 0.25f);

			buf[(y * size + x) * 4 + 0] = (unsigned char) (mapInfo->light.unitSpecularColor.x * spec * 255);
			buf[(y * size + x) * 4 + 1] = (unsigned char) (mapInfo->light.unitSpecularColor.y * spec * 255);
			buf[(y * size + x) * 4 + 2] = (unsigned char) (mapInfo->light.unitSpecularColor.z * spec * 255);
			buf[(y * size + x) * 4 + 3] = 255;
		}
	}

	glBuildMipmaps(glType, GL_RGBA8, size, size, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	delete[] buf;
}
