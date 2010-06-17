/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * documentation for the functions in this file can be found at:
 * http://www.libsdl.org/docs/html/
 */

#include "SDL.h"


#ifdef __cplusplus
extern "C" {
#endif

// @see sdlstub_cppbit.cpp
int stub_sdl_getSystemMilliSeconds();
void stub_sdl_sleepMilliSeconds(int milliSeconds);


static int startSystemMilliSeconds;
static struct SDL_Surface stubSurface;
static struct SDL_RWops stubRWops;
static Uint8 stubKeyState[0];
static SDL_version stubVersion;


extern DECLSPEC int SDLCALL SDL_Init(Uint32 flags) {

	startSystemMilliSeconds = stub_sdl_getSystemMilliSeconds();

	stubSurface.w = 512;
	stubSurface.h = 512;

	return 0;
}

extern DECLSPEC int SDLCALL SDL_InitSubSystem(Uint32 flags) {
	return 0;
}

extern DECLSPEC char* SDLCALL SDL_GetError() {
	return "using the SDL stub library";
}

extern DECLSPEC int SDLCALL SDL_GL_SetAttribute(SDL_GLattr attr, int value) {
	return 0;
}

extern DECLSPEC SDL_Surface* SDLCALL SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags) {
	return &stubSurface;
}

extern DECLSPEC struct SDL_RWops* SDLCALL SDL_RWFromFile(const char* file, const char* mode) {
	return &stubRWops;
}

extern DECLSPEC SDL_Surface* SDLCALL SDL_LoadBMP_RW(SDL_RWops* src, int freesrc) {
	return &stubSurface;
}

extern DECLSPEC void SDLCALL SDL_Quit() {
}

extern DECLSPEC void SDLCALL SDL_FreeSurface(SDL_Surface* surface) {
}

extern DECLSPEC void SDLCALL SDL_GL_SwapBuffers() {
}

extern DECLSPEC void SDLCALL SDL_Delay(Uint32 ms) {
	stub_sdl_sleepMilliSeconds(ms);
}

extern DECLSPEC Uint32 SDLCALL SDL_GetTicks() {
	return stub_sdl_getSystemMilliSeconds() - startSystemMilliSeconds;
}

extern DECLSPEC void SDLCALL SDL_WarpMouse(Uint16 x, Uint16 y) {
}

extern DECLSPEC int SDLCALL SDL_WM_IconifyWindow() {
}

extern DECLSPEC int SDLCALL SDL_EnableUNICODE(int i) {
}

extern DECLSPEC int SDLCALL SDL_EnableKeyRepeat(int i, int j) {
}

extern DECLSPEC void SDLCALL SDL_SetModState(SDLMod i) {
}

extern DECLSPEC int SDLCALL SDL_PollEvent(SDL_Event* event) {
	return 0;
}

extern DECLSPEC int SDLCALL SDL_PushEvent(SDL_Event* event) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_PumpEvents() {
}

extern DECLSPEC void SDLCALL SDL_WM_SetIcon(SDL_Surface* icon, Uint8* mask) {
}

extern DECLSPEC void SDLCALL SDL_WM_SetCaption(const char* title, const char* icon) {
}

extern DECLSPEC int SDLCALL SDL_GL_GetAttribute(SDL_GLattr attr, int* value) {
	*value = 0;
}

extern DECLSPEC SDL_GrabMode SDLCALL SDL_WM_GrabInput(SDL_GrabMode mode) {
}

extern DECLSPEC int SDLCALL SDL_GetWMInfo(void* infostruct) {

	// TODO: probably needs to populate infostruct with something reasonable...

	// I _think_ this triggers SpringApp.cpp to just use the screen geometry
	return 0;
}

extern DECLSPEC Uint8* SDLCALL SDL_GetKeyState(int* numkeys) {
	*numkeys = 0;
	return stubKeyState;
}

extern DECLSPEC SDLMod SDLCALL SDL_GetModState() {
	return 0;
}

extern DECLSPEC const SDL_version* SDLCALL SDL_Linked_Version() {
	return &stubVersion;
}

extern DECLSPEC int SDLCALL SDL_ShowCursor(int toggle) {
}

extern DECLSPEC Uint8 SDLCALL SDL_GetMouseState(int* x, int* y) {
	return 0;
}

extern DECLSPEC int SDLCALL SDL_SetGamma(float red, float green, float blue) {
	return 0;
}

extern DECLSPEC int SDLCALL SDL_NumJoysticks() {
	return 0;
}

extern DECLSPEC const char* SDLCALL SDL_JoystickName(int device_index) {
	return "";
}

extern DECLSPEC SDL_Joystick* SDLCALL SDL_JoystickOpen(int device_index) {
	return 0;
}

extern DECLSPEC void SDLCALL SDL_JoystickClose(SDL_Joystick* joystick) {
}

extern DECLSPEC SDL_Rect** SDLCALL SDL_ListModes(SDL_PixelFormat* format, Uint32 flags) {
	return NULL;
}

extern DECLSPEC int SDLCALL SDL_PeepEvents(SDL_Event* events, int numevents, SDL_eventaction action, Uint32 mask) {
	return 0;
}

extern DECLSPEC Uint8 SDLCALL SDL_GetAppState() {
	return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

