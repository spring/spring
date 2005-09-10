/*
 * WGLPBufferFramebuffer.cpp
 * WGL PBuffer object class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */
#include "WGLPBufferFramebuffer.h"

extern HDC hDC;
extern HGLRC hRC;
extern HWND hWnd;

WGLPBufferFramebuffer::WGLPBufferFramebuffer(const unsigned int t, const unsigned int w, const unsigned int h): BaseFramebuffer(t,w,h)
{
	hPBuffer = 0;
	hDCPBuffer = 0;
	HRCPBuffer = 0;
}

WGLPBufferFramebuffer::~WGLPBufferFramebuffer()
{
	if (active)
		delesect();
	if (initialized)
		uninit();
}

bool WGLPBufferFramebuffer::init()
{
	if (initialized)
		return false;
	texinit(texture);
	int pb_attr[16] = {
		WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB,
		WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB,
		0
	};
	hPBuffer = wglCreatePbufferARM(hDC, PixelFormat, width, height, pb_attr);
	hDCPBuffer = wglGetPbufferDCARM(hPBuffer);
	hRCPBuffer = hRC;
	if (!hPBuffer)
		return false;
	initialized = true;
	return true;
}

bool WGLPBufferFramebuffer::uninit()
{
	if (!initialized)
		return false;
	texuninit(texture);
	if (hPBuffer != NULL) {
		wglReleasePbufferDCARM(hPBuffer, hDCPBuffer);
		wglDestroyPbufferARB(hPBuffer);
		hPBuffer = 0;
	}
	if (hDCPBuffer != NULL) {
		ReleaseDC(hWnd, hDCPBuffer);
		hDCPBuffer = NULL;
	}
	wglMakeCurrent(hDC, hRC);
	initialized = false;
	return true;
}

bool WGLPBufferFramebuffer::select()
{
	if (!initialized || active)
		return false;
	if (wglMakeCurrent(hDCPBuffer, hRCPBuffer) == FALSE)
		return false;
	active = true;
	return true;
}

bool WGLPBufferFramebuffer::deselect()
{
	if (!initialized || !active)
		return false;
	if (wglMakeCurrent(hDC, hRC) == FALSE)
		return false;
	active = false;
	return true;
}
