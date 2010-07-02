/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#ifdef WIN32
#include "Platform/Win/win32.h"
#elif defined HEADLESS
#include "lib/headlessStubs/glxextstub.h" // for glXGetVideoSyncSGI() glXWaitVideoSyncSGI()
#else
#include <GL/glxew.h> // for glXGetVideoSyncSGI() glXWaitVideoSyncSGI()
#endif
#include "mmgr.h"

#include "VerticalSync.h"
#include "GL/myGL.h"
#include "LogOutput.h"
#include "ConfigHandler.h"


CVerticalSync VSync;


/******************************************************************************/

CVerticalSync::CVerticalSync()
{
	frames = -1;
}


CVerticalSync::~CVerticalSync()
{
}


/******************************************************************************/

void CVerticalSync::Init()
{
	SetFrames(configHandler->Get("VSync", -1));
}


void CVerticalSync::SetFrames(int f)
{
	configHandler->Set("VSync", f);
	
	frames = f;

#if !defined(WIN32) && !defined(__APPLE__)
	if (frames > 0)
		if (!GLXEW_SGI_video_sync)
			frames = 0; // disable
#endif

#ifdef WIN32
	// VSync enabled is the default for OpenGL drivers
	if (frames < 0) {
		return;
	}

	// WGL_EXT_swap_control is the only WGL extension
	// exposed in glGetString(GL_EXTENSIONS)
	if (strstr((const char*)glGetString(GL_EXTENSIONS), "WGL_EXT_swap_control")) {
		typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
		PFNWGLSWAPINTERVALEXTPROC SwapIntervalProc =
			(PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if (SwapIntervalProc) {
			SwapIntervalProc(frames);
		}
	}
#endif
}


/******************************************************************************/

void CVerticalSync::Delay() const
{
#if !defined(WIN32) && !defined(__APPLE__)
	if (frames > 0) {
		GLuint frameCount;
		if (glXGetVideoSyncSGI(&frameCount) == 0)
			glXWaitVideoSyncSGI(frames, (frameCount+1) % frames, &frameCount);
	}
#endif
}


/******************************************************************************/
