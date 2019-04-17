/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AdvWater.h"
#include "ISky.h"
#include "WaterRendering.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "System/Exceptions.h"


CAdvWater::CAdvWater(bool refractive)
{
	uint8_t scrap[512 * 512 * 4];

	glGenTextures(1, &reflectTexture);
	glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	glGenTextures(1, &bumpmapTexture);
	glBindTexture(GL_TEXTURE_2D, bumpmapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);

	glGenTextures(4, rawBumpTextures);

	{
		for (int y = 0; y < 64; ++y) {
			for (int x = 0; x < 64; ++x) {
				scrap[(y*64 + x)*4 + 0] = 128;
				scrap[(y*64 + x)*4 + 1] = static_cast<uint8_t>(fastmath::sin(y * math::TWOPI / 64.0f) * 128 + 128);
				scrap[(y*64 + x)*4 + 2] = 0;
				scrap[(y*64 + x)*4 + 3] = 255;
			}
		}


		glBindTexture(GL_TEXTURE_2D, rawBumpTextures[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);
	}
	{
		for (int y = 0; y < 64; ++y) {
			for (int x = 0; x < 64; ++x) {
				const float ang = 26.5f * math::DEG_TO_RAD;
				const float pos = y * 2 + x;

				scrap[(y*64 + x)*4 + 0] = static_cast<uint8_t>((fastmath::sin(pos*math::TWOPI / 64.0f)) * 128 * fastmath::sin(ang)) + 128;
				scrap[(y*64 + x)*4 + 1] = static_cast<uint8_t>((fastmath::sin(pos*math::TWOPI / 64.0f)) * 128 * fastmath::cos(ang)) + 128;
			}
		}


		glBindTexture(GL_TEXTURE_2D, rawBumpTextures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);
	}
	{
		for (int y = 0; y < 64; ++y) {
			for (int x = 0; x < 64; ++x) {
				const float ang = -19.0f * math::DEG_TO_RAD;
				const float pos = 3.0f * y - x;

				scrap[(y*64 + x)*4 + 0] = static_cast<uint8_t>((fastmath::sin(pos*math::TWOPI / 64.0f)) * 128 * fastmath::sin(ang)) + 128;
				scrap[(y*64 + x)*4 + 1] = static_cast<uint8_t>((fastmath::sin(pos*math::TWOPI / 64.0f)) * 128 * fastmath::cos(ang)) + 128;
			}
		}


		glBindTexture(GL_TEXTURE_2D, rawBumpTextures[2]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &scrap[0]);
	}
	if (refractive) {
		glGenTextures(1, &subsurfTexture);
		glBindTexture(GL_TEXTURE_RECTANGLE, subsurfTexture);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, 3, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_RGB, GL_INT, 0);
	}


	if (!refractive) {
		const std::string& vsText = Shader::GetShaderSource("GLSL/ReflectiveWaterVertProg.glsl");
		const std::string& fsText = Shader::GetShaderSource("GLSL/ReflectiveWaterFragProg.glsl");

		waterShader = shaderHandler->CreateProgramObject("[AdvWater]", "ReflectiveWaterShader");
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject(vsText, "", GL_VERTEX_SHADER));
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject(fsText, "", GL_FRAGMENT_SHADER));
	} else {
		const std::string& vsText = Shader::GetShaderSource("GLSL/RefractiveWaterVertProg.glsl");
		const std::string& fsText = Shader::GetShaderSource("GLSL/RefractiveWaterFragProg.glsl");

		waterShader = shaderHandler->CreateProgramObject("[AdvWater]", "RefractiveWaterShader");
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject(vsText, "", GL_VERTEX_SHADER));
		waterShader->AttachShaderObject(shaderHandler->CreateShaderObject(fsText, "", GL_FRAGMENT_SHADER));
	}
	{
		waterShader->Link();

		if (!waterShader->IsValid())
			throw opengl_error("[AdvWater] invalid shader");

		waterShader->Enable();
		waterShader->SetUniform("reflmap_tex", 0);
		waterShader->SetUniform("bumpmap_tex", 1);
		waterShader->SetUniform("subsurf_tex", 2); // refractive
		waterShader->SetUniform("shading_tex", 3); // refractive

		waterShader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		waterShader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::Identity());
		waterShader->SetUniform("u_forward_vec", 0.0f, 0.0f);
		waterShader->SetUniform("u_gamma_expon", 1.0f, 1.0f, 1.0f);
		waterShader->SetUniform("u_texrect_size", 0.0f, 0.0f); // refractive
		waterShader->SetUniform("u_shading_sgen", 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 0.0f, 0.0f, 0.0f); // refractive
		waterShader->SetUniform("u_shading_tgen", 0.0f, 0.0f, 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0.0f); // refractive

		waterShader->Disable();
		waterShader->Validate();
	}

	reflectFBO.Bind();
	reflectFBO.AttachTexture(reflectTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);
	reflectFBO.CreateRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT32, 512, 512);
	bumpFBO.Bind();
	bumpFBO.AttachTexture(bumpmapTexture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0);
	FBO::Unbind();

	if (bumpFBO.IsValid())
		return;

	throw content_error("[AdvWater] invalid bumpFBO");
}

