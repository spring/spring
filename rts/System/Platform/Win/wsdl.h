/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * emulate SDL on Windows.
 */

#ifndef INCLUDED_WSDL
#define INCLUDED_WSDL

#include <windows.h>

#include "SDL_events.h"

namespace wsdl
{
void ResetMouseButtons();
void Init(HWND hWnd); // set on startup
void SDL_WarpMouse(int x, int y);
LRESULT OnActivate(HWND hWnd, UINT state, HWND hWndActDeact, BOOL fMinimized);
LRESULT OnMouseMotion(HWND hWnd, int x, int y, UINT flags);
LRESULT OnMouseButton(HWND hWnd, UINT uMsg, int client_x, int client_y, UINT flags);
LRESULT OnMouseWheel(HWND hWnd, int screen_x, int screen_y, int zDelta, UINT fwKeys);
}

#endif // #ifndef INCLUDED_WSDL
