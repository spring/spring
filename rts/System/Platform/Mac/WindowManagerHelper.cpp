/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/WindowManagerHelper.h"
#include <SDL_video.h>


namespace WindowManagerHelper {

void BlockCompositing(SDL_Window* window)
{
	//FIXME implement?
}


int GetWindowState(SDL_Window* window)
{
	return SDL_GetWindowFlags(window);
}


void SetWindowResizable(SDL_Window* window, bool resizable)
{
	//FIXME implement?
}

}; // namespace WindowManagerHelper
