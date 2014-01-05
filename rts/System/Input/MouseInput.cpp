/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
	This workaround fixes the windows slow mouse movement problem
	(happens on full-screen mode + pressing keys).
	The code hacks around the mouse input from DirectInput,
	which SDL uses in full-screen mode.
	Instead it installs a window message proc and reads input from WM_MOUSEMOVE.
	On non-windows, the normal SDL events are used for mouse input

	new:
	It also workarounds a issue with SDL+windows and hardware cursors
	(->it has to block WM_SETCURSOR),
	so it is used now always even in window mode!

	newer:
	SDL_Event struct is used for new input handling.
	Several people confirmed its working.
*/


#include "MouseInput.h"
#include "InputHandler.h"

#include "Game/GlobalUnsynced.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "System/maindefines.h"

#include <boost/bind.hpp>
#include <SDL_events.h>
#ifdef _WIN32
	#include <SDL_syswm.h>
	#include "System/Platform/Win/wsdl.h"
#endif
#if defined(_X11) && !defined(HEADLESS)
	#include <SDL_syswm.h>
	#include <X11/Xlib.h>
#endif


IMouseInput* mouseInput = NULL;


IMouseInput::IMouseInput()
{
	inputCon = input.AddHandler(boost::bind(&IMouseInput::HandleSDLMouseEvent, this, _1));
}


bool IMouseInput::HandleSDLMouseEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_MOUSEMOTION: {
			mousepos = int2(event.motion.x, event.motion.y);
			if (mouse) {
				mouse->MouseMove(mousepos.x, mousepos.y, event.motion.xrel, event.motion.yrel);
			}
		} break;
		case SDL_MOUSEBUTTONDOWN: {
			mousepos = int2(event.button.x, event.button.y);
			if (mouse) {
				mouse->MousePress(mousepos.x, mousepos.y, event.button.button);
			}
		} break;
		case SDL_MOUSEBUTTONUP: {
			mousepos = int2(event.button.x, event.button.y);
			if (mouse) {
				mouse->MouseRelease(mousepos.x, mousepos.y, event.button.button);
			}
		} break;
		case SDL_MOUSEWHEEL: {
			if (mouse) {
				mouse->MouseWheel(event.wheel.y);
			}
		} break;
		case SDL_WINDOWEVENT: {
			if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
				// mouse left window (set mouse pos internally to window center to prevent endless scrolling)
				mousepos.x = globalRendering->viewSizeX / 2;
				mousepos.y = globalRendering->viewSizeY / 2;
				if (mouse) {
					mouse->MouseMove(mousepos.x, mousepos.y, 0, 0);
				}
			}
		} break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////

#ifdef WIN32

class CWin32MouseInput : public IMouseInput
{
public:
	static CWin32MouseInput *inst;

	LONG_PTR sdl_wndproc;
	HWND wnd;
	HCURSOR hCursor;

	static LRESULT CALLBACK SpringWndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (mouse) {
			switch (msg) {
				case WM_XBUTTONDOWN:
				{
					if ((short)LOWORD(wParam) & MK_XBUTTON1)
						mouse->MousePress((short)LOWORD(lParam), (short)HIWORD(lParam), 4);
					if ((short)LOWORD(wParam) & MK_XBUTTON2)
						mouse->MousePress((short)LOWORD(lParam), (short)HIWORD(lParam), 5);
				} return 0;
				case WM_XBUTTONUP:
				{
					if ((short)LOWORD(wParam) & MK_XBUTTON1)
						mouse->MouseRelease((short)LOWORD(lParam), (short)HIWORD(lParam), 4);
					if ((short)LOWORD(wParam) & MK_XBUTTON2)
						mouse->MouseRelease((short)LOWORD(lParam), (short)HIWORD(lParam), 5);
				} return 0;
			}
		}

		switch (msg) {
			case WM_MOUSEMOVE:
				return wsdl::OnMouseMotion(wnd, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)wParam);

			case WM_MOUSEWHEEL:
				return wsdl::OnMouseWheel(wnd, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (int)(short)HIWORD(wParam), (UINT)(short)LOWORD(wParam));

