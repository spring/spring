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

#include "StdAfx.h"
#include "mmgr.h"

#include "Game/UI/MouseHandler.h"

#include <boost/bind.hpp>
#include <SDL_events.h>
#include <SDL_syswm.h>
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "GlobalUnsynced.h"
#ifdef _WIN32
	#include "Platform/Win/win32.h"
	#include "Platform/Win/wsdl.h"
#endif
#include "MouseInput.h"
#include "InputHandler.h"
#include "LogOutput.h"



IMouseInput* mouseInput = NULL;


IMouseInput::IMouseInput()
{
	inputCon = input.AddHandler(boost::bind(&IMouseInput::HandleSDLMouseEvent, this, _1));
}


IMouseInput::~IMouseInput()
{}

bool IMouseInput::HandleSDLMouseEvent (const SDL_Event& event)
{
	switch (event.type) {
		case SDL_MOUSEMOTION: {
			mousepos = int2(event.motion.x, event.motion.y);
			if (mouse) {
				mouse->MouseMove(mousepos.x, mousepos.y, event.motion.xrel, event.motion.yrel);
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN: {
			mousepos = int2(event.button.x, event.button.y);
			if (mouse) {
				if (event.button.button == SDL_BUTTON_WHEELUP) {
					mouse->MouseWheel(1.0f);
				} else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
					mouse->MouseWheel(-1.0f);
				} else {
					mouse->MousePress(event.button.x, event.button.y, event.button.button);
				}
			}
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			mousepos = int2(event.button.x, event.button.y);
			if (mouse) {
				mouse->MouseRelease(event.button.x, event.button.y, event.button.button);
			}
			break;
		}
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
		if(!SDL_GetWMInfo(&info))
			return;

		wnd = info.window;
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
	mousepos = pos;
#ifdef WIN32
	wsdl::SDL_WarpMouse(pos.x, pos.y);
#else
	SDL_WarpMouse(pos.x, pos.y);
#endif

	//! delete all SDL_MOUSEMOTION in the queue to avoid fast jumps when entering `middle click scrolling`
	static SDL_Event events[100];
	SDL_PeepEvents(&events[0], 100, SDL_GETEVENT, SDL_MOUSEMOTIONMASK);
}

IMouseInput *IMouseInput::Get()
{
	if (!mouseInput) {
#ifdef WIN32
		mouseInput = new CWin32MouseInput;
#else
		mouseInput = new IMouseInput;
#endif
	}
	return mouseInput;
}

