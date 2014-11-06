/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Combiner.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"


CONFIG(bool, HighResInfoTexture).defaultValue(true);


CInfoTextureCombiner::CInfoTextureCombiner()
: CPboInfoTexture("info")
, disabled(true)
{
	texSize = (configHandler->GetBool("HighResInfoTexture")) ? int2(gs->pwr2mapx, gs->pwr2mapy) : int2(gs->pwr2mapx >> 1, gs->pwr2mapy >> 1);
	//texSize = (configHandler->GetBool("HighResInfoTexture")) ? int2(512, 512) : int2(256, 256);
	texChannels = 4;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, -1, GL_RGBA8, texSize.x, texSize.y);

	if (FBO::IsSupported()) {
		fbo.Bind();
		fbo.AttachTexture(texture);
		/*bool status =*/ fbo.CheckStatus("CInfoTextureCombiner");
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		if (fbo.IsValid()) glClear(GL_COLOR_BUFFER_BIT);
		FBO::Unbind();

		// create mipmaps
		glBindTexture(GL_TEXTURE_2D, texture);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	shader = shaderHandler->CreateProgramObject("[CInfoTextureCombiner]", "CInfoTextureCombiner", false);
}


void CInfoTextureCombiner::SwitchMode(const std::string& name)
{
	if (name.empty()) {
		curMode = name;
		disabled = true;
		CreateShader("", true);
		return;
	}

	if (name == "los") {
		disabled = !CreateShader("shaders/GLSL/infoLOS.lua");
	} else
	if (name == "metal") {
		disabled = !CreateShader("shaders/GLSL/infoMetal.lua", true, float4(0.f, 0.f, 0.f, 1.0f));
	} else
	if (name == "height") {
		disabled = !CreateShader("shaders/GLSL/infoHeight.lua");
	} else
	if (name == "path") {
		disabled = !CreateShader("shaders/GLSL/infoPath.lua");
	} else {
		//FIXME allow "info:myluainfotex"
		disabled = !CreateShader(name);
	}

	curMode = (disabled) ? "" : name;
}


bool CInfoTextureCombiner::CreateShader(const std::string& filename, const bool clear, const float4 clearColor)
{
	if (clear) {
		// clear
		fbo.Bind();
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		if (fbo.IsValid()) glClear(GL_COLOR_BUFFER_BIT);
		FBO::Unbind();

		// create mipmaps
		glBindTexture(GL_TEXTURE_2D, texture);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	if (filename.empty())
		return false;

	shader->Release();
	return shader->LoadFromLua(filename);
}


void CInfoTextureCombiner::Update()
{
	shader->Enable();
	fbo.Bind();
	glViewport(0,0, texSize.x, texSize.y);
	glEnable(GL_BLEND);

	shader->BindTextures();

	const float isx = 2.0f * (gs->mapx / float(gs->pwr2mapx)) - 1.0f;
	const float isy = 2.0f * (gs->mapy / float(gs->pwr2mapy)) - 1.0f;

	glBegin(GL_QUADS);
		glTexCoord2f(0.f, 0.f); glVertex2f(-1.f, -1.f);
		glTexCoord2f(0.f, 1.f); glVertex2f(-1.f, +isy);
		glTexCoord2f(1.f, 1.f); glVertex2f(+isx, +isy);
		glTexCoord2f(1.f, 0.f); glVertex2f(+isx, -1.f);
	glEnd();

	glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
	FBO::Unbind();
	shader->Disable();

	// create mipmaps
	glBindTexture(GL_TEXTURE_2D, texture);
	glGenerateMipmap(GL_TEXTURE_2D);
}
