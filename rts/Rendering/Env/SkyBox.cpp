/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SkyBox.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "System/float3.h"
#include "System/Log/ILog.h"


#define LOG_SECTION_SKY_BOX "SkyBox"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_SKY_BOX)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_SKY_BOX


// between 1 and 4 drawcalls per frame (opaque, water, env * 2)
// each call updates 6 vertices using a different camera and is
// double-buffered; this means buffer size must be the smallest
// common multiple of {1,2,3,4} to avoid flickering
static constexpr unsigned int SKYBOX_VERTEX_CNT = 6 * (2 * 3);
static constexpr unsigned int SKYBOX_BUFFER_LEN = SKYBOX_VERTEX_CNT * 2;

static_assert((SKYBOX_BUFFER_LEN % 4) == 0, "");


CSkyBox::CSkyBox(const std::string& texture)
{
	LoadTexture(texture);
	LoadBuffer();
}

CSkyBox::~CSkyBox()
{
	(skyBox.GetBuffer())->Kill();
}


void CSkyBox::LoadTexture(const std::string& texture)
{
	#ifndef HEADLESS
	CBitmap btex;

	if (!btex.Load(texture) || btex.textype != GL_TEXTURE_CUBE_MAP) {
		LOG_L(L_WARNING, "could not load skybox texture from file %s", texture.c_str());
		return;
	}

	skyTex.SetRawTexID(btex.CreateTexture());
	skyTex.SetRawSize(int2(btex.xsize, btex.ysize));

	glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	#endif
}

void CSkyBox::LoadBuffer()
{
	#ifndef HEADLESS
	const std::string& vsText = Shader::GetShaderSource("GLSL/SkyBoxVertProg.glsl");
	const std::string& fsText = Shader::GetShaderSource("GLSL/SkyBoxFragProg.glsl");

	const std::array<SkyBoxAttrType, 2> vertAttrs = {{
		{0,  2, GL_FLOAT,  (sizeof(float) * 5),  "a_vertex_xy"  , VA_TYPE_OFFSET(float, 0)},
		{1,  3, GL_FLOAT,  (sizeof(float) * 5),  "a_texcoor_xyz", VA_TYPE_OFFSET(float, 2)},
	}};

	static GL::RenderDataBuffer skyBuf;

	skyBuf.Kill();
	skyBox.Setup(&skyBuf, &vertAttrs, SKYBOX_BUFFER_LEN, 0);

	Shader::GLSLShaderObject shaderObjs[2] = {{GL_VERTEX_SHADER, vsText, ""}, {GL_FRAGMENT_SHADER, fsText, ""}};
	Shader::IProgramObject* shaderProg = skyBuf.CreateShader((sizeof(shaderObjs) / sizeof(shaderObjs[0])), 0, &shaderObjs[0], nullptr);

	shaderProg->Enable();
	shaderProg->SetUniform("u_skycube_tex", 0);
	shaderProg->SetUniform("u_gamma_exponent", globalRendering->gammaExponent);
	shaderProg->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shaderProg->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
	shaderProg->Disable();

	vtxPtr = skyBox.GetElemsMap();
	vtxPos = vtxPtr;

	memset(vtxPos, 0, SKYBOX_BUFFER_LEN * sizeof(SkyBoxVertType));
	#endif
}


void CSkyBox::Draw(Game::DrawMode mode)
{
	#ifndef HEADLESS
	if (!globalRendering->drawSky)
		return;

	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->DisableDepthMask();
	glAttribStatePtr->DisableDepthTest();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyTex.GetID());

	SkyBoxBuffer* buffer = &skyBox;
	Shader::IProgramObject* shader = buffer->GetShader();

	{
		if (vtxPos == vtxPtr)
			buffer->Wait();

		*(vtxPos++) = {{0.0f, 1.0f}, -camera->CalcPixelDir(                         0,                          0)};
		*(vtxPos++) = {{1.0f, 1.0f}, -camera->CalcPixelDir(globalRendering->viewSizeX,                          0)};
		*(vtxPos++) = {{1.0f, 0.0f}, -camera->CalcPixelDir(globalRendering->viewSizeX, globalRendering->viewSizeY)};

		*(vtxPos++) = {{1.0f, 0.0f}, -camera->CalcPixelDir(globalRendering->viewSizeX, globalRendering->viewSizeY)};
		*(vtxPos++) = {{0.0f, 0.0f}, -camera->CalcPixelDir(                         0, globalRendering->viewSizeY)};
		*(vtxPos++) = {{0.0f, 1.0f}, -camera->CalcPixelDir(                         0,                          0)};
	}
	{
		shader->Enable();
		shader->SetUniform("u_gamma_exponent", globalRendering->gammaExponent);
		buffer->Submit(GL_TRIANGLES, (((vtxPos - 6) - vtxPtr) + SKYBOX_VERTEX_CNT) % SKYBOX_BUFFER_LEN, 6);
		shader->Disable();
	}

	// wraparound
	if ((vtxPos - vtxPtr) >= SKYBOX_BUFFER_LEN)
		vtxPos = vtxPtr;
	if (vtxPos == vtxPtr)
		buffer->Sync();

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glAttribStatePtr->EnableDepthMask();
	glAttribStatePtr->EnableDepthTest();
	#endif
}

