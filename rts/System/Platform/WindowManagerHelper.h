/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WINDOW_MANAGER_HELPER_H
#define WINDOW_MANAGER_HELPER_H

#include <string>

class CBitmap;
struct SDL_Window;

namespace WindowManagerHelper {
	/**
	 * Sets the window-manager icon for the running process.
	 * It will be displayed in the OS task-bar, for example.
	 * @param icon This should be either a 24bit RGB or 32bit RGBA 32x32 image.
	 *             Use NULL to remove the icon and cleanup.
	 *             After the method returns, the bitmap may be deleted.
	 * @see SDL_WM_SetIcon()
	 * Note: Must be called before the first call to SDL_SetVideoMode.
	 */
	void SetIcon(const CBitmap* icon);
	void FreeIcon();

	/**
	 * Sets the window-manager caption/title for the running process.
	 * @param title will be displayed in the window title (in windowed mode)
	 *   and in the OS task-bar
	 *   example: "MyGame"
	 * @see #SetCaption(const std::string&, const std::string&)
	 */
	void SetCaption(const std::string& title);


	/**
	 * @brief disables desktop compositing (kwin, aero, compiz, ...) to fix tearing & vsync problems
	 */
	void BlockCompositing(SDL_Window* window);

	/**
	 * @brief returns the window-state of the given window in SDL_GetWindowFlags() format
	 */
	int GetWindowState(SDL_Window* window);
};

#endif // WINDOW_MANAGER_HELPER_H
