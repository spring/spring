/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL.h>

#include "SplashScreen.hpp"
#include "Game/GameVersion.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/float4.h"
#include "System/FileSystem/ArchiveScanner.h"

#ifndef HEADLESS
void ShowSplashScreen(const std::string& splashScreenFile, const std::function<bool()>& testDoneFunc)
{
	CVertexArray* va = GetVertexArray();
	CBitmap bmp;

	// passing an empty name would cause bmp FileHandler to also
	// search inside the VFS since its default mode is RAW_FIRST
	if (splashScreenFile.empty() || !bmp.Load(splashScreenFile))
		bmp.AllocDummy({0, 0, 0, 0});

	// not constexpr to circumvent a VS bug
	// https://developercommunity.visualstudio.com/content/problem/10720/constexpr-function-accessing-character-array-leads.html
	const char* fmtStrs[5] = {
		"[Initializing Virtual File System]",
		"* archives scanned: %u",
		"* scantime elapsed: %.1fs",
		"Spring %s",
		"This program is distributed under the GNU General Public License, see doc/LICENSE for more information.",
	};

	char versionStrBuf[512];

	memset(versionStrBuf, 0, sizeof(versionStrBuf));
	snprintf(versionStrBuf, sizeof(versionStrBuf), fmtStrs[3], (SpringVersion::GetFull()).c_str());

	const unsigned int splashTex = bmp.CreateTexture();
	const unsigned int fontFlags = FONT_NORM | FONT_SCALE;

	const float4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	const float4 coors = {0.5f, 0.175f, 0.8f, 0.04f}; // x, y, scale, space

	const float textWidth[3] = {font->GetTextWidth(fmtStrs[0]), font->GetTextWidth(fmtStrs[4]), font->GetTextWidth(versionStrBuf)};
	const float normWidth[3] = {
		textWidth[0] * globalRendering->pixelX * font->GetSize() * coors.z,
		textWidth[1] * globalRendering->pixelX * font->GetSize() * coors.z,
		textWidth[2] * globalRendering->pixelX * font->GetSize() * coors.z,
	};

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);

	for (spring_time t0 = spring_now(), t1 = t0; !testDoneFunc(); t1 = spring_now()) {
		glClear(GL_COLOR_BUFFER_BIT);

		glBindTexture(GL_TEXTURE_2D, splashTex);
		va->Initialize();
		va->AddVertex2dT({ZeroVector.x, 1.0f - ZeroVector.y}, {0.0f, 0.0f});
		va->AddVertex2dT({  UpVector.x, 1.0f -   UpVector.y}, {0.0f, 1.0f});
		va->AddVertex2dT({  XYVector.x, 1.0f -   XYVector.y}, {1.0f, 1.0f});
		va->AddVertex2dT({ RgtVector.x, 1.0f -  RgtVector.y}, {1.0f, 0.0f});
		va->DrawArray2dT(GL_QUADS);

		font->Begin();
		font->SetTextColor(color.x, color.y, color.z, color.w);
		font->glFormat(coors.x - (normWidth[0] * 0.500f), coors.y                             , coors.z, fontFlags, fmtStrs[0]);
		font->glFormat(coors.x - (normWidth[0] * 0.475f), coors.y - (coors.w * coors.z * 1.0f), coors.z, fontFlags, fmtStrs[1], CArchiveScanner::GetNumScannedArchives());
		font->glFormat(coors.x - (normWidth[0] * 0.475f), coors.y - (coors.w * coors.z * 2.0f), coors.z, fontFlags, fmtStrs[2], (t1 - t0).toSecsf());
		font->End();

		// always render Spring's license notice
		font->Begin();
		font->SetOutlineColor(0.0f, 0.0f, 0.0f, 0.65f);
		font->SetTextColor(color.x, color.y, color.z, color.w);
		font->glFormat(coors.x - (normWidth[2] * 0.5f), coors.y * 0.5f - (coors.w * coors.z * 1.0f), coors.z, fontFlags | FONT_OUTLINE, versionStrBuf);
		font->glFormat(coors.x - (normWidth[1] * 0.5f), coors.y * 0.5f - (coors.w * coors.z * 2.0f), coors.z, fontFlags | FONT_OUTLINE, fmtStrs[4]);
		font->End();

		globalRendering->SwapBuffers(true, true);

		// prevent WM's from assuming the window is unresponsive and
		// (in recent versions of Windows) generating a kill-request
		SDL_PollEvent(nullptr);
	}

	glPopAttrib();
	glDeleteTextures(1, &splashTex);
}
#endif

