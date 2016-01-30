/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WindowManagerHelper.h"

#include <string>
#include <SDL_video.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "Game/GameVersion.h"


namespace WindowManagerHelper {

static SDL_Surface* currentIcon = nullptr;

void SetIcon(const CBitmap* icon) {
	if (SpringVersion::IsHeadless())
		return;

	if (icon != NULL) {
		// 24bit RGB or 32bit RGBA
		if (((icon->channels != 3) && (icon->channels != 4))
//#ifdef    WIN32
			// on windows, the icon has to be 32x32
			|| (icon->xsize != 32)
			|| (icon->ysize != 32)
//#endif
			) {
			LOG_L(L_WARNING, "window-manager icon: Trying to set a window-manager icon with the wrong format.");
			LOG_L(L_WARNING, "window-manager icon: It has to be 24bit or 32bit, and on windows it additionally has to be 32x32 pixels.");
		} else {
			// supplied bitmap is usable as icon
			SDL_Surface* newIcon = icon->CreateSDLSurface();
			if (newIcon == NULL) {
				LOG_L(L_WARNING, "window-manager icon: Failed to create SDL surface, reason: %s", SDL_GetError());
			} else {
				SDL_SetWindowIcon(globalRendering->window, newIcon);
				if (currentIcon != NULL) {
					// release the old icon
					unsigned char* pixelData = (unsigned char*) currentIcon->pixels;
					SDL_FreeSurface(currentIcon);
					delete[] pixelData;
					currentIcon = NULL;
				}
				currentIcon = newIcon;
			}
		}
	}
}


void FreeIcon() {
	if (globalRendering != NULL)
		SDL_SetWindowIcon(globalRendering->window, NULL);

	if (currentIcon != NULL) {
		// release the old icon
		unsigned char* pixelData = (unsigned char*) currentIcon->pixels;
		SDL_FreeSurface(currentIcon);
		delete[] pixelData;
		currentIcon = NULL;
	}
}


void SetCaption(const std::string& title) {
	SDL_SetWindowTitle(globalRendering->window, title.c_str());
}

}; // namespace WindowManagerHelper
