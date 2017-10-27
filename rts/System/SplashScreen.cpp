/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL.h>

#include "SplashScreen.hpp"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/FileSystem/ArchiveScanner.h"

#ifndef HEADLESS
// identity projection
static constexpr VA_TYPE_T QUAD_ELEMS[] = {
	{{-1.0f,  1.0f, 0.0f},   0.0f, 0.0f},
	{{-1.0f, -1.0f, 0.0f},   0.0f, 1.0f},
	{{ 1.0f, -1.0f, 0.0f},   1.0f, 1.0f},
	{{ 1.0f,  1.0f, 0.0f},   1.0f, 0.0f},
};

static constexpr uint32_t QUAD_INDCS[] = {0, 1, 2,   2, 3, 0};


static constexpr float4 TEXT_COLOR = {1.0f, 1.0f, 1.0f, 1.0f};
static constexpr float4 TEXT_COORS = {0.5f, 0.175f, 0.8f, 0.04f}; // x, y, scale, space

// not constexpr to circumvent a VS bug
// https://developercommunity.visualstudio.com/content/problem/10720/constexpr-function-accessing-character-array-leads.html
static const char* FMT_STRS[5] = {
	"[Initializing Virtual File System]",
	"* archives scanned: %u",
	"* scantime elapsed: %.1fms",
	"Spring %s",
	"This program is distributed under the GNU General Public License, see doc/LICENSE for more information.",
};



void ShowSplashScreen(
	const std::string& splashScreenFile,
	const std::string& springVersionStr,
	const std::function<bool()>& testDoneFunc
) {
	CBitmap bmp;

	// passing an empty name would cause bmp FileHandler to also
	// search inside the VFS since its default mode is RAW_FIRST
	if (splashScreenFile.empty() || !bmp.Load(splashScreenFile))
		bmp.AllocDummy({0, 0, 0, 0});

	const unsigned int splashTex = bmp.CreateTexture();
	const unsigned int fontFlags = FONT_NORM | FONT_SCALE | FONT_BUFFERED;


	const float textWidth[3] = {
		font->GetTextWidth(FMT_STRS[0]),
		font->GetTextWidth(FMT_STRS[4]),
		font->GetTextWidth("Spring " + springVersionStr),
	};
	const float normWidth[3] = {
		textWidth[0] * globalRendering->pixelX * font->GetSize() * TEXT_COORS.z,
		textWidth[1] * globalRendering->pixelX * font->GetSize() * TEXT_COORS.z,
		textWidth[2] * globalRendering->pixelX * font->GetSize() * TEXT_COORS.z,
	};


	GL::TRenderDataBuffer<VA_TYPE_T>* buffer = GL::GetRenderBufferT();
	Shader::IProgramObject* shader = buffer->GetShader();

	// slower than location-based SetUniform, but works without pre-initializing uniforms via CreateShader
	// template expects a TV but argument is const TV*, tell compiler it needs to invoke matrix::operator()
	shader->Enable();
	shader->SetUniformMatrix4x4<const char*, float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<const char*, float>("u_proj_mat", false, CMatrix44f::Identity());
	shader->SetUniform("u_tex0", 0);
	shader->Disable();


	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	for (spring_time t0 = spring_now(), t1 = t0; !testDoneFunc(); t1 = spring_now()) {
		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, splashTex);

		shader->Enable();
		buffer->Append(QUAD_ELEMS, sizeof(QUAD_ELEMS) / sizeof(QUAD_ELEMS[0]));
		buffer->Append(QUAD_INDCS, sizeof(QUAD_INDCS) / sizeof(QUAD_INDCS[0]));
		buffer->SubmitIndexed(GL_TRIANGLES);
		shader->Disable(); // font uses its own

		font->SetTextColor(TEXT_COLOR.x, TEXT_COLOR.y, TEXT_COLOR.z, TEXT_COLOR.w);
		font->glFormat(TEXT_COORS.x - (normWidth[0] * 0.500f), TEXT_COORS.y                                       , TEXT_COORS.z, fontFlags, FMT_STRS[0]);
		font->glFormat(TEXT_COORS.x - (normWidth[0] * 0.475f), TEXT_COORS.y - (TEXT_COORS.w * TEXT_COORS.z * 1.0f), TEXT_COORS.z, fontFlags, FMT_STRS[1], CArchiveScanner::GetNumScannedArchives());
		font->glFormat(TEXT_COORS.x - (normWidth[0] * 0.475f), TEXT_COORS.y - (TEXT_COORS.w * TEXT_COORS.z * 2.0f), TEXT_COORS.z, fontFlags, FMT_STRS[2], (t1 - t0).toMilliSecsf());

		// always render Spring's license notice (even on a white background)
		font->SetOutlineColor(0.0f, 0.0f, 0.0f, 0.65f);
		font->SetTextColor(TEXT_COLOR.x, TEXT_COLOR.y, TEXT_COLOR.z, TEXT_COLOR.w);
		font->glFormat(TEXT_COORS.x - (normWidth[2] * 0.5f), TEXT_COORS.y * 0.5f - (TEXT_COORS.w * TEXT_COORS.z * 1.0f), TEXT_COORS.z, fontFlags | FONT_OUTLINE, FMT_STRS[3], springVersionStr.c_str());
		font->glFormat(TEXT_COORS.x - (normWidth[1] * 0.5f), TEXT_COORS.y * 0.5f - (TEXT_COORS.w * TEXT_COORS.z * 2.0f), TEXT_COORS.z, fontFlags | FONT_OUTLINE, FMT_STRS[4]);
		font->DrawBufferedGL4();

		// also swaps <buffer>
		globalRendering->SwapBuffers(true, true);

		SDL_PollEvent(nullptr);
	}

	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &splashTex);
}
#endif

