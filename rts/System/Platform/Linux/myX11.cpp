/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEADLESS

#include "StdAfx.h"
#include "myX11.h"

#include <string.h>
#include <string>
#include <map>

#include <SDL.h>
#if !defined(HEADLESS)
	#include <SDL_syswm.h>
#endif


void MyX11GetFrameBorderOffset(Display* display, Window& window, int* out_left, int* out_top)
{
	// XGetWindowProperty stuff
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_remaining;
	int* data;
	int status;

	Atom net_frame_extents = XInternAtom(display, "_NET_FRAME_EXTENTS", True);

	status = XGetWindowProperty(
		display,
		window,
		net_frame_extents,
		0,      // long_offset
		4,      // long_length - we expect 4 32-bit values for _NET_FRAME_EXTENTS
		false,  // delete
		AnyPropertyType,
		&actual_type,
		&actual_format,
		&nitems,
		&bytes_remaining,
		(unsigned char**)&data);

	*out_left = *out_top = 0;
	if (status == Success) {
		if ((nitems == 4) && (bytes_remaining == 0)) {
			*out_left = data[0];
			*out_top  = data[2];
		}
		XFree(data);
	}
}


int MyX11GetWindowState(Display* display, Window& window)
{
	// XGetWindowProperty stuff
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_remaining;
	Atom* data;
	int status;

	Atom atom = XInternAtom(display, "_NET_WM_STATE", true);

	const std::string windowStatesStr[8] = {
		"_NET_WM_STATE_STICKY",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_SHADED",
		"_NET_WM_STATE_HIDDEN",
		"_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE_ABOVE",
		"_NET_WM_STATE_BELOW"
	};

	Atom windowStatesAtoms[8];
	std::map<Atom,int> windowStatesInt;
	for (int i=0; i<8; i++) {
		windowStatesAtoms[i] = XInternAtom(display, windowStatesStr[i].c_str(), false);
		windowStatesInt[windowStatesAtoms[i]] = 1<<i;
	}

	status = XGetWindowProperty(
		display,
		window,
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
		int windowState = 0;
		for (int i=0; i<nitems; i++) {
			Atom& a = data[i];
			std::map<Atom,int>::iterator it = windowStatesInt.find(a);
			if (it != windowStatesInt.end()) {
				windowState += it->second;
			}
		}
		XFree(data);
		return windowState;
	}
	return 0;
}


void MyX11SetWindowState(Display* display, Window& window, int windowState)
{
	if (windowState <= 0)
		return;

	const std::string windowStatesStr[8] = {
		"_NET_WM_STATE_STICKY",
		"_NET_WM_STATE_MAXIMIZED_VERT",
		"_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_SHADED",
		"_NET_WM_STATE_HIDDEN",
		"_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE_ABOVE",
		"_NET_WM_STATE_BELOW"
	};

	Atom windowStatesAtoms[8];
	for (int i=0; i<8; i++) {
		windowStatesAtoms[i] = XInternAtom(display, windowStatesStr[i].c_str(), false);
	}

	Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);

	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = window;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;

	Window root = XDefaultRootWindow(display);

	static const int _NET_WM_STATE_REMOVE = 0;
	static const int _NET_WM_STATE_ADD = 1;

	for (int i=0; i<8; i++) {
		if (1<<i & windowState) {
			xev.xclient.data.l[0] = _NET_WM_STATE_ADD;
			xev.xclient.data.l[1] = windowStatesAtoms[i];
			XSendEvent(display, root, false, SubstructureNotifyMask, &xev);
		} else {
			xev.xclient.data.l[0] = _NET_WM_STATE_REMOVE;
			xev.xclient.data.l[1] = windowStatesAtoms[i];
			XSendEvent(display, root, false, SubstructureNotifyMask, &xev);
		}
	}
}

#endif // #ifndef HEADLESS