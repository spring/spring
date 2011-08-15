/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined HEADLESS
	#include "lib/headlessStubs/wglstub.h"
	#include "lib/headlessStubs/glxextstub.h"
#elif defined WIN32
	#include "System/Platform/Win/win32.h"
	#include <wingdi.h>
#else
	#include <GL/glxew.h> // for glXGetVideoSyncSGI() glXWaitVideoSyncSGI()
#endif
#include "System/mmgr.h"

#include "VerticalSync.h"
#include "GL/myGL.h"
#include "System/Config/ConfigHandler.h"

CONFIG(int, VSync).defaultValue(-1);

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
	SetFrames(configHandler->GetInt("VSync"));
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
