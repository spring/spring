/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Height.h"
#include "Map/HeightLinePalette.h"
#include "Map/HeightMapTexture.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

// currently defined in HeightLinePalette.cpp
//CONFIG(bool, ColorElev).defaultValue(true).description("If heightmap (default hotkey [F1]) should be colored or not.");

constexpr VA_TYPE_0 VERTS[] = {
	{{ 0.0f,  0.0f, 0.0f}}, // bl
	{{ 0.0f, +1.0f, 0.0f}}, // tl
	{{+1.0f, +1.0f, 0.0f}}, // tr

	{{+1.0f, +1.0f, 0.0f}}, // tr
	{{+1.0f,  0.0f, 0.0f}}, // br
	{{ 0.0f,  0.0f, 0.0f}}, // bl
};


CHeightTexture::CHeightTexture()
: CPboInfoTexture("height")
, CEventClient("[CHeightTexture]", 271990, false)
, needUpdate(true)
{
	eventHandler.AddClient(this);

	texSize = int2(mapDims.mapxp1, mapDims.mapyp1);
	texChannels = 4;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texSize.x, texSize.y);

	glGenTextures(1, &paletteTex);
	glBindTexture(GL_TEXTURE_2D, paletteTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, &CHeightLinePalette::paletteColored[0].r);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 1, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, &CHeightLinePalette::paletteBlackAndWhite[0].r);

	{
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CHeightTexture");
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
			gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
			gl_Position.xy = aVertexPos.xy * 2.0 - 1.0;
			vTexCoords = aVertexPos.st;
		}
	)";

	const std::string fragmentCode = R"(
		#version 410 core

		uniform sampler2D texHeight;
		uniform sampler2D texPalette;
		uniform float paletteOffset;

		in vec2 vTexCoords;
		layout(location = 0) out vec4 fFragColor;

		void main() {
			float h = texture(texHeight, vTexCoords).r;
			vec2 tc = vec2(h * (8. / 256.), paletteOffset);
			fFragColor = texture(texPalette, tc);
		}
	)";

	const char* errorFormats[2] = {
		"%s-shader compilation error: %s",
		"%s-shader validation error: %s",
	};

	shader = shaderHandler->CreateProgramObject("[CHeightTexture]", "CHeightTexture");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();

	if (!shader->IsValid()) {
		LOG_L(L_ERROR, errorFormats[0], shader->GetName().c_str(), shader->GetLog().c_str());
		throw opengl_error("");
	}

	shader->Enable();
	shader->SetUniform("texHeight", 0);
	shader->SetUniform("texPalette", 1);
	shader->SetUniform("paletteOffset", configHandler->GetBool("ColorElev") ? 0.0f : 1.0f);
	shader->Disable();
	shader->Validate();

	if (!shader->IsValid()) {
		LOG_L(L_ERROR, errorFormats[1], shader->GetName().c_str(), shader->GetLog().c_str());
		throw opengl_error("");
	}
}


void CHeightTexture::UpdateCPU()
{
	const SColor* extraTexPal = CHeightLinePalette::GetData();
	const float* heightMap = readMap->GetCornerHeightMapUnsynced();

	infoTexPBO.Bind();
	infoTexPBO.New(texSize.x * texSize.y * texChannels, GL_STREAM_DRAW);
	auto infoTexMem = reinterpret_cast<SColor*>(infoTexPBO.MapBuffer());

	for (int y = 0; y < texSize.y; ++y) {
		for (int x = 0; x < texSize.x; ++x) {
			const int idx = y * texSize.x + x;
			const float height = heightMap[idx];
			const unsigned int value = ((unsigned int)(height * 8.0f)) % 255;
			infoTexMem[idx] = extraTexPal[value];
		}
	}

	infoTexPBO.UnmapBuffer();
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RGBA, GL_UNSIGNED_BYTE, infoTexPBO.GetPtr());
	infoTexPBO.Invalidate();
	infoTexPBO.Unbind();
}


CHeightTexture::~CHeightTexture()
{
	glDeleteTextures(1, &paletteTex);
}


void CHeightTexture::Update()
{
	needUpdate = false;

	if (!fbo.IsValid() || !shader->IsValid() || (heightMapTexture->GetTextureID() == 0))
		return UpdateCPU();

	fbo.Bind();
	glAttribStatePtr->ViewPort(0, 0,  texSize.x, texSize.y);
	glAttribStatePtr->DisableBlendMask();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, paletteTex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());

	GL::RenderDataBuffer0* rdb = GL::GetRenderBuffer0();

	shader->Enable();
	rdb->SafeAppend(VERTS, sizeof(VERTS) / sizeof(VERTS[0]));
	rdb->Submit(GL_TRIANGLES);
	shader->Disable();

	glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0,  globalRendering->viewSizeX, globalRendering->viewSizeY);

	FBO::Unbind();
}

