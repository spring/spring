/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>
#include <algorithm>

#include "SkyBox.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Env/DebugCubeMapTexture.h"
#include "Game/Game.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "System/Exceptions.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Color.h"
#include "System/Log/ILog.h"

#define LOG_SECTION_SKY_BOX "SkyBox"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SKY_BOX)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SKY_BOX


CSkyBox::CSkyBox(const std::string& texture)
	: skyVAO()
	, shader{nullptr}
{
#ifndef HEADLESS
	CBitmap btex;
	if (!btex.Load(texture) || btex.textype != GL_TEXTURE_CUBE_MAP) {
		LOG_L(L_WARNING, "could not load skybox texture from file %s", texture.c_str());
	} else {
		skyTex.SetRawTexID(btex.CreateTexture());
		skyTex.SetRawSize(int2(btex.xsize, btex.ysize));
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	shader = shaderHandler->CreateProgramObject("[SkyBox]", "SkyBox");
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapVS.glsl", "", GL_VERTEX_SHADER));
	shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/CubeMapFS.glsl", "", GL_FRAGMENT_SHADER));
	shader->Link();
	shader->Enable();
	shader->SetUniform("skybox", 0);
	shader->Disable();
	shader->Validate();
#endif
	globalRendering->drawFog = (fogStart <= 0.99f);
}

CSkyBox::~CSkyBox()
{
#ifndef HEADLESS
	shaderHandler->ReleaseProgramObject("[SkyBox]", "SkyBox");
#endif
}

void CSkyBox::Draw()
{
#ifndef HEADLESS
	if (!globalRendering->drawSky)
		return;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	//glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	CMatrix44f view = camera->GetViewMatrix();
	view.SetPos(float3());
	glLoadMatrixf(view);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(camera->GetProjectionMatrix());

	if (globalRendering->drawDebugCubeMap)
		glBindTexture(GL_TEXTURE_CUBE_MAP, debugCubeMapTexture.GetId());
	else
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());

	skyVAO.Bind();
	assert(shader->IsValid());
	shader->Enable();

	glDrawArrays(GL_TRIANGLES, 0, 36);

	shader->Disable();
	skyVAO.Unbind();

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	// glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	sky->SetupFog();

	glEnable(GL_DEPTH_TEST);
#endif
}