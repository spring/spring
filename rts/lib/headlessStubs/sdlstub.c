/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * documentation for the functions in this file can be found at:
 * http://www.libsdl.org/docs/html/
 */

#include <SDL.h>


#ifdef __cplusplus
extern "C" {
#endif


static struct SDL_Surface stubSurface;
static struct SDL_RWops stubRWops;
static Uint8 stubKeyState[1];
static SDL_version stubVersion;
static Uint32 stubSubSystemsInit = 0;


extern DECLSPEC void SDLCALL SDL_free(void* p) {
	return;
}

extern DECLSPEC int SDLCALL SDL_Init(Uint32 flags) {

	stubSurface.w = 512;
	stubSurface.h = 512;
	stubSubSystemsInit = SDL_INIT_EVERYTHING;

	return 0;
}

extern DECLSPEC Uint32 SDLCALL SDL_WasInit(Uint32 flags) {
	return (stubSubSystemsInit & flags);
}

extern DECLSPEC int SDLCALL SDL_InitSubSystem(Uint32 flags) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_QuitSubSystem(Uint32 flags) {
}

extern DECLSPEC const char* SDLCALL SDL_GetError() {
	return "using the SDL stub library";
}

extern DECLSPEC int SDLCALL SDL_GL_SetAttribute(SDL_GLattr attr, int value) {
	return 0;
}

extern DECLSPEC SDL_Window* SDLCALL SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags) {
	static int foo;
	return (SDL_Window*)(&foo);
}

extern DECLSPEC void SDLCALL SDL_DestroyWindow(SDL_Window * window) {}
extern DECLSPEC void SDLCALL SDL_MinimizeWindow(SDL_Window * window) {}
extern DECLSPEC void SDLCALL SDL_MaximizeWindow(SDL_Window * window) {}
extern DECLSPEC void SDLCALL SDL_RestoreWindow(SDL_Window * window) {}

extern DECLSPEC int SDLCALL SDL_GL_MakeCurrent(SDL_Window * window, SDL_GLContext context){
	return 0;
}

extern DECLSPEC const char *SDLCALL SDL_GetWindowTitle(SDL_Window * window) {
	return "";
}

extern DECLSPEC Uint32 SDLCALL SDL_GetWindowPixelFormat(SDL_Window* window) { return 0; }
extern DECLSPEC const char* SDLCALL SDL_GetPixelFormatName(Uint32 format) { return ""; }

extern DECLSPEC struct SDL_RWops* SDLCALL SDL_RWFromFile(const char* file, const char* mode) {
	return &stubRWops;
}

extern DECLSPEC SDL_Surface* SDLCALL SDL_LoadBMP_RW(SDL_RWops* src, int freesrc) {
	return &stubSurface;
}

extern DECLSPEC void SDLCALL SDL_Quit() {
}

extern DECLSPEC SDL_Surface* SDLCALL SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
	return NULL;
}

extern DECLSPEC SDL_Surface* SDLCALL SDL_CreateRGBSurfaceFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
	return NULL;
}

extern DECLSPEC void SDLCALL SDL_FreeSurface(SDL_Surface* surface) {
}

extern DECLSPEC void SDLCALL SDL_GL_SwapWindow(SDL_Window* window) {
}

extern DECLSPEC int SDLCALL SDL_SetRelativeMouseMode(SDL_bool enabled) { return SDL_FALSE; }
extern DECLSPEC void SDLCALL SDL_WarpMouseInWindow(SDL_Window* window, int x, int y) {
}

extern DECLSPEC int SDLCALL SDL_WM_IconifyWindow() {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_HideWindow(SDL_Window* window) {
}

extern DECLSPEC int SDLCALL SDL_SetWindowFullscreen(SDL_Window* window, Uint32 flags) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_SetWindowBordered(SDL_Window* window, SDL_bool bordered) {
}

extern DECLSPEC int SDLCALL SDL_EnableKeyRepeat(int i, int j) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_SetModState(SDL_Keymod modstate) {
}

extern DECLSPEC int SDLCALL SDL_PollEvent(SDL_Event* event) { return 0; }
extern DECLSPEC int SDLCALL SDL_PushEvent(SDL_Event* event) { return 0; }

extern DECLSPEC void SDLCALL SDL_FlushEvent(Uint32 type) {}
extern DECLSPEC void SDLCALL SDL_PumpEvents() {}

extern DECLSPEC void SDLCALL SDL_SetWindowTitle(SDL_Window* window, const char* title) {
}

extern DECLSPEC void SDLCALL SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon) {
}

extern DECLSPEC void SDLCALL SDL_SetWindowMinimumSize(SDL_Window* window, int min_w, int min_h) {
}

extern DECLSPEC int SDLCALL SDL_GL_GetAttribute(SDL_GLattr attr, int* value) {
	*value = 0;
	return 0;
}


