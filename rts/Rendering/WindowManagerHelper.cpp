/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "StdAfx.h"
#include "WindowManagerHelper.h"

#include <string>
#include <SDL_video.h>

#include "Rendering/Textures/Bitmap.h"
#include "System/LogOutput.h"


SDL_Surface* WindowManagerHelper::currentIcon = NULL;

void WindowManagerHelper::SetIcon(const CBitmap* icon) {

	if (icon != NULL) {
		// 24bit RGB or 32bit RBA
		if (((icon->channels != 3) && (icon->channels != 4)) 
#ifdef    WIN32
			// on windows, the icon has to be 32x32
			|| (icon->xsize != 32)
			|| (icon->ysize != 32)
#endif // WIN32
			) {
			logOutput.Print("Warning: window-manager icon: Trying to set a window-manager icon with the wrong format.");
			logOutput.Print("Warning: window-manager icon: It has to be 24bit or 32bit, and on windows it additionally has to be 32x32 pixels.");
		} else {
			// supplied bitmap is usable as icon
			SDL_Surface* newIcon = icon->CreateSDLSurface(true);
			if (newIcon == NULL) {
				logOutput.Print("Warning: window-manager icon: Failed to create SDL surface, reason: %s", SDL_GetError());
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

void WindowManagerHelper::SetCaption(const std::string& caption) {
	SDL_WM_SetCaption(caption.c_str(), caption.c_str());
}
