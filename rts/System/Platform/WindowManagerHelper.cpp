/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WindowManagerHelper.h"

#include <string>
#include <SDL_video.h>

#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "Game/GameVersion.h"


SDL_Surface* WindowManagerHelper::currentIcon = NULL;

void WindowManagerHelper::SetIcon(const CBitmap* icon) {
	if (SpringVersion::IsHeadless())
		return;

	if (icon != NULL) {
		// 24bit RGB or 32bit RGBA
		if (((icon->channels != 3) && (icon->channels != 4)) 
#ifdef    WIN32
			// on windows, the icon has to be 32x32
			|| (icon->xsize != 32)
			|| (icon->ysize != 32)
#endif // WIN32
			) {
			LOG_L(L_WARNING, "window-manager icon: Trying to set a window-manager icon with the wrong format.");
			LOG_L(L_WARNING, "window-manager icon: It has to be 24bit or 32bit, and on windows it additionally has to be 32x32 pixels.");
		} else {
			// supplied bitmap is usable as icon
			SDL_Surface* newIcon = icon->CreateSDLSurface(true);
			if (newIcon == NULL) {
				LOG_L(L_WARNING, "window-manager icon: Failed to create SDL surface, reason: %s", SDL_GetError());
			} else {
				SDL_WM_SetIcon(newIcon, NULL);
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

void WindowManagerHelper::FreeIcon() {
	SDL_WM_SetIcon(NULL, NULL);
	if (currentIcon != NULL) {
		// release the old icon
		unsigned char* pixelData = (unsigned char*) currentIcon->pixels;
		SDL_FreeSurface(currentIcon);
		delete[] pixelData;
		currentIcon = NULL;
	}
}

void WindowManagerHelper::SetCaption(const std::string& title, const std::string& titleShort) {

	// titleShort may only ever be used under X11, but not QT(KDE) or Windows
	// for more details, see:
	// http://www.gpwiki.org/index.php/SDL:Tutorials:Initializing_SDL_Libraries#About_the_icon_.28.22Icon_Title.22.29_parameter_on_different_window-managers
	SDL_WM_SetCaption(title.c_str(), titleShort.c_str());
}

void WindowManagerHelper::SetCaption(const std::string& title) {
	SetCaption(title, title);
}