extern DECLSPEC SDL_GLContext SDLCALL SDL_GL_CreateContext(SDL_Window* window) {
	static int foo;
	return &foo;
}

extern DECLSPEC void SDLCALL SDL_GL_DeleteContext(SDL_GLContext context) {
}


extern DECLSPEC void SDLCALL SDL_SetWindowGrab(SDL_Window* window, SDL_bool grabbed) {
}

extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowGrab(SDL_Window* window) {
	return 0;
}

extern DECLSPEC Uint32 SDLCALL SDL_GetWindowFlags(SDL_Window* window) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_EnableScreenSaver() {}
extern DECLSPEC void SDLCALL SDL_DisableScreenSaver() {}

extern DECLSPEC char* SDLCALL SDL_GetClipboardText() {
	return "";
}

extern DECLSPEC int SDLCALL SDL_SetClipboardText(const char* text) {
	return -1;
}

extern DECLSPEC const Uint8 *SDLCALL SDL_GetKeyboardState(int* numkeys) {
	*numkeys = 0;
	return stubKeyState;
}

extern DECLSPEC SDL_Keymod SDLCALL SDL_GetModState() {
	return 0;
}

extern DECLSPEC SDL_Keycode SDLCALL SDL_GetKeyFromScancode(SDL_Scancode scancode) {
	return 0;
}

extern DECLSPEC SDL_Scancode SDLCALL SDL_GetScancodeFromKey(SDL_Keycode key) {
	return 0;
}

extern DECLSPEC const SDL_version* SDLCALL SDL_Linked_Version() {
	return &stubVersion;
}

extern DECLSPEC void SDLCALL SDL_GetVersion(SDL_version* ver) {
	*ver = stubVersion;
}

extern DECLSPEC int SDLCALL SDL_ShowCursor(int toggle) {
	return 0;
}

extern DECLSPEC Uint32 SDLCALL SDL_GetMouseState(int* x, int* y) {
	return 0;
}

extern DECLSPEC int SDLCALL SDL_NumJoysticks() {
	return 0;
}

extern DECLSPEC const char* SDLCALL SDL_JoystickName(SDL_Joystick* device_index) {
	return "";
}

extern DECLSPEC SDL_Joystick* SDLCALL SDL_JoystickOpen(int device_index) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_JoystickClose(SDL_Joystick* joystick) {
}

extern DECLSPEC int SDLCALL SDL_GetNumDisplayModes(int displayIndex) {
	return 0;
}

extern DECLSPEC int SDLCALL SDL_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode* mode) {
	mode->format = SDL_PIXELFORMAT_RGB24;
	mode->w = 640;
	mode->h = 480;
	mode->refresh_rate = 100;
	mode->driverdata = NULL;
	return 0;
}

extern DECLSPEC int SDLCALL SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode* mode) {
	return SDL_GetDesktopDisplayMode(0, mode);
}

extern DECLSPEC int SDLCALL SDL_GetWindowDisplayMode(SDL_Window* window, SDL_DisplayMode* mode) {
	return SDL_GetDesktopDisplayMode(0, mode);
}

extern DECLSPEC int SDLCALL SDL_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode* mode) {
	return SDL_GetDesktopDisplayMode(0, mode);
}


extern DECLSPEC int SDLCALL SDL_PeepEvents(SDL_Event* events, int numevents, SDL_eventaction action, Uint32 minType, Uint32 maxType) {
	return 0;
}

extern DECLSPEC Uint8 SDLCALL SDL_GetAppState() {
	return 0;
}

extern DECLSPEC int SDL_GetNumVideoDisplays(void) {
	return 0;
}
extern DECLSPEC int SDL_GetDisplayBounds(int displayIndex, SDL_Rect* rect) {
	if (rect == 0) return -1;
	rect->w = 640;
	rect->h = 480;
	rect->x = 0;
	rect->y = 0;
	return 0;
}

extern DECLSPEC int SDL_GL_GetSwapInterval() {
	return 0;
}

extern DECLSPEC void SDL_SetWindowPosition(SDL_Window * window, int x, int y) {
}

extern DECLSPEC void SDL_SetWindowSize(SDL_Window * window, int w, int h) {
}


extern DECLSPEC SDL_PowerState SDL_GetPowerInfo(int *secs, int *pct) {
	return SDL_POWERSTATE_UNKNOWN;
}

extern DECLSPEC SDL_bool SDL_SetHint(const char* name, const char* value) {
	return SDL_TRUE;
}

extern DECLSPEC void SDLCALL SDL_SetTextInputRect(SDL_Rect *rect) {
}

extern DECLSPEC void SDLCALL SDL_StartTextInput(void) {
}

extern DECLSPEC void SDLCALL SDL_StopTextInput(void) {
}

#ifdef __cplusplus
} // extern "C"
#endif
