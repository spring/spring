/*
IFramebuffer implementation using PBuffers, based on old spring windows-only shadow code
By Jelmer Cnossen
*/
#ifndef WINPBUFFER_H
#define WINPBUFFER_H

#ifndef WIN32
#error This file should only be compiled on windows
#endif

#include "Platform/Win/win32.h"
#include <GL/wglew.h>

class WinPBuffer : public IFramebuffer
{
public:
	WinPBuffer (int shadowMapSize);
	~WinPBuffer ();

	void checkFBOStatus(void);
	void attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype);
	void select(void);
	void deselect(void);
	bool valid(void);

protected:
	HPBUFFERARB hPBuffer;	// Handle to a p-buffer.
	HDC hDCPBuffer;			// Handle to a device context.
	HGLRC hRCPBuffer;		// Handle to a GL rendering context.

	HDC hMainDC;
	HWND hWnd;

	int shadowMapSize;
	unsigned int shadowTex;
	bool copyDepthTexture;
};

#endif
