/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * documentation for the functions in this file can be found at:
 * http://www.libsdl.org/docs/html/
 */

#include "SDL.h"

// @see sdlstub_cppbit.cpp
int stub_sdl_getSystemMilliSeconds();
void stub_sdl_sleepMilliSeconds(int milliSeconds);


static int startSystemMilliSeconds;
static struct SDL_Surface stubSurface;
static struct SDL_RWops stubRWops;
static Uint8 stubKeyState[0];
static SDL_version stubVersion;


int SDL_Init(Uint32 flags) {

	startSystemMilliSeconds = stub_sdl_getSystemMilliSeconds();

	stubSurface.w = 512;
	stubSurface.h = 512;

	return 0;
}

int SDL_InitSubSystem(Uint32 flags) {
	return 0;
}

char* SDL_GetError() {
	return "using the SDL stub library";
}

int SDL_GL_SetAttribute(SDL_GLattr attr, int value) {
	return 0;
}

SDL_Surface* SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags) {
	return &stubSurface;
}

struct SDL_RWops* SDL_RWFromFile(const char* file, const char* mode) {
	return &stubRWops;
}

SDL_Surface* SDL_LoadBMP_RW(SDL_RWops* src, int freesrc) {
	return &stubSurface;
}

void SDL_Quit() {
}

void SDL_FreeSurface(SDL_Surface* surface) {
}

void SDL_GL_SwapBuffers() {
}

void SDL_Delay(Uint32 ms) {
	stub_sdl_sleepMilliSeconds(ms);
}

Uint32 SDL_GetTicks() {
	return stub_sdl_getSystemMilliSeconds() - startSystemMilliSeconds;
}

void SDL_WarpMouse(Uint16 x, Uint16 y) {
   printf("SDL_WarpMouse\n");
}

int SDL_WM_IconifyWindow() {
   printf("IconifyWindow\n");
}

int SDL_EnableUNICODE(int i) {
   printf("EnableUNICODE\n");
}

int SDL_EnableKeyRepeat(int i, int j) {
   printf("EnableKeyRepeat\n");
}

void SDL_SetModState(SDLMod i) {
   printf("SetModState\n");
}

int SDL_PollEvent(SDL_Event* event) {
 //  printf("PollEvent\n");
	return 0;
}

int SDL_PushEvent(SDL_Event* event) {
 //  printf("PushEvent\n");
	return 0;
}

void SDL_PumpEvents() {
}

void SDL_WM_SetIcon(SDL_Surface* icon, Uint8* mask) {
   printf("SetIcon\n");
}

void SDL_WM_SetCaption(const char* title, const char* icon) {
   printf("SetCaption\n");
}

int SDL_GL_GetAttribute(SDL_GLattr attr, int* value) {
   printf("GetAttribute\n");
	*value = 0;
}

SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode) {
   printf("GrabInput\n");
}

int SDL_GetWMInfo(void* infostruct) {

	// TODO: probably needs to populate infostruct with something reasonable...

	// I _think_ this triggers SpringApp.cpp to just use the screen geometry
	return 0;
}

Uint8* SDL_GetKeyState(int* numkeys) {
   printf("GetKeyState\n");
	*numkeys = 0;
	return stubKeyState;
}

SDLMod SDL_GetModState() {
   printf("GetModState\n");
	return 0;
}

const SDL_version*  SDL_Linked_Version() {
   printf("Linked_Version\n");
	return &stubVersion;
}

int SDL_ShowCursor(int toggle) {
   printf("ShowCursor\n");
}

Uint8 SDL_GetMouseState(int* x, int* y) {
   printf("GetMouseState\n");
	return 0;
}

int SDL_SetGamma(float red, float green, float blue) {
   printf("SetGamma\n");
	return 0;
}

int SDL_NumJoysticks() {
	return 0;
}

const char* SDL_JoystickName(int device_index) {
	return "";
}

SDL_Joystick* SDL_JoystickOpen(int device_index) {
	return 0;
}

void SDL_JoystickClose(SDL_Joystick* joystick) {
}

SDL_Rect** SDL_ListModes(SDL_PixelFormat* format, Uint32 flags) {
	return NULL;
}

int SDL_PeepEvents(SDL_Event* events, int numevents, SDL_eventaction action, Uint32 mask) {
	return 0;
}

Uint8 SDL_GetAppState() {
	return 0;
}
