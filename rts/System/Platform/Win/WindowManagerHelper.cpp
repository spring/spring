/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/WindowManagerHelper.h"
#include "Rendering/GlobalRendering.h"
#include <SDL_syswm.h>
#include <windows.h>


namespace WindowManagerHelper {

void BlockCompositing(SDL_Window* window)
{
#ifndef HEADLESS
	// only available in WinVista+
	HMODULE dwmapiDllHandle = LoadLibrary("dwmapi.dll");

	if (dwmapiDllHandle != nullptr) {
		typedef HRESULT (*DwmEnableCompositionFunction)(UINT uCompositionAction);
		auto DwmEnableComposition = (DwmEnableCompositionFunction) ::GetProcAddress(dwmapiDllHandle, "DwmEnableComposition");
		if (DwmEnableComposition != nullptr) {
			static const unsigned int DWM_EC_DISABLECOMPOSITION = 0U;
			DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
		}

		FreeLibrary(dwmapiDllHandle);
	}
#endif
}


int GetWindowState(SDL_Window* window)
{
	int state = 0;
#ifndef HEADLESS
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	struct SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(globalRendering->window, &info);

	if (GetWindowPlacement(info.info.win.window, &wp)) {
		if (wp.showCmd == SW_SHOWMAXIMIZED)
			state = SDL_WINDOW_MAXIMIZED;
		if (wp.showCmd == SW_SHOWMINIMIZED)
			state = SDL_WINDOW_MINIMIZED;
	}
#endif
	return state;
}

}; // namespace WindowManagerHelper
