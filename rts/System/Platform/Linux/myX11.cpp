/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEADLESS

#include "myX11.h"
#include <SDL_syswm.h>


void MyX11BlockCompositing(SDL_Window* window)
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info))
		return;

	auto x11display = info.info.x11.display;
	auto x11window  = info.info.x11.window;

	bool b = true;
	Atom blockCompositAtom = XInternAtom(x11display, "_KDE_NET_WM_BLOCK_COMPOSITING", false);
	XChangeProperty(x11display, x11window, blockCompositAtom, XA_INTEGER, 8, PropModeReplace, (const unsigned char*)&b, 1);
}


int MyX11GetWindowState(SDL_Window* window)
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info))
		return 0;

	auto x11display = info.info.x11.display;
	auto x11window  = info.info.x11.window;

	// XGetWindowProperty stuff
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_remaining;
	Atom* data;
	int status;

	Atom atom = XInternAtom(x11display, "_NET_WM_STATE", true);

	Atom maxVAtom = XInternAtom(x11display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
	Atom maxHAtom = XInternAtom(x11display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	Atom  minAtom = XInternAtom(x11display, "_NET_WM_STATE_HIDDEN", false);

	status = XGetWindowProperty(
		x11display,
		x11window,
		atom,
		0,
		20,
		false,
		AnyPropertyType,
		&actual_type,
		&actual_format,
		&nitems,
		&bytes_remaining,
		(unsigned char**)&data);

	if (status == Success) {
		int maximized = 0;
		bool minimized = false;
		for (int i=0; i<nitems; i++) {
			Atom& a = data[i];
			if (a == maxVAtom) {
				maximized |= 1;
			} else
			if (a == maxHAtom) {
				maximized |= 2;
			} else
			if (a == minAtom) {
				minimized = true;
			}
		}
		XFree(data);

		int flags = 0;
		flags |= (maximized == 3) ? SDL_WINDOW_MAXIMIZED : 0;
		flags |= (minimized) ? SDL_WINDOW_MINIMIZED : 0;
		return flags;
	}
	return 0;
}
#endif // #ifndef HEADLESS