CAdvWater::~CAdvWater()
{
	shaderHandler->ReleaseProgramObjects("[AdvWater]");

	glDeleteTextures(1, &reflectTexture);
	glDeleteTextures(1, &bumpmapTexture);
	glDeleteTextures(4, rawBumpTextures);

	if (subsurfTexture == 0)
		return;

	glDeleteTextures(1, &subsurfTexture);
}


void CAdvWater::Draw(bool useBlending)
{
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	const int vpx = globalRendering->viewPosX;
	const int vsx = globalRendering->viewSizeX;
	const int vsy = globalRendering->viewSizeY;


	constexpr float numDivs = 20.0f;
	const     float invDivs = 1.0f / numDivs;

	const float3& cameraPos = camera->GetPos();
	const float3& planeColor = waterRendering->surfaceColor;
	const float3  gammaExpon = OnesVector * globalRendering->gammaExponent;

	float3 base = camera->CalcPixelDir(vpx      , vsy) * numDivs;
	float3 dv   = camera->CalcPixelDir(vpx      ,   0) - camera->CalcPixelDir(vpx, vsy);
	float3 dh   = camera->CalcPixelDir(vpx + vsx,   0) - camera->CalcPixelDir(vpx,   0);

	float3 xbase;
	float3 forward = camera->GetDir();
	float3 dir;
	float3 zpos;

	forward.ANormalize2D();

	// in screen-space
	float ymax = -0.1f;
	float yinc = invDivs;
	float yscr = 1.0f;

	uint8_t col[4];
	col[0] = static_cast<uint8_t>(planeColor.x * 255.0f);
	col[1] = static_cast<uint8_t>(planeColor.y * 255.0f);
	col[2] = static_cast<uint8_t>(planeColor.z * 255.0f);

	glAttribStatePtr->DisableDepthMask();

	if (useBlending) {
		glAttribStatePtr->EnableBlendMask();
	} else {
		glAttribStatePtr->DisableBlendMask();
	}


	if (subsurfTexture != 0) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture()); // has water depth encoded in alpha

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_RECTANGLE, subsurfTexture);
		glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bumpmapTexture);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
	}

	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));


	GL::RenderDataBufferTC* rdb = GL::GetRenderBufferTC();
	Shader::IProgramObject* ipo = waterShader;

	ipo->Enable();
	ipo->SetUniformMatrix4x4<float>("u_movi_mat", false, camera->GetViewMatrix());
	ipo->SetUniformMatrix4x4<float>("u_proj_mat", false, camera->GetProjectionMatrix());
	ipo->SetUniform("u_forward_vec", forward.x, forward.z);
	ipo->SetUniform("u_gamma_expon", gammaExpon.x, gammaExpon.y, gammaExpon.z);
	ipo->SetUniform("u_texrect_size", globalRendering->viewSizeX * 10.0f, globalRendering->viewSizeY * 10.0f); // range [0, width], not [0,1]

	// generate a grid, bottom to top
	for (int a = 0, yn = int(numDivs), xn = yn + 1; a < 5; ++a) {
		bool maxReached = false;

		for (int y = 0; y < yn; ++y) {
			dir = ((xbase = base) + ZeroVector).ANormalize();

			if ((maxReached |= (dir.y >= ymax)))
				break;

			for (int x = 0; x < xn; ++x) {
				// bottom
				dir = (xbase + ZeroVector).ANormalize();
				zpos = cameraPos + dir * (cameraPos.y / -dir.y);
				zpos.y = fastmath::sin(zpos.z * 0.1f + gs->frameNum * 0.06f) * 0.06f + 0.05f;
				col[3] = static_cast<uint8_t>((0.8f + 0.7f * dir.y) * 255);

				rdb->SafeAppend({zpos, x * invDivs, yscr, col});

				// top
				dir = (xbase + dv).ANormalize();
				zpos = cameraPos + dir * (cameraPos.y / -dir.y);
				zpos.y = fastmath::sin(zpos.z * 0.1f + gs->frameNum * 0.06f) * 0.06f + 0.05f;
				col[3] = static_cast<uint8_t>((0.8f + 0.7f * dir.y) * 255);

				rdb->SafeAppend({zpos, x * invDivs, yscr - yinc, col});

				xbase += dh;
			}

			{
				// pseudo-terminate row with degenerate verts
				// dir = (xbase + dv).ANormalize();
				// zpos = cameraPos + dir * (cameraPos.y / -dir.y);
				// zpos.y = fastmath::sin(zpos.z * 0.1f + gs->frameNum * 0.06f) * 0.06f + 0.05f;
				rdb->SafeAppend({zpos, 1.0f, yscr - yinc, col});

				dir = (base + dv).ANormalize();
				zpos = cameraPos + dir * (cameraPos.y / -dir.y);
				zpos.y = fastmath::sin(zpos.z * 0.1f + gs->frameNum * 0.06f) * 0.06f + 0.05f;

				rdb->SafeAppend({zpos, 0.0f, yscr - yinc, col});
			}

			base += dv;
			yscr -= yinc;
		}

		if (!maxReached)
			break;

		dv   *= 0.5f;
		ymax *= 0.5f;
		yinc *= 0.5f;
	}

	rdb->Submit(GL_TRIANGLE_STRIP);
	ipo->Disable();

	glAttribStatePtr->EnableDepthMask();
	glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// for translucent stuff like water, the default mode is blending and alpha testing enabled
	if (!useBlending)
		glAttribStatePtr->EnableBlendMask();
}