			//! can't use message crackers: they do not provide for passing uMsg
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				return wsdl::OnMouseButton(wnd, msg, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)wParam);

			case WM_SETCURSOR:
			{
				if (inst->hCursor!=NULL) {
					Uint16 hittest = LOWORD(lParam);
					if ( hittest == HTCLIENT ) {
						SetCursor(inst->hCursor);
						return TRUE;
					}
				}
			} break;

			case WM_ACTIVATE:
			{
				wsdl::ResetMouseButtons();
				// wsdl::OnActivate(wnd, LOWORD(wParam), NULL, HIWORD(lParam));
				// FIXME: move to SpringApp somehow and use GLContext.h instead!
				if (globalRendering->fullScreen) {
					if (LOWORD(wParam) == WA_INACTIVE) {
						FBO::GLContextLost();
					} else if (LOWORD(wParam) == WA_ACTIVE) {
						FBO::GLContextReinit();
					}
				}
			} break;
		}
		return CallWindowProc((WNDPROC)inst->sdl_wndproc, wnd, msg, wParam, lParam);
	}

	void SetWMMouseCursor(void* wmcursor)
	{
		hCursor = (HCURSOR)wmcursor;
	}

	void InstallWndCallback()
	{
		sdl_wndproc = GetWindowLongPtr(wnd, GWLP_WNDPROC);
		SetWindowLongPtr(wnd,GWLP_WNDPROC,(LONG_PTR)SpringWndProc);
	}

	CWin32MouseInput()
	{
		inst = this;

		hCursor = NULL;

		sdl_wndproc = 0;

		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if(!SDL_GetWindowWMInfo(globalRendering->window, &info))
			return;

		wnd = info.win.window;
		wsdl::Init(wnd);

		InstallWndCallback();
	}
	~CWin32MouseInput()
	{
		//! reinstall the SDL window proc
		SetWindowLongPtr(wnd, GWLP_WNDPROC, sdl_wndproc);
	}
};
CWin32MouseInput* CWin32MouseInput::inst = 0;
#endif

void IMouseInput::SetPos(int2 pos)
{
	if (!globalRendering->active) {
		return;
	}

	mousepos = pos;
	int2 curpos;
#ifndef WIN32 // seems SDL_GetMouseState isn't always updated on windows when cursor is hidden? (2012 - untested)
	SDL_GetMouseState(&curpos.x, &curpos.y);
	if (pos.x == curpos.x && pos.y == curpos.y) {
		// calling SDL_WarpMouse at 300fps eats ~5% cpu usage, so only update when needed
		return;
	}
#endif

#ifdef WIN32
	wsdl::SDL_WarpMouse(pos.x, pos.y);
#else
	SDL_WarpMouseInWindow(globalRendering->window, pos.x, pos.y);

	#if defined(_X11) && !defined(HEADLESS)
		// SDL Workaround!
		// SDL_WarpMouse has a bug on Linux in fullscreen mode & SDL_ShowCursor(SDL_DISABLE).
		// It just won't move the real X11 cursor pos (instead it seems to update some SDL internal vars only).
		// But we use SDL_ShowCursor for MiddleClickScroll and want that the cursor spawns
		// at screen center when calling SDL_ShowCursor(SDL_ENABLE), so we call X11 here to
		// force cursor pos update even when the cursor is hidden.
		/*if (globalRendering->fullScreen) {
			SDL_SysWMinfo info;
			SDL_VERSION(&info.version);
			if(SDL_GetWindowWMInfo(globalRendering->window, &info)) {
				//info.info.x11.lock_func();
					Display* display = info.info.x11.display;
					Window& window = info.info.x11.window;

					if (display && window != None)
						XWarpPointer(display, None, window, 0, 0, 0, 0, pos.x, pos.y);
				//info.info.x11.unlock_func();
			}
		}*/
	#endif
#endif

	// SDL_WarpMouse generates SDL_MOUSEMOTION events
	// in `middle click scrolling` those SDL generated ones would point into
	// the oppossite direction the user moved the mouse, and so events would
	// cancel each other -> camera wouldn't move at all
	// so we need to catch those SDL generated events and delete them

	// delete all SDL_MOUSEMOTION in the queue
	SDL_PumpEvents();
	static SDL_Event events[100];
	SDL_PeepEvents(&events[0], 100, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION);
}



IMouseInput* IMouseInput::GetInstance()
{
	if (mouseInput == NULL) {
#ifdef WIN32
		mouseInput = new CWin32MouseInput;
#else
		mouseInput = new IMouseInput;
#endif
	}
	return mouseInput;
}

void IMouseInput::FreeInstance(IMouseInput* mouseInp) {
	delete mouseInp; mouseInput = NULL;
}
