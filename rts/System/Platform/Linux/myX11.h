/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <X11/Xlib.h>
#undef KeyPress
#undef KeyRelease
#undef GrayScale

void MyX11GetFrameBorderOffset(Display* display, Window& window, int* out_left, int* out_top)

int MyX11GetWindowState(Display* display, Window& window)
void MyX11SetWindowState(Display* display, Window& window, int windowState)
