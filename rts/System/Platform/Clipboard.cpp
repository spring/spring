/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Clipboard.h"
#include <SDL_clipboard.h>


std::string CClipboard::GetContents() const
{
	return SDL_GetClipboardText();
};
