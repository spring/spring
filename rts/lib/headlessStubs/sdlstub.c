/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * documentation for the functions in this file can be found at:
 * http://www.libsdl.org/docs/html/
 */

#include "SDL.h"

int startsystemmilliseconds;

int getsystemmilliseconds();
void sleepmilliseconds( int milliseconds );

int SDL_Init(Uint32 flagss){
   startsystemmilliseconds = getsystemmilliseconds();
   return 0;
}

int SDL_InitSubSystem(Uint32 flags) {
   return 0;
}

char *SDL_GetError(){
   return "";
}

int SDL_GL_SetAttribute(SDL_GLattr attr, int value ){
   return 0;
}

struct SDL_Surface stubSurface;

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags){
   stubSurface.w = 512; stubSurface.h = 512;
   return &stubSurface;
}

struct SDL_RWops sdl_rwops;

struct SDL_RWops *SDL_RWFromFile(const char *file, const char *mode ) {
   return &sdl_rwops;
}

SDL_Surface * SDL_LoadBMP_RW(SDL_RWops *src, int freesrc) {
   stubSurface.w = 512; stubSurface.h = 512;
   return &stubSurface;
}

void SDL_Quit() {
}

void SDL_FreeSurface( SDL_Surface *surface ) {
}

void SDL_GL_SwapBuffers() {
}

void SDL_Delay(Uint32 ms ) {
   sleepmilliseconds(ms);
}

Uint32 SDL_GetTicks(){
   return getsystemmilliseconds() - startsystemmilliseconds;
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

int SDL_PollEvent(SDL_Event *event) {
 //  printf("PollEvent\n");
   return 0;
}

int SDL_PushEvent(SDL_Event *event) {
 //  printf("PushEvent\n");
   return 0;
}

void SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask) {
   printf("SetIcon\n");
}

void SDL_WM_SetCaption(const char *title, const char *icon) {
   printf("SetCaption\n");
}

int SDL_GL_GetAttribute(SDL_GLattr attr, int* value) {
   printf("GetAttribute\n");
   *value = 0;
}

SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode) {
   printf("GrabInput\n");
}

// this probably needs to populate infostruct with something reasonable...
int SDL_GetWMInfo(void * infostruct) {
   printf("GetWMInfo\n");
   return 0; // I *think* this triggers SpringApp.cpp to just use the screen geometry
     // we probably need to stub out X11 and Xcursors too...
}

static Uint8 stubvalue[0];
Uint8 * SDL_GetKeyState(int *numkeys) {
   printf("GetKeyState\n");
   *numkeys = 0;
   return stubvalue;
}

SDLMod SDL_GetModState() {
   printf("GetModState\n");
   return 0;
}

SDL_version stubversion;
const SDL_version *  SDL_Linked_Version() {
   printf("Linked_Version\n");
   return &stubversion;
}

int SDL_ShowCursor(int toggle) {
   printf("ShowCursor\n");
}

Uint8 SDL_GetMouseState(int *x, int *y) {
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

const char *SDL_JoystickName(int device_index) {
   return "";
}

SDL_Joystick *SDL_JoystickOpen(int device_index) {
   return 0;
}

void SDL_JoystickClose(SDL_Joystick *joystick) {
}

SDL_Rect** SDL_ListModes(SDL_PixelFormat *format, Uint32 flags) {
	return NULL;
}

int SDL_PeepEvents(SDL_Event *events, int numevents, SDL_eventaction action, Uint32 mask) {
	return 0;
}

Uint8 SDL_GetAppState() {
	return 0;
}
