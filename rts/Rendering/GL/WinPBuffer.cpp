/*
IFramebuffer implementation using PBuffers, based on old spring windows-only shadow code
By Jelmer Cnossen
*/
#ifndef WIN32
#error This file should only be compiled on windows
#endif

#include "StdAfx.h"

#include <SDL_syswm.h>
#include <GL/glew.h>
#include "Platform/errorhandler.h"
#include "FBO.h"
#include "WinPBuffer.h"


WinPBuffer::WinPBuffer(int shadowMapSize) : shadowMapSize (shadowMapSize)
{
	hDCPBuffer = 0;
	hPBuffer = 0;
	hRCPBuffer = 0;

	//
	// Set some p-buffer attributes so that we can use this p-buffer as a
	// 2D RGBA texture target.
	//

	int pb_attr[16] = 
	{
		WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB,                // Our p-buffer will have a texture format of RGBA
		WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB,                  // Of texture target will be GL_TEXTURE_2D
		0                                                            // Zero terminates the list
	};
/*	if(!copyDepthTexture){
		pb_attr[4]=WGL_DEPTH_TEXTURE_FORMAT_NV;												//add nvidia depth texture support if supported
		pb_attr[5]=WGL_TEXTURE_DEPTH_COMPONENT_NV;
		pb_attr[6]=0;
	}*/

	// Get DC & RC

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	if (!SDL_GetWMInfo (&wmInfo)) {
		handleerror (0, "Can't get window information from SDL", "ERROR", MBF_OK|MBF_EXCL);
		return;
	}

	hWnd = wmInfo.window;
	hMainDC = GetDC(hWnd);

	//
	// Create the p-buffer...
	//

	hPBuffer = wglCreatePbufferARB(hMainDC, GetPixelFormat(hMainDC), shadowMapSize, shadowMapSize, pb_attr );

	if( !hPBuffer )
	{
		handleerror(NULL,"pbuffer creation error: wglCreatePbufferARB() failed!",
			"ERROR",MBF_OK|MBF_EXCL);
		return;
	}

	hDCPBuffer = wglGetPbufferDCARB( hPBuffer );
	hRCPBuffer = wmInfo.hglrc; // same as normal RC
}

WinPBuffer::~WinPBuffer()
{
	if( hPBuffer != NULL ){
		wglReleasePbufferDCARB( hPBuffer, hDCPBuffer );
		wglDestroyPbufferARB( hPBuffer );
		hPBuffer=0;
	}
	if( hDCPBuffer != NULL )
	{
		ReleaseDC( hWnd, hDCPBuffer );
		hDCPBuffer = NULL;
	}
	wglMakeCurrent( hMainDC, hRCPBuffer );
}

bool WinPBuffer::valid(void)
{
	return hPBuffer != NULL;
}


void WinPBuffer::checkFBOStatus(void)
{
}

void WinPBuffer::attachTexture(const unsigned int tex, const unsigned int textype, FramebufferAttachType attachtype)
{
	if (attachtype == FBO_ATTACH_DEPTH)
		shadowTex = tex;
}

void WinPBuffer::select(void)
{
	/*if(!copyDepthTexture && !firstDraw){
		glBindTexture(GL_TEXTURE_2D, shadowTexture);
		if( wglReleaseTexImageARB( hPBuffer, WGL_DEPTH_COMPONENT_NV ) == FALSE )
		{
			handleerror(NULL,"Could not release p-buffer from render texture!",
				"ERROR",MBF_OK|MBF_EXCL);
			exit(-1);
		}
	}*/

	if( wglMakeCurrent( hDCPBuffer, hRCPBuffer) == FALSE )
	{
		handleerror(0,"Could not make the p-buffer's context current!",
			"ERROR",MBF_OK|MBF_INFO);
		exit(-1);
	}
}

void WinPBuffer::deselect(void)
{
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, shadowMapSize-2, shadowMapSize-2);

	if( wglMakeCurrent( hMainDC, hRCPBuffer ) == FALSE )
	{
		handleerror(NULL,"Could not make the window's context current!",
			"ERROR",MBF_OK|MBF_EXCL);
		exit(-1);
	}
}

