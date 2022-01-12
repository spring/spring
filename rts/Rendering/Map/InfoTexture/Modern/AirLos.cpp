/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AirLos.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/LosHandler.h"
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


CAirLosTexture::CAirLosTexture()
: CPboInfoTexture("airlos")
, uploadTex(0)
{
	texSize = losHandler->airLos.size;
	texChannels = 1;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, -1, GL_R8, texSize.x, texSize.y);

	infoTexPBO.Bind();
	infoTexPBO.New(texSize.x * texSize.y * texChannels * 2, GL_STREAM_DRAW);
	infoTexPBO.Unbind();

	{
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CAirLosTexture");
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

		uniform sampler2D tex0;

		in vec2 vTexCoords;
		layout(location = 0) out vec4 fFragColor;

		void main() {
			vec2 f = texture(tex0, vTexCoords).rg;
			float c = (f.r + f.g) * 200000.0;
			fFragColor = vec4(c);
		}
	)";

	const char* errorFormats[2] = {
		"%s-shader compilation error: %s",
		"%s-shader validation error: %s",
	};

	shader = shaderHandler->CreateProgramObject("[CAirLosTexture]", "CAirLosTexture");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();

	if (!shader->IsValid()) {
		LOG_L(L_ERROR, errorFormats[0], shader->GetName().c_str(), shader->GetLog().c_str());
		throw opengl_error("");
	}

	shader->Enable();
	shader->SetUniform("tex0", 0);
	shader->Disable();
	shader->Validate();

	if (!shader->IsValid()) {
		LOG_L(L_ERROR, errorFormats[1], shader->GetName().c_str(), shader->GetLog().c_str());
		throw opengl_error("");
	}

	glGenTextures(1, &uploadTex);
	glBindTexture(GL_TEXTURE_2D, uploadTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, texSize.x, texSize.y);
}


CAirLosTexture::~CAirLosTexture()
{
	glDeleteTextures(1, &uploadTex);
}


void CAirLosTexture::UpdateCPU()
{
	infoTexPBO.Bind();
	uint8_t* infoTexMem = reinterpret_cast<uint8_t*>(infoTexPBO.MapBuffer());

	if (!losHandler->globalLOS[gu->myAllyTeam]) {
		const uint16_t* myAirLos = &losHandler->airLos.losMaps[gu->myAllyTeam].front();
		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				infoTexMem[y * texSize.x + x] = 255 * (myAirLos[y * texSize.x + x] != 0);
			}
		}
	} else {
		memset(infoTexMem, 255, texSize.x * texSize.y);
	}

	infoTexPBO.UnmapBuffer();
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RED, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
	glGenerateMipmap(GL_TEXTURE_2D);
	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();
}


void CAirLosTexture::Update()
{
	if (!fbo.IsValid() || !shader->IsValid() || uploadTex == 0)
		return UpdateCPU();

	if (losHandler->globalLOS[gu->myAllyTeam]) {
		fbo.Bind();
		glAttribStatePtr->ViewPort(0,0, texSize.x, texSize.y);
		glAttribStatePtr->ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT);
		glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);
		FBO::Unbind();

		glBindTexture(GL_TEXTURE_2D, texture);
		glGenerateMipmap(GL_TEXTURE_2D);
		return;
	}


	infoTexPBO.Bind();

	      uint8_t* infoTexMem = reinterpret_cast<uint8_t*>(infoTexPBO.MapBuffer());
	const uint16_t* myAirLos = &losHandler->airLos.losMaps[gu->myAllyTeam].front();

	memcpy(infoTexMem, myAirLos, texSize.x * texSize.y * texChannels * sizeof(uint16_t));
	infoTexPBO.UnmapBuffer();


	//Trick: Upload the ushort as 2 ubytes, and then check both for `!=0` in the shader.
	// Faster than doing it on the CPU! And uploading it as shorts would be slow, cause the GPU
	// has no native support for them and so the transformation would happen on the CPU, too.
	glBindTexture(GL_TEXTURE_2D, uploadTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RG, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();

	// do post-processing on the gpu (los-checking & scaling)
	fbo.Bind();

	glAttribStatePtr->ViewPort(0, 0,  texSize.x, texSize.y);
	glAttribStatePtr->DisableBlendMask();

	GL::RenderDataBuffer0* rdb = GL::GetRenderBuffer0();

	shader->Enable();
	rdb->SafeAppend(VERTS, sizeof(VERTS) / sizeof(VERTS[0]));
	rdb->Submit(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);

	FBO::Unbind();

	// generate mipmaps
	glBindTexture(GL_TEXTURE_2D, texture);
	glGenerateMipmap(GL_TEXTURE_2D);
}