void CAdvWater::UpdateWater(CGame* game)
{
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	glAttribStatePtr->PushColorBufferBit();
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE);

	{
		bumpFBO.Bind();

		glAttribStatePtr->ViewPort(0, 0, 128, 128);
		glAttribStatePtr->ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);

		const SColor color = {0.2f, 0.2f, 0.2f};

		GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
		Shader::IProgramObject* shader = buffer->GetShader();

		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));

		{
			glBindTexture(GL_TEXTURE_2D, rawBumpTextures[0]);
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0046f, color}); // tl
			buffer->SafeAppend({  UpVector, 0.0f, 2.0f + gs->frameNum * 0.0046f, color}); // bl
			buffer->SafeAppend({  XYVector, 2.0f, 2.0f + gs->frameNum * 0.0046f, color}); // br

			buffer->SafeAppend({  XYVector, 2.0f, 2.0f + gs->frameNum * 0.0046f, color}); // br
			buffer->SafeAppend({ RgtVector, 2.0f, 0.0f + gs->frameNum * 0.0046f, color}); // tr
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0046f, color}); // tl


			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0026f, color});
			buffer->SafeAppend({  UpVector, 0.0f, 4.0f + gs->frameNum * 0.0026f, color});
			buffer->SafeAppend({  XYVector, 2.0f, 4.0f + gs->frameNum * 0.0026f, color});

			buffer->SafeAppend({  XYVector, 2.0f, 4.0f + gs->frameNum * 0.0026f, color});
			buffer->SafeAppend({ RgtVector, 2.0f, 0.0f + gs->frameNum * 0.0026f, color});
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0026f, color});


			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0012f, color});
			buffer->SafeAppend({  UpVector, 0.0f, 8.0f + gs->frameNum * 0.0012f, color});
			buffer->SafeAppend({  XYVector, 2.0f, 8.0f + gs->frameNum * 0.0012f, color});

			buffer->SafeAppend({  XYVector, 2.0f, 8.0f + gs->frameNum * 0.0012f, color});
			buffer->SafeAppend({ RgtVector, 2.0f, 0.0f + gs->frameNum * 0.0012f, color});
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0012f, color});
			buffer->Submit(GL_TRIANGLES);
		}
		{
			glBindTexture(GL_TEXTURE_2D, rawBumpTextures[1]);
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0036f, color});
			buffer->SafeAppend({  UpVector, 0.0f, 1.0f + gs->frameNum * 0.0036f, color});
			buffer->SafeAppend({  XYVector, 1.0f, 1.0f + gs->frameNum * 0.0036f, color});

			buffer->SafeAppend({  XYVector, 1.0f, 1.0f + gs->frameNum * 0.0036f, color});
			buffer->SafeAppend({ RgtVector, 1.0f, 0.0f + gs->frameNum * 0.0036f, color});
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0036f, color});
			buffer->Submit(GL_TRIANGLES);
		}
		{
			glBindTexture(GL_TEXTURE_2D, rawBumpTextures[2]);
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0082f, color});
			buffer->SafeAppend({  UpVector, 0.0f, 1.0f + gs->frameNum * 0.0082f, color});
			buffer->SafeAppend({  XYVector, 1.0f, 1.0f + gs->frameNum * 0.0082f, color});

			buffer->SafeAppend({  XYVector, 1.0f, 1.0f + gs->frameNum * 0.0082f, color});
			buffer->SafeAppend({ RgtVector, 1.0f, 0.0f + gs->frameNum * 0.0082f, color});
			buffer->SafeAppend({ZeroVector, 0.0f, 0.0f + gs->frameNum * 0.0082f, color});
			buffer->Submit(GL_TRIANGLES);
		}

		// this fixes a memory leak on ATI cards
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	reflectFBO.Bind();
	glAttribStatePtr->ClearColor(sky->fogColor[0], sky->fogColor[1], sky->fogColor[2], 1.0f);
	glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	CCamera* prvCam = CCameraHandler::GetSetActiveCamera(CCamera::CAMTYPE_UWREFL);
	CCamera* curCam = CCameraHandler::GetActiveCamera();

	{
		curCam->CopyStateReflect(prvCam);
		curCam->UpdateLoadViewPort(0, 0, 512, 512);

		DrawReflections(true, true);
	}

	CCameraHandler::SetActiveCamera(prvCam->GetCamType());
	prvCam->Update();
	prvCam->LoadViewPort();

	FBO::Unbind();

	glAttribStatePtr->PopBits();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

