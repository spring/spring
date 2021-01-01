/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Clipboard.h"
#include <SDL_clipboard.h>


std::string CClipboard::GetContents() const
{
	char* text = SDL_GetClipboardText();

	if (text == nullptr)
		return "";

	std::string s = text;
	SDL_free(text);
	return s;
}
