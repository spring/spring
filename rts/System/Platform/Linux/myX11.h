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
 * @brief returns the window-state of the given window
 * @see MyX11SetWindowState()
 */
int MyX11GetWindowState(SDL_Window* window);

#endif // #ifndef HEADLESS

#endif // #ifndef MY_X11_H
