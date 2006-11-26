
#include "StdAfx.h"
#include "Platform/Win/win32.h"
#include "MouseInput.h"
#include "Game/UI/MouseHandler.h"
#include "Game/CameraController.h"
#include "Platform/ConfigHandler.h"

#include <SDL_events.h>
#include <SDL_syswm.h>

IMouseInput *mouseInput = 0;

IMouseInput::IMouseInput()
{
	scrollWheelSpeed = configHandler.GetInt("ScrollWheelSpeed",25);
	scrollWheelSpeed = max(-255, min(255, scrollWheelSpeed));
}

IMouseInput::~IMouseInput() {
}

//////////////////////////////////////////////////////////////////////

#ifdef WIN32
class CWin32MouseInput : public IMouseInput
{
public:
	static CWin32MouseInput *inst;

	/* This hack fixes the windows slow mouse movement problem (happens on fullscreen mode + pressing keys).
	This code hacks around the mouse input from DirectInput, which SDL uses in fullscreen mode.
	Instead it installs a window message proc and reads input from WM_MOUSEMOVE */

	LONG_PTR sdl_wndproc;
	int2 mousepos;
	bool mousemoved;
	HWND wnd;

	// SDL runs the window in a different thread, hence the indirectness of the mouse pos handling
	static LRESULT CALLBACK SpringWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg==WM_MOUSEMOVE) {
			inst->mousepos = int2(LOWORD(lparam),HIWORD(lparam));
			inst->mousemoved = true;
			return FALSE;
		}
		return CallWindowProc((WNDPROC)inst->sdl_wndproc, wnd, msg, wparam, lparam);
	}

	void InstallWndCallback()
	{
		sdl_wndproc = GetWindowLongPtr(wnd, GWLP_WNDPROC);
		SetWindowLongPtr(wnd,GWLP_WNDPROC,(LONG_PTR)SpringWndProc);
	}
	
	CWin32MouseInput()
	{
		inst = this;

		mousemoved = false;
		sdl_wndproc = 0;

		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if(!SDL_GetWMInfo(&info)) 
			return;

		wnd = info.window;

		// In windowed mode, SDL uses straight Win32 API to handle mouse movement, which works ok.
		if (fullscreen)
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
		if (fullscreen)
			SetCursorPos (pos.x, pos.y);
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
		if (!mouse)
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
				mouse->currentCamController->MouseWheelMove(scrollWheelSpeed);
			else if (event.button.button == SDL_BUTTON_WHEELDOWN)
				mouse->currentCamController->MouseWheelMove(-scrollWheelSpeed);
			else
				mouse->MousePress(mousepos.x, mousepos.y,event.button.button);
			break;
		case SDL_MOUSEBUTTONUP:
			mouse->MouseRelease(mousepos.x, mousepos.y,event.button.button);
			break;
		}
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
		SDL_WarpMouse (pos.x,pos.y);
	}

	void HandleSDLMouseEvent (SDL_Event& event)
	{
		switch(event.type) {
		case SDL_MOUSEMOTION:
			mousepos = int2(event.motion.x,event.motion.y);
			if(mouse)
				mouse->MouseMove(mousepos.x,mousepos.y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			mousepos = int2(event.button.x,event.button.y);
			if (mouse) {
				if (event.button.button == SDL_BUTTON_WHEELUP)
					mouse->currentCamController->MouseWheelMove(scrollWheelSpeed);
				else if (event.button.button == SDL_BUTTON_WHEELDOWN)
					mouse->currentCamController->MouseWheelMove(-scrollWheelSpeed);
				else
					mouse->MousePress(event.button.x,event.button.y,event.button.button);
			}
			break;
		case SDL_MOUSEBUTTONUP:
			mousepos = int2(event.button.x,event.button.y);
			if (mouse)
				mouse->MouseRelease(event.button.x,event.button.y,event.button.button);
			break;
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

