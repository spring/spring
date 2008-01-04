/* Author: Tobi Vollebregt */

#include "StdAfx.h"
#include "Clipboard.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <SDL_syswm.h>
#endif

std::string CClipboard::GetContents() const
{
	std::string contents;
#ifdef _WIN32
	OpenClipboard(NULL);
	const void* p = GetClipboardData(CF_TEXT);
	if (p != NULL) {
		contents = (char*)p;
	}
	CloseClipboard();
#elif defined(__APPLE__)
	// Nothing for now
#else
	// only works with the cut-buffer method (xterm)
	// (and not with the more recent selections method)
	SDL_SysWMinfo sdlinfo;
	SDL_VERSION(&sdlinfo.version);
	if (SDL_GetWMInfo(&sdlinfo)) {
		sdlinfo.info.x11.lock_func();
		Display* display = sdlinfo.info.x11.display;
		int count = 0;
		char* msg = XFetchBytes(display, &count);
		if ((msg != NULL) && (count > 0)) {
			contents.append((char*)msg, count);
		}
		XFree(msg);
		sdlinfo.info.x11.unlock_func();
	}
#endif
	return contents;
};
