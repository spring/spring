/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Radar.h"
#include "InfoTextureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

constexpr VA_TYPE_0 VERTS[] = {
	{{-1.0f, -1.0f, 0.0f}}, // bl
	{{-1.0f, +1.0f, 0.0f}}, // tl
	{{+1.0f, +1.0f, 0.0f}}, // tr

	{{+1.0f, +1.0f, 0.0f}}, // tr
	{{+1.0f, -1.0f, 0.0f}}, // br
	{{-1.0f, -1.0f, 0.0f}}, // bl
};


CRadarTexture::CRadarTexture()
: CPboInfoTexture("radar")
, uploadTexRadar(0)
, uploadTexJammer(0)
{
	texSize = losHandler->radar.size;
	texChannels = 2;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, -1, GL_RG8, texSize.x, texSize.y);

	infoTexPBO.Bind();
	infoTexPBO.New(texSize.x * texSize.y * texChannels * sizeof(uint16_t), GL_STREAM_DRAW);
	infoTexPBO.Unbind();

	{
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CRadarTexture");
		FBO::Unbind();
	}

	if (!fbo.IsValid())
		throw opengl_error("");

	const std::string vertexCode = R"(
		#version 410 core

		layout(location = 0) in vec3 aVertexPos;
		layout(location = 1) in vec2 aTexCoords; // ignored
		out vec2 vTexCoords;

		void main() {
			vTexCoords = aVertexPos.xy * 0.5 + 0.5;
			gl_Position = vec4(aVertexPos, 1.0);
		}
	)";

	const std::string fragmentCode = R"(
		#version 410 core

		uniform sampler2D texLoS;
		uniform sampler2D texRadar;
		uniform sampler2D texJammer;

		in vec2 vTexCoords;
		layout(location = 0) out vec4 fFragColor;

		void main() {
			float los = texture(texLoS, vTexCoords).r;

			vec2 fr = texture(texRadar,  vTexCoords).rg;
			vec2 fj = texture(texJammer, vTexCoords).rg;
			float cr = (fr.r + fr.g) * 200000.0;
			float cj = (fj.r + fj.g) * 200000.0;
			fFragColor = vec4(cr, los * cj, 0.0f, 0.0f);
		}
	)";

	const char* errorFormats[2] = {
		"%s-shader compilation error: %s",
		"%s-shader validation error: %s",
	};

	shader = shaderHandler->CreateProgramObject("[CRadarTexture]", "CRadarTexture");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();

	if (!shader->IsValid()) {
		LOG_L(L_ERROR, errorFormats[0], shader->GetName().c_str(), shader->GetLog().c_str());
		throw opengl_error("");
	}

	shader->Enable();
	shader->SetUniform("texRadar",  1);
	shader->SetUniform("texJammer", 0);
	shader->SetUniform("texLoS",    2);
	shader->Disable();
	shader->Validate();

	if (!shader->IsValid()) {
		LOG_L(L_ERROR, errorFormats[1], shader->GetName().c_str(), shader->GetLog().c_str());
		throw opengl_error("");
	}

	glGenTextures(1, &uploadTexRadar);
	glBindTexture(GL_TEXTURE_2D, uploadTexRadar);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, texSize.x, texSize.y);

	glGenTextures(1, &uploadTexJammer);
	glBindTexture(GL_TEXTURE_2D, uploadTexJammer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, texSize.x, texSize.y);
}


CRadarTexture::~CRadarTexture()
{
	glDeleteTextures(1, &uploadTexRadar);
	glDeleteTextures(1, &uploadTexJammer);
}


void CRadarTexture::UpdateCPU()
{
	infoTexPBO.Bind();
	uint8_t* infoTexMem = reinterpret_cast<uint8_t*>(infoTexPBO.MapBuffer());

	if (!losHandler->globalLOS[gu->myAllyTeam]) {
		const int jammerAllyTeam = modInfo.separateJammers ? gu->myAllyTeam : 0;

		const uint16_t* myLos = &losHandler->los.losMaps[gu->myAllyTeam].front();
		const uint16_t* myRadar = &losHandler->radar.losMaps[gu->myAllyTeam].front();
		const uint16_t* myJammer = &losHandler->jammer.losMaps[jammerAllyTeam].front();

		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				const int idx = y * texSize.x + x;
				infoTexMem[idx * 2 + 0] = ( myRadar[idx] != 0) ? 255 : 0;
				infoTexMem[idx * 2 + 1] = (myJammer[idx] != 0 && myLos[idx] != 0) ? 255 : 0;
			}
		}
	} else {
		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				const int idx = y * texSize.x + x;
				infoTexMem[idx * 2 + 0] = 255;
				infoTexMem[idx * 2 + 1] = 0;
			}
		}
	}

	infoTexPBO.UnmapBuffer();
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RG, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();
}


void CRadarTexture::Update()
{
	if (!fbo.IsValid() || !shader->IsValid() || uploadTexRadar == 0 || uploadTexJammer == 0)
		return UpdateCPU();

	if (losHandler->globalLOS[gu->myAllyTeam]) {
		fbo.Bind();
		glAttribStatePtr->ViewPort(0,0, texSize.x, texSize.y);
		glAttribStatePtr->ClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);
		glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);
		FBO::Unbind();

		glBindTexture(GL_TEXTURE_2D, texture);
		glGenerateMipmap(GL_TEXTURE_2D);
		return;
	}

	const int jammerAllyTeam = modInfo.separateJammers ? gu->myAllyTeam : 0;


	infoTexPBO.Bind();

	const size_t arraySize = texSize.x * texSize.y * sizeof(uint16_t);
	      uint8_t* infoTexMem = reinterpret_cast<uint8_t*>(infoTexPBO.MapBuffer());
	const uint16_t* myRadar  = &losHandler->radar.losMaps[gu->myAllyTeam].front();
	const uint16_t* myJammer = &losHandler->jammer.losMaps[jammerAllyTeam].front();

	memcpy(infoTexMem, myRadar , arraySize); infoTexMem += arraySize;
	memcpy(infoTexMem, myJammer, arraySize);

	infoTexPBO.UnmapBuffer();


	//Trick: Upload the ushort as 2 ubytes, and then check both for `!=0` in the shader.
	// Faster than doing it on the CPU! And uploading it as shorts would be slow, cause the GPU
	// has no native support for them and so the transformation would happen on the CPU, too.
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, uploadTexRadar);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RG, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, uploadTexJammer);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RG, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr(arraySize));

	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();

	// do post-processing on the gpu (los-checking & scaling)
	fbo.Bind();
	glAttribStatePtr->ViewPort(0, 0,  texSize.x, texSize.y);
	glAttribStatePtr->DisableBlendMask();

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetInfoTexture("los")->GetTexture());

	GL::RenderDataBuffer0* rdb = GL::GetRenderBuffer0();

	shader->Enable();
	rdb->SafeAppend(VERTS, sizeof(VERTS) / sizeof(VERTS[0]));
	rdb->Submit(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);

	FBO::Unbind();

	// generate mipmaps
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glGenerateMipmap(GL_TEXTURE_2D);
}

