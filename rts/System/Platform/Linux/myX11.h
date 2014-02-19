/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MY_X11_H
#define MY_X11_H

#ifndef HEADLESS

#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease
#undef GrayScale

struct SDL_Window;


/**
 * @brief disables kwin compositing to fix tearing
 */
void MyX11BlockCompositing(SDL_Window* window);


/**
 * @brief returns the window-state of the given window
 */
int MyX11GetWindowState(SDL_Window* window);

#endif // #ifndef HEADLESS

#endif // #ifndef MY_X11_H
