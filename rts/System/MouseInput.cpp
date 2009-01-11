/*
	This workaround fixes the windows slow mouse movement problem (happens on fullscreen mode + pressing keys).
	The code hacks around the mouse input from DirectInput, which SDL uses in fullscreen mode.
	Instead it installs a window message proc and reads input from WM_MOUSEMOVE.
	On non-windows, the normal SDL events are used for mouse input

	new:
	It also workarounds a issue with SDL+windows and hardware cursors (->it has to block WM_SETCURSOR),
	so it is used now always even in window mode!
*/

#include "StdAfx.h"
#include "mmgr.h"

#include "Game/UI/MouseHandler.h"

#include <SDL_events.h>
#include <SDL_syswm.h>
#include "Rendering/GL/FBO.h"
#include "GlobalUnsynced.h"
#include "Platform/Win/win32.h"
#include "MouseInput.h"
#include "LogOutput.h"



IMouseInput* mouseInput = NULL;


IMouseInput::IMouseInput()
{}


IMouseInput::~IMouseInput()
{}


//////////////////////////////////////////////////////////////////////

#ifdef WIN32

class CWin32MouseInput : public IMouseInput
{
public:
	static CWin32MouseInput *inst;

	LONG_PTR sdl_wndproc;
	int2 mousepos;
	bool mousemoved;
	HWND wnd;
	HCURSOR hCursor;

	// SDL runs the window in a different thread, hence the indirectness of the mouse pos handling
	static LRESULT CALLBACK SpringWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (mouse) {
			switch (msg) {
				case WM_LBUTTONDOWN:
					mouse->MousePress((short)LOWORD(lparam), (short)HIWORD(lparam), 1);
					return TRUE;
				case WM_RBUTTONDOWN:
					mouse->MousePress((short)LOWORD(lparam), (short)HIWORD(lparam), 3);
					return TRUE;
				case WM_MBUTTONDOWN:
					mouse->MousePress((short)LOWORD(lparam), (short)HIWORD(lparam), 2);
					return TRUE;
				case WM_LBUTTONUP:
					mouse->MouseRelease((short)LOWORD(lparam), (short)HIWORD(lparam), 1);
					return TRUE;
				case WM_RBUTTONUP:
					mouse->MouseRelease((short)LOWORD(lparam), (short)HIWORD(lparam), 3);
					return TRUE;
				case WM_MBUTTONUP:
					mouse->MouseRelease((short)LOWORD(lparam), (short)HIWORD(lparam), 2);
					return TRUE;
				case WM_MOUSEWHEEL: {
					float delta = (((short)HIWORD(wparam))/120.0f);
					mouse->MouseWheel(delta);
					return TRUE;
				}
				case WM_XBUTTONDOWN:
					if ((short)LOWORD(wparam) & MK_XBUTTON1)
						mouse->MousePress((short)LOWORD(lparam), (short)HIWORD(lparam), 4);
					if ((short)LOWORD(wparam) & MK_XBUTTON2)
						mouse->MousePress((short)LOWORD(lparam), (short)HIWORD(lparam), 5);
					return TRUE;
				case WM_XBUTTONUP:
					if ((short)LOWORD(wparam) & MK_XBUTTON1)
						mouse->MouseRelease((short)LOWORD(lparam), (short)HIWORD(lparam), 4);
					if ((short)LOWORD(wparam) & MK_XBUTTON2)
						mouse->MouseRelease((short)LOWORD(lparam), (short)HIWORD(lparam), 5);
					return TRUE;
				}

		}

		switch (msg) {
			case WM_MOUSEMOVE:
				// cast to short to preserve sign
				inst->mousepos = int2((short)LOWORD(lparam),(short)HIWORD(lparam));
				inst->mousemoved = true;
				return FALSE;
			case WM_SETCURSOR:
				if (inst->hCursor!=NULL) {
					Uint16 hittest = LOWORD(lparam);
					if ( hittest == HTCLIENT ) {
						SetCursor(inst->hCursor);
						return TRUE;
					}
				}
				break;
			case WM_ACTIVATE:
				// FIXME: move to SpringApp somehow and use GLContext.h instead!
				if(fullscreen) {
					if (wparam == WA_INACTIVE) {
						FBO::GLContextLost();
					}else if (wparam == WA_ACTIVE) {
						FBO::GLContextReinit();
					}
				}
				break;
		}

		return CallWindowProc((WNDPROC)inst->sdl_wndproc, wnd, msg, wparam, lparam);
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

		mousemoved = false;
		sdl_wndproc = 0;

		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if(!SDL_GetWMInfo(&info))
			return;

		wnd = info.window;

		InstallWndCallback();
	}
	~CWin32MouseInput()
	{
		// reinstall the SDL window proc
		SetWindowLongPtr(wnd, GWLP_WNDPROC, sdl_wndproc);
	}

	int2 GetPos()
	{
		return mousepos;
	}
	void SetPos(int2 pos)
	{
		mousepos = pos;
		if (fullscreen)
			SetCursorPos(pos.x, pos.y);
		else
			SDL_WarpMouse(pos.x, pos.y);
	}

	void Update()
	{
		if (mousemoved) {
			if (mouse) mouse->MouseMove(mousepos.x, mousepos.y);
			mousemoved = false;
		}
	}

	void HandleSDLMouseEvent (SDL_Event& event)
	{
		/*if (!mouse)
			return;

		switch (event.type) {
		case SDL_SYSWMEVENT:{
			SDL_SysWMmsg *msg = event.syswm.msg;
			if (msg->msg == 0x020B) { // WM_XBUTTONDOWN, beats me why it isn't defined by default
				if (msg->wParam & 0x20) // MK_XBUTTON1
					mouse->MousePress (mousepos.x, mousepos.y, 4);
				if (msg->wParam & 0x40) // MK_XBUTTON2
					mouse->MousePress (mousepos.x, mousepos.y, 5);
			}

			break;}
		case SDL_MOUSEMOTION: // the normal SDL method works fine in windowed mode
			if(!fullscreen) {
				mousepos = int2(event.motion.x, event.motion.y);
				mouse->MouseMove(mousepos.x, mousepos.y);
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_WHEELUP)
				mouse->MouseWheel(1.0f);
			else if (event.button.button == SDL_BUTTON_WHEELDOWN)
				mouse->MouseWheel(-1.0f);
			else
				mouse->MousePress(mousepos.x, mousepos.y,event.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			mouse->MouseRelease(mousepos.x, mousepos.y,event.button.button);
			break;
		}*/
	}
};
CWin32MouseInput* CWin32MouseInput::inst = 0;
#endif

class CDefaultMouseInput : public IMouseInput
{
public:
	int2 mousepos;

	int2 GetPos()
	{
		return mousepos;
	}

	void SetPos(int2 pos)
	{
		mousepos = pos;
		SDL_WarpMouse(pos.x, pos.y);
	}

	void HandleSDLMouseEvent (SDL_Event& event)
	{
		switch (event.type) {
			case SDL_MOUSEMOTION: {
				mousepos = int2(event.motion.x, event.motion.y);
				if (mouse) {
					mouse->MouseMove(mousepos.x, mousepos.y);
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
	}
};


IMouseInput *IMouseInput::Get ()
{
	if (!mouseInput) {
#ifdef WIN32
		mouseInput = new CWin32MouseInput;
#else
		mouseInput = new CDefaultMouseInput;
#endif
	}
	return mouseInput;
}

