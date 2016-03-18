/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "AdvWater.h"
#include "ISky.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Exceptions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAdvWater::CAdvWater(bool loadShader)
{
	if (!FBO::IsSupported())
		throw content_error("Water Error: missing FBO support");

	std::vector<unsigned char> scrap(512 * 512 * 4);

	glGenTextures(1, &reflectTexture);
	glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	glGenTextures(1, &bumpTexture);
	glBindTexture(GL_TEXTURE_2D, bumpTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	glGenTextures(4, rawBumpTexture);

	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			scrap[(y*64 + x)*4 + 0] = 128;
			scrap[(y*64 + x)*4 + 1] = (unsigned char)(std::sin(y*PI*2.0f/64.0f)*128 + 128);
			scrap[(y*64 + x)*4 + 2] = 0;
			scrap[(y*64 + x)*4 + 3] = 255;
		}
	}
	glBindTexture(GL_TEXTURE_2D, rawBumpTexture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			const float ang = 26.5f*PI/180.0f;
			const float pos = y*2+x;
			scrap[(y*64 + x)*4 + 0] = (unsigned char)((std::sin(pos*PI*2.0f/64.0f))*128*std::sin(ang)) + 128;
			scrap[(y*64 + x)*4 + 1] = (unsigned char)((std::sin(pos*PI*2.0f/64.0f))*128*std::cos(ang)) + 128;
		}
	}
	glBindTexture(GL_TEXTURE_2D, rawBumpTexture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 64; ++x) {
			const float ang = -19*PI/180.0f;
			const float pos = 3*y - x;
			scrap[(y*64 + x)*4 + 0] = (unsigned char)((std::sin(pos*PI*2.0f/64.0f))*128*std::sin(ang)) + 128;
			scrap[(y*64 + x)*4 + 1] = (unsigned char)((std::sin(pos*PI*2.0f/64.0f))*128*std::cos(ang)) + 128;
		}
	}
	glBindTexture(GL_TEXTURE_2D, rawBumpTexture[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	if (loadShader) {
		// NOTE: needs a VP with OPTION ARB_position_invariant for clipping if !haveGLSL
		waterFP = LoadFragmentProgram("ARB/water.fp");
	}

	waterSurfaceColor = mapInfo->water.surfaceColor;

	reflectFBO.Bind();
	reflectFBO.AttachTexture(reflectTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
	reflectFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT32, 512, 512);
	bumpFBO.Bind();
	bumpFBO.AttachTexture(bumpTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
	FBO::Unbind();

	if (!bumpFBO.IsValid()) {
		throw content_error("Water Error: Invalid FBO");
	}
}

CAdvWater::~CAdvWater()
{
	glDeleteTextures(1, &reflectTexture);
	glDeleteTextures(1, &bumpTexture);
	glDeleteTextures(4, rawBumpTexture);
	glSafeDeleteProgram(waterFP);
}

void CAdvWater::Draw()
{
	Draw(true);
}

void CAdvWater::Draw(bool useBlending)
{
	if (!mapInfo->water.forceRendering && !readMap->HasVisibleWater())
		return;

	float3 base = camera->CalcPixelDir(globalRendering->viewPosX, globalRendering->viewSizeY);
	float3 dv   = camera->CalcPixelDir(globalRendering->viewPosX, 0) - camera->CalcPixelDir(globalRendering->viewPosX, globalRendering->viewSizeY);
	float3 dh   = camera->CalcPixelDir(globalRendering->viewPosX + globalRendering->viewSizeX, 0) - camera->CalcPixelDir(globalRendering->viewPosX, 0);

	float3 xbase;
	const int numDivs = 20;

	base *= numDivs;
	float maxY = -0.1f;
	float yInc = 1.0f / numDivs;
	float screenY = 1;

	unsigned char col[4];
	col[0] = (unsigned char)(waterSurfaceColor.x * 255);
	col[1] = (unsigned char)(waterSurfaceColor.y * 255);
	col[2] = (unsigned char)(waterSurfaceColor.z * 255);

	glDisable(GL_ALPHA_TEST);
	if (useBlending) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
	glDepthMask(0);
	glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, bumpTexture);
		GLfloat plan[] = {0.02f, 0, 0, 0};
		glTexGeni(GL_S,GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGenfv(GL_S,GL_EYE_PLANE, plan);
		glEnable(GL_TEXTURE_GEN_S);

		GLfloat plan2[] = {0, 0, 0.02f, 0};
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGenfv(GL_T, GL_EYE_PLANE, plan2);
		glEnable(GL_TEXTURE_GEN_T);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_2D, reflectTexture);

	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, waterFP);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);

	float3 forward = camera->GetDir();
	forward.y = 0;
	forward.ANormalize();

	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, forward.z,  forward.x, 0, 0);
	glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, -forward.x, forward.z, 0, 0);

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(5*numDivs*(numDivs + 1)*2, 5*numDivs, VA_SIZE_TC); //! alloc room for all vertexes and strips
	float3 dir, zpos;

	for (int a = 0; a < 5; ++a) { //! CAUTION: loop count must match EnlargeArrays above
		bool maxReached = false;
		for (int y = 0; y < numDivs; ++y) {
			dir = base;
			dir.ANormalize();

			if (dir.y >= maxY) {
				maxReached = true;
				break;
			}

			xbase = base;
			for (int x = 0; x < numDivs + 1; ++x) { //! CAUTION: loop count must match EnlargeArrays above
				dir = xbase + dv;
				dir.ANormalize();
				zpos = camera->GetPos() + dir*(camera->GetPos().y / -dir.y);
				zpos.y = std::sin(zpos.z*0.1f + gs->frameNum*0.06f)*0.06f + 0.05f;
				col[3] = (unsigned char)((0.8f + 0.7f*dir.y)*255);
				va->AddVertexQTC(zpos, x*(1.0f/numDivs), screenY - yInc, col);

				dir = xbase;
				dir.ANormalize();
				zpos = camera->GetPos() + dir*(camera->GetPos().y / -dir.y);
				zpos.y = std::sin(zpos.z*0.1f + gs->frameNum*0.06f)*0.06f + 0.05f;
				col[3] = (unsigned char)((0.8f + 0.7f*dir.y)*255);
				va->AddVertexQTC(zpos, x*(1.0f/numDivs), screenY, col);

				xbase += dh;
			}
			va->EndStrip();
			base += dv;
			screenY -= yInc;
		}

		if (!maxReached)
			break;

		dv   *= 0.5f;
		maxY *= 0.5f;
		yInc *= 0.5f;
	}
	va->DrawArrayTC(GL_TRIANGLE_STRIP);

	glDepthMask(1);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	glActiveTextureARB(GL_TEXTURE0_ARB);

	if (!useBlending) { // for translucent stuff like water, the default mode is blending and alpha testing enabled
		glEnable(GL_BLEND);
	}
}

