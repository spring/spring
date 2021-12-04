/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <string>

class CClipboard
{
public:
	CClipboard() {}

	std::string GetContents() const;
};

#endif // !CLIPBOARD_H
