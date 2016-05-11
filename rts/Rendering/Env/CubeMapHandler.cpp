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
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "System/Config/ConfigHandler.h"

CONFIG(int, CubeTexSizeSpecular).defaultValue(128).minimumValue(1);
CONFIG(int, CubeTexSizeReflection).defaultValue(128).minimumValue(1);

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
}

bool CubeMapHandler::Init() {
	specTexSize = configHandler->GetInt("CubeTexSizeSpecular");
	reflTexSize = configHandler->GetInt("CubeTexSizeReflection");
	specTexBuf.resize(specTexSize * 4, 0);

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
}



void CubeMapHandler::UpdateReflectionTexture()
{
	if (!unitDrawer->UseAdvShading())
		return;

	// NOTE:
	//   we unbind later in WorldDrawer::GenerateIBLTextures() to save render
	//   context switches (which are one of the slowest OpenGL operations!)
	//   together with VP restoration
	reflectionCubeFBO.Bind();

	switch (currReflectionFace) {
		case 0: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,  RgtVector, false); } break;
		case 1: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, -RgtVector, false); } break;
		case 2: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,  UpVector,  false); } break;
		case 3: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, -UpVector,  false); } break;
		case 4: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,  FwdVector, false); } break;
		case 5: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, -FwdVector, false); } break;
		default: {} break;
	}

	if (mapSkyReflections) {
		// draw only the sky (into its own cubemap) for SSMF
		// by reusing data from previous frame we could also
		// make terrain reflect itself, not just the sky
		switch (currReflectionFace) {
			case  6: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,  RgtVector, true); } break;
			case  7: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, -RgtVector, true); } break;
			case  8: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,  UpVector,  true); } break;
			case  9: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, -UpVector,  true); } break;
			case 10: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,  FwdVector, true); } break;
			case 11: { CreateReflectionFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, -FwdVector, true); } break;
			default: {} break;
		}

		currReflectionFace +=  1;
		currReflectionFace %= 12;
	} else {
		// touch the FBO at least once per frame
		currReflectionFace += 1;
		currReflectionFace %= 6;
	}
}

void CubeMapHandler::CreateReflectionFace(unsigned int glType, const float3& camDir, bool skyOnly)
{
	reflectionCubeFBO.AttachTexture((skyOnly? skyReflectionTexID: envReflectionTexID), glType);

	glPushAttrib(GL_FOG_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	if (!skyOnly) {
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	} else {
		// do not need depth-testing for the sky alone
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
	}

	{
		CCamera* prvCam = CCamera::GetSetActiveCamera(CCamera::CAMTYPE_ENVMAP);
		CCamera* curCam = CCamera::GetActiveCamera();

		bool draw = true;

		#if 0
		switch (glType) {
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB: { glClearColor(1.0f, 0.0f, 0.0f, 1.0f); draw = false; } break; // red
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB: { glClearColor(0.0f, 1.0f, 0.0f, 1.0f); draw = false; } break; // green
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB: { glClearColor(0.0f, 0.0f, 1.0f, 1.0f); draw = false; } break; // blue
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB: { glClearColor(1.0f, 1.0f, 0.0f, 1.0f); draw = false; } break; // yellow
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB: { glClearColor(1.0f, 0.0f, 1.0f, 1.0f); draw = false; } break; // purple
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB: { glClearColor(0.0f, 1.0f, 1.0f, 1.0f); draw = false; } break; // cyan
			default: {} break;
		}

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		#endif

		#if 1
		// work around CCamera::GetRgtFromRot bugs
		switch (glType) {
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB: { /*fwd =  Rgt*/ curCam->right =  FwdVector; curCam->up =   UpVector; } break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB: { /*fwd = -Rgt*/ curCam->right = -FwdVector; curCam->up =   UpVector; } break;
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB: { /*fwd =   Up*/ curCam->right =  RgtVector; curCam->up = -FwdVector; } break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB: { /*fwd =  -Up*/ curCam->right =  RgtVector; curCam->up =  FwdVector; } break;
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB: { /*fwd =  Fwd*/ curCam->right = -RgtVector; curCam->up =   UpVector; } break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB: { /*fwd = -Fwd*/ curCam->right =  RgtVector; curCam->up =   UpVector; } break;
			default: {} break;
		}

		if (draw) {
			// env-reflections are only correct when drawn from an inverted
			// perspective (meaning right becomes left and up becomes down)
			curCam->forward  = camDir;
			curCam->right   *= -1.0f;
			curCam->up      *= -1.0f;

			// we want a *horizontal* FOV of 90 degrees; this gets us close
			// enough (assumes a 16:10 horizontal aspect-ratio common case)
			curCam->SetVFOV(64.0f);
			curCam->SetPos(prvCam->GetPos());
			#else
			curCam->SetRotZ(0.0f);
			curCam->SetDir(camDir);
			#endif

			curCam->UpdateLoadViewPort(0, 0, reflTexSize, reflTexSize);
			// update matrices (not dirs or viewport)
			curCam->Update(false, true, false);

			// generate the face
			game->SetDrawMode(CGame::gameReflectionDraw);
			sky->Draw();

			if (!skyOnly) {
				readMap->GetGroundDrawer()->Draw(DrawPass::TerrainReflection);
			}

			game->SetDrawMode(CGame::gameNormalDraw);
		}

		CCamera::SetActiveCamera(prvCam->GetCamType());
		prvCam->Update();
	}

	glPopAttrib();
}


void CubeMapHandler::UpdateSpecularTexture()
{
	if (!unitDrawer->UseAdvShading())
		return;

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, specularTexID);

	int specularTexRow = specularTexIter / 3; //FIXME WTF

	switch (specularTexIter % 3) {
		case 0: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, specTexSize, float3( 1,  1,  1), float3( 0, 0, -2), float3(0, -2,  0), specularTexRow, &specTexBuf[0]);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, specTexSize, float3(-1,  1, -1), float3( 0, 0,  2), float3(0, -2,  0), specularTexRow, &specTexBuf[0]);
			break;
		}
		case 1: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, specTexSize, float3(-1,  1, -1), float3( 2, 0,  0), float3(0,  0,  2), specularTexRow, &specTexBuf[0]);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, specTexSize, float3(-1, -1,  1), float3( 2, 0,  0), float3(0,  0, -2), specularTexRow, &specTexBuf[0]);
			break;
		}
		case 2: {
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, specTexSize, float3(-1,  1,  1), float3( 2, 0,  0), float3(0, -2,  0), specularTexRow, &specTexBuf[0]);
			UpdateSpecularFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, specTexSize, float3( 1,  1, -1), float3(-2, 0,  0), float3(0, -2,  0), specularTexRow, &specTexBuf[0]);
			break;
		}
	}

	// update one face of one row per frame
	specularTexIter += 1;
	specularTexIter %= (specTexSize * 3);
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
		const float spec = std::min(1.0f, std::pow(dot, sunLighting->specularExponent) + std::pow(dot, 3.0f) * 0.25f);

		buf[x * 4 + 0] = (sunLighting->unitSpecularColor.x * spec * 255);
		buf[x * 4 + 1] = (sunLighting->unitSpecularColor.y * spec * 255);
		buf[x * 4 + 2] = (sunLighting->unitSpecularColor.z * spec * 255);
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

