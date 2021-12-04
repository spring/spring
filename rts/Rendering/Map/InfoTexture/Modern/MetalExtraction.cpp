/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MetalExtraction.h"
#include "InfoTextureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
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


CMetalExtractionTexture::CMetalExtractionTexture()
: CPboInfoTexture("metalextraction")
, updateN(0)
{
	texSize = int2(mapDims.hmapx, mapDims.hmapy);
	texChannels = 1;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//Info: 32F isn't really needed for the final result, but it allows us
	//  to upload the CPU array directly to the GPU w/o any (slow) cpu-side
	//  transformation. The transformation (0..1 range rescaling) happens
	//  then on the gpu instead.
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, texSize.x, texSize.y);

	{
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CMetalExtractionTexture");
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
			fFragColor = texture(tex0, vTexCoords) * 800.0;
		}
	)";

	const char* errorFormats[2] = {
		"%s-shader compilation error: %s",
		"%s-shader validation error: %s",
	};

	shader = shaderHandler->CreateProgramObject("[CMetalExtractionTexture]", "CMetalExtractionTexture");
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
}


bool CMetalExtractionTexture::IsUpdateNeeded()
{
	// update only once per second
	return (updateN++ % GAME_SPEED == 0);
}


void CMetalExtractionTexture::Update()
{
	// los-checking is done in FBO: when FBO isn't working don't expose hidden data!
	if (!fbo.IsValid() || !shader->IsValid())
		return;

	CInfoTexture* infoTex = infoTextureHandler->GetInfoTexture("los");

	assert(metalMap.GetSizeX() == texSize.x && metalMap.GetSizeZ() == texSize.y);

	// upload raw data to gpu
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RED, GL_FLOAT, metalMap.GetExtractionMap());

	// do post-processing on the gpu (los-checking & scaling)
	fbo.Bind();

	glAttribStatePtr->ViewPort(0, 0,  texSize.x, texSize.y);
	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_ZERO, GL_SRC_COLOR);
	glBindTexture(GL_TEXTURE_2D, infoTex->GetTexture());

	GL::RenderDataBuffer0* rdb = GL::GetRenderBuffer0();

	shader->Enable();
	rdb->SafeAppend(VERTS, sizeof(VERTS) / sizeof(VERTS[0]));
	rdb->Submit(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);

	FBO::Unbind();
}
