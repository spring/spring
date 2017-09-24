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
#include "System/MainDefines.h"
#include "System/SafeUtil.h"

#include <functional>
#include <SDL_events.h>
#include <SDL_syswm.h>


IMouseInput* mouseInput = nullptr;


IMouseInput::IMouseInput()
{
	inputCon = input.AddHandler(std::bind(&IMouseInput::HandleSDLMouseEvent, this, std::placeholders::_1));
}

IMouseInput::~IMouseInput()
{
	inputCon.disconnect();
}


bool IMouseInput::HandleSDLMouseEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_MOUSEMOTION: {
			mousepos = int2(event.motion.x, event.motion.y);

			if (mouse != nullptr)
				mouse->MouseMove(mousepos.x, mousepos.y, event.motion.xrel, event.motion.yrel);

		} break;
		case SDL_MOUSEBUTTONDOWN: {
			mousepos = int2(event.button.x, event.button.y);

			if (mouse != nullptr)
				mouse->MousePress(mousepos.x, mousepos.y, event.button.button);

		} break;
		case SDL_MOUSEBUTTONUP: {
			mousepos = int2(event.button.x, event.button.y);

			if (mouse != nullptr)
				mouse->MouseRelease(mousepos.x, mousepos.y, event.button.button);

		} break;
		case SDL_MOUSEWHEEL: {
			if (mouse != nullptr)
				mouse->MouseWheel(event.wheel.y);

		} break;
		case SDL_WINDOWEVENT: {
			if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
				// mouse left window; set pos internally to center-pixel to prevent endless scrolling
				mousepos = globalRendering->GetScreenCenter();

				if (mouse != nullptr)
					mouse->MouseMove(-mousepos.x, -mousepos.y, 0, 0);

			} break;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////

#if defined(WIN32) && !defined (HEADLESS)

class CWin32MouseInput : public IMouseInput
{
public:
	static CWin32MouseInput *inst;

	LONG_PTR sdl_wndproc;
	HWND wnd;
	HCURSOR hCursor;

	static LRESULT CALLBACK SpringWndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
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
		}
		return CallWindowProc((WNDPROC)inst->sdl_wndproc, wnd, msg, wParam, lParam);
	}

	void SetWMMouseCursor(void* wmcursor)
	{
		hCursor = (HCURSOR)wmcursor;
	}

	void InstallWndCallback()
	{
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (!SDL_GetWindowWMInfo(globalRendering->window, &info))
			return;

		wnd = info.info.win.window;

		LONG_PTR cur_wndproc = GetWindowLongPtr(wnd, GWLP_WNDPROC);
		if (cur_wndproc != (LONG_PTR)SpringWndProc) {
			sdl_wndproc = GetWindowLongPtr(wnd, GWLP_WNDPROC);
			SetWindowLongPtr(wnd,GWLP_WNDPROC,(LONG_PTR)SpringWndProc);
		}
	}

	CWin32MouseInput()
	{
		inst = this;
		hCursor = nullptr;
		sdl_wndproc = 0;
		wnd = 0;
		InstallWndCallback();
	}
	~CWin32MouseInput()
	{
		// reinstall the SDL window proc
		SetWindowLongPtr(wnd, GWLP_WNDPROC, sdl_wndproc);
	}
};
CWin32MouseInput* CWin32MouseInput::inst = nullptr;
#endif

void IMouseInput::SetPos(int2 pos)
{
	if (!globalRendering->active)
		return;

	// calling SDL_WarpMouse at 300fps eats ~5% cpu usage, so only update when needed
	if (pos.x == mousepos.x && pos.y == mousepos.y)
		return;

	mousepos = pos;

	SDL_WarpMouseInWindow(globalRendering->window, pos.x, pos.y);

	// SDL_WarpMouse generates SDL_MOUSEMOTION events
	// in `middle click scrolling` those SDL generated ones would point into
	// the opposite direction the user moved the mouse, and so events would
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
#if defined(WIN32) && !defined(HEADLESS)
		mouseInput = new CWin32MouseInput;
#else
		mouseInput = new IMouseInput;
#endif
	}
	return mouseInput;
}

void IMouseInput::FreeInstance(IMouseInput* mouseInp) {
	spring::SafeDelete(mouseInp);
}