void CAdvWater::UpdateWater(CGame* game)
{
	if (!mapInfo->water.forceRendering && !readMap->HasVisibleWater())
		return;

	glPushAttrib(GL_FOG_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	{
		bumpFBO.Bind();
		glViewport(0, 0, 128, 128);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glColor3f(0.2f, 0.2f, 0.2f);

		CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(12, 0, VA_SIZE_T);

		glBindTexture(GL_TEXTURE_2D, rawBumpTexture[0]);

		va->AddVertexQT(ZeroVector, 0, 0 + gs->frameNum*0.0046f);
		va->AddVertexQT(  UpVector, 0, 2 + gs->frameNum*0.0046f);
		va->AddVertexQT(  XYVector, 2, 2 + gs->frameNum*0.0046f);
		va->AddVertexQT( RgtVector, 2, 0 + gs->frameNum*0.0046f);

		va->AddVertexQT(ZeroVector, 0, 0 + gs->frameNum*0.0026f);
		va->AddVertexQT(  UpVector, 0, 4 + gs->frameNum*0.0026f);
		va->AddVertexQT(  XYVector, 2, 4 + gs->frameNum*0.0026f);
		va->AddVertexQT( RgtVector, 2, 0 + gs->frameNum*0.0026f);

		va->AddVertexQT(ZeroVector, 0, 0 + gs->frameNum*0.0012f);
		va->AddVertexQT(  UpVector, 0, 8 + gs->frameNum*0.0012f);
		va->AddVertexQT(  XYVector, 2, 8 + gs->frameNum*0.0012f);
		va->AddVertexQT( RgtVector, 2, 0 + gs->frameNum*0.0012f);

		va->DrawArrayT(GL_QUADS);

		va = GetVertexArray();
		va->Initialize();
		glBindTexture(GL_TEXTURE_2D, rawBumpTexture[1]);

		va->AddVertexQT(ZeroVector, 0, 0 + gs->frameNum*0.0036f);
		va->AddVertexQT(  UpVector, 0, 1 + gs->frameNum*0.0036f);
		va->AddVertexQT(  XYVector, 1, 1 + gs->frameNum*0.0036f);
		va->AddVertexQT( RgtVector, 1, 0 + gs->frameNum*0.0036f);

		va->DrawArrayT(GL_QUADS);

		va = GetVertexArray();
		va->Initialize();
		glBindTexture(GL_TEXTURE_2D, rawBumpTexture[2]);

		va->AddVertexQT(ZeroVector, 0, 0 + gs->frameNum*0.0082f);
		va->AddVertexQT(  UpVector, 0, 1 + gs->frameNum*0.0082f);
		va->AddVertexQT(  XYVector, 1, 1 + gs->frameNum*0.0082f);
		va->AddVertexQT( RgtVector, 1, 0 + gs->frameNum*0.0082f);

		va->DrawArrayT(GL_QUADS);

		// this fixes a memory leak on ATI cards
		glBindTexture(GL_TEXTURE_2D, 0);
		glColor3f(1.0f, 1.0f, 1.0f);
	}


	reflectFBO.Bind();
	glClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const double clipPlaneEqs[2 * 4] = {
		0.0, 1.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
	};

	CCamera* prvCam = CCamera::GetSetActiveCamera(CCamera::CAMTYPE_UWREFL);
	CCamera* curCam = CCamera::GetActiveCamera();

	{
		curCam->CopyStateReflect(prvCam);
		curCam->UpdateLoadViewPort(0, 0, 512, 512);

		DrawReflections(&clipPlaneEqs[0], true, true);
	}

	CCamera::SetActiveCamera(prvCam->GetCamType());
	prvCam->Update();
	prvCam->LoadViewPort();

	FBO::Unbind();

	glPopAttrib();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
