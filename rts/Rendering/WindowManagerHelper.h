/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WINDOW_MANAGER_HELPER_H
#define WINDOW_MANAGER_HELPER_H

#include <string>

class CBitmap;
struct SDL_Surface;

struct WindowManagerHelper {
public:
	/**
	 * Sets the window-manager icon for the running process.
	 * It will be displayed in the OS task-bar, for example.
	 * @param icon This should be either a 24bit RGB or 32bit RGBA 32x32 image.
	 *             Use NULL to remove the icon and cleanup.
	 *             After the method returns, the bitmap may be deleted.
	 * @see SDL_WM_SetIcon()
	 * Note: Must be called before the first call to SDL_SetVideoMode.
	 */
	static void SetIcon(const CBitmap* icon);

	/**
	 * Sets the window-manager caption/title for the running process.
	 * It will be displayed in the OS task-bar, for example.
	 * @param caption example: "Spring 0.82.6.1 - MyMod 1.0"
	 * @see SDL_WM_SetCaption()
	 */
	static void SetCaption(const std::string& caption);

private:
	static SDL_Surface* currentIcon;
};

#endif // WINDOW_MANAGER_HELPER_H
