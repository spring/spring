/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WindowManagerHelper.h"

#include <string>
#include <SDL_video.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "Game/GameVersion.h"


namespace WindowManagerHelper {

struct WindowIcon {
	CBitmap bmp;
	SDL_Surface* surf;
};

static WindowIcon windowIcon = {{}, nullptr};


void SetIcon(CBitmap* bmp) {
	if (SpringVersion::IsHeadless())
		return;

	// 24bit RGB or 32bit RGBA
	if (((bmp->channels != 3) && (bmp->channels != 4))
//#ifdef    WIN32
		// on windows, the icon has to be 32x32
		|| (bmp->xsize != 32)
		|| (bmp->ysize != 32)
//#endif
	) {
		LOG_L(L_WARNING, "window-manager icon: Trying to set a window-manager icon with the wrong format.");
		LOG_L(L_WARNING, "window-manager icon: It has to be 24bit or 32bit, and on windows it additionally has to be 32x32 pixels.");
		return;
	}

	// supplied bitmap is usable as icon, keep it
	if (SetIconSurface(bmp)) {
		windowIcon.bmp = std::move(*bmp);
	}
}


bool SetIconSurface(CBitmap* bmp) {
	assert(globalRendering != nullptr);

	if (bmp == nullptr) {
		// only reached on exit
		SDL_FreeSurface(windowIcon.surf);
		SDL_SetWindowIcon(globalRendering->window, windowIcon.surf = nullptr);
		return false;
	}

	SDL_Surface* surf = bmp->CreateSDLSurface();

	if (surf == nullptr) {
		// keep any previous surface in case of failure
		LOG_L(L_WARNING, "[WindowManagerHelper::%s] failed to create SDL surface, reason: %s", __func__, SDL_GetError());
		return false;
	}

	SDL_FreeSurface(windowIcon.surf);
	SDL_SetWindowIcon(globalRendering->window, windowIcon.surf = surf);
	return true;
}


void SetCaption(const std::string& title) {
	SDL_SetWindowTitle(globalRendering->window, title.c_str());
}

}; // namespace WindowManagerHelper

