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
	static void FreeIcon();

	/**
	 * Sets the window-manager captions/titles for the running process.
	 * @param title will be displayed in the window title (in windowed mode)
	 *   example: "MyGame 1.0 - Chicken Mode (Spring 0.83.0.1)"
	 * @param titleShort will be displayed in the OS task-bar
	 *   example: "MyGame"
	 *   This may only ever be used under X11, but not QT(KDE) or Windows.
	 * @see SDL_WM_SetCaption()
	 */
	static void SetCaption(const std::string& title,
			const std::string& titleShort);
	/**
	 * Sets the window-manager caption/title for the running process.
	 * @param title will be displayed in the window title (in windowed mode)
	 *   and in the OS task-bar
	 *   example: "MyGame"
	 * @see #SetCaption(const std::string&, const std::string&)
	 */
	static void SetCaption(const std::string& title);

private:
	static SDL_Surface* currentIcon;
};

#endif // WINDOW_MANAGER_HELPER_H
