/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MetalExtraction.h"
#include "InfoTextureHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/LosHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"



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

	if (FBO::IsSupported()) {
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CMetalExtractionTexture");
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
			gl_FragColor = texture2D(tex0, texCoord) * 800.0;
		}
	)";

	shader = shaderHandler->CreateProgramObject("[CMetalExtractionTexture]", "CMetalExtractionTexture");
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

	if (!fbo.IsValid() || !shader->IsValid()) {
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

	const      CMetalMap* metalMap        = readMap->metalMap;
	const          float* extractDepthMap = metalMap->GetExtractionMap();
//	const unsigned short* myAirLos        = &losHandler->airLosMaps[gu->myAllyTeam].front();
	assert(metalMap->GetSizeX() == texSize.x && metalMap->GetSizeZ() == texSize.y);

	// upload raw data to gpu
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texSize.x, texSize.y, GL_RED, GL_FLOAT, extractDepthMap);

	// do post-processing on the gpu (los-checking & scaling)
	fbo.Bind();
	shader->Enable();
	glViewport(0,0, texSize.x, texSize.y);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetInfoTexture("los")->GetTexture());
	glBegin(GL_QUADS);
		glVertex2f(-1.f, -1.f);
		glVertex2f(-1.f, +1.f);
		glVertex2f(+1.f, +1.f);
		glVertex2f(+1.f, -1.f);
	glEnd();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDisable(GL_BLEND);
	glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
	shader->Disable();
	FBO::Unbind();
}
