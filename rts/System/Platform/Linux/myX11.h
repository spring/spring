/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MY_X11_H
#define MY_X11_H

#ifndef HEADLESS

#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease
#undef GrayScale

/**
 * @brief returns the offset of a window to the display
 * @param display  The display
 * @param window   The window
 * @param out_left Pointer to an int, that will hold the x offset
 * @param out_top  Pointer to an int, that will hold the y offset
 */
void MyX11GetFrameBorderOffset(Display* display, Window& window, int* out_left, int* out_top);

/**
 * @brief returns the window-state of the given window
 * @see MyX11SetWindowState()
 */
int MyX11GetWindowState(Display* display, Window& window);

/**
 * @brief sets the window to windowState (maximized, minimized, ...)
 * @see MyX11GetWindowState()
 */
void MyX11SetWindowState(Display* display, Window& window, int windowState);

#endif // #ifndef HEADLESS

#endif // #ifndef MY_X11_H
