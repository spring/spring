/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AirLos.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/LosHandler.h"
#include "System/Exceptions.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"



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

	if (FBO::IsSupported()) {
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CAirLosTexture");
		FBO::Unbind();
	}

	const std::string vertexCode = R"(
		varying vec2 texCoord;

		void main() {
			texCoord = gl_Vertex.xy * 0.5 + 0.5;
			gl_Position = vec4(gl_Vertex.xyz, 1.0);
		}
	)";

	const std::string fragmentCode = R"(
		uniform sampler2D tex0;
		varying vec2 texCoord;

		void main() {
			vec2 f = texture2D(tex0, texCoord).rg;
			float c = (f.r + f.g) * 200000.0;
			gl_FragColor = vec4(c);
		}
	)";

	shader = shaderHandler->CreateProgramObject("[CAirLosTexture]", "CAirLosTexture", false);
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(vertexCode,   "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject(fragmentCode, "", GL_FRAGMENT_SHADER));
	shader->Link();
	if (!shader->IsValid()) {
		const char* fmt = "%s-shader compilation error: %s";
		LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
	} else {
		shader->Enable();
		shader->SetUniform("tex0", 0);
		shader->Disable();
		shader->Validate();
		if (!shader->IsValid()) {
			const char* fmt = "%s-shader validation error: %s";
			LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
		}
	}

	if (fbo.IsValid() && shader->IsValid()) {
		glGenTextures(1, &uploadTex);
		glBindTexture(GL_TEXTURE_2D, uploadTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSpringTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, texSize.x, texSize.y);
	} else {
		throw opengl_error("");
	}
}


CAirLosTexture::~CAirLosTexture()
{
	glDeleteTextures(1, &uploadTex);
}


void CAirLosTexture::UpdateCPU()
{
	infoTexPBO.Bind();
	auto infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer());

	if (!losHandler->globalLOS[gu->myAllyTeam]) {
		const unsigned short* myAirLos = &losHandler->airLos.losMaps[gu->myAllyTeam].front();
		for (int y = 0; y < texSize.y; ++y) {
			for (int x = 0; x < texSize.x; ++x) {
				infoTexMem[y * texSize.x + x] = (myAirLos[y * texSize.x + x] != 0) ? 255 : 0;
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
		glViewport(0,0, texSize.x, texSize.y);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
		FBO::Unbind();

		glBindTexture(GL_TEXTURE_2D, texture);
		glGenerateMipmap(GL_TEXTURE_2D);
		return;
	}

	infoTexPBO.Bind();
	auto infoTexMem = reinterpret_cast<unsigned char*>(infoTexPBO.MapBuffer());
	const unsigned short* myAirLos = &losHandler->airLos.losMaps[gu->myAllyTeam].front();
	memcpy(infoTexMem, myAirLos, texSize.x * texSize.y * texChannels * sizeof(short));
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
	glViewport(0,0, texSize.x, texSize.y);
	shader->Enable();
	glDisable(GL_BLEND);
	glBegin(GL_QUADS);
		glVertex2f(-1.f, -1.f);
		glVertex2f(-1.f, +1.f);
		glVertex2f(+1.f, +1.f);
		glVertex2f(+1.f, -1.f);
	glEnd();
	shader->Disable();
	glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
	FBO::Unbind();

	// generate mipmaps
	glBindTexture(GL_TEXTURE_2D, texture);
	glGenerateMipmap(GL_TEXTURE_2D);
}
