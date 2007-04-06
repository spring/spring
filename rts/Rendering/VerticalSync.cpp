#include "StdAfx.h"
// VerticalSync.cpp implementation of the CVerticalSync class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include "Platform/Win/win32.h"
#else
#include <GL/glxew.h> // for glXWaitVideoSyncSGI()
#endif
#include "VerticalSync.h"
#include "Rendering/GL/myGL.h"
#include "System/LogOutput.h"
#include "System/Platform/ConfigHandler.h"
#include "mmgr.h"


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
	SetFrames(configHandler.GetInt("VSync", -1));
}


void CVerticalSync::SetFrames(int f)
{
	configHandler.SetInt("VSync", f);
	
	frames = f;

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

void CVerticalSync::Delay()
{
#ifndef WIN32
	if (frames > 0) {
		if (!GLXEW_SGI_video_sync) {
			frames = 0; // disable
	} else {
			GLuint frameCount;
			if (glXGetVideoSyncSGI(&frameCount) == 0) {
				glXWaitVideoSyncSGI(frames, frameCount % frames, &frameCount);
			}
		}
	}
#endif
}


/******************************************************************************/
