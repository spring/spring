/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "VerticalSync.h"
#include "GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#if defined HEADLESS
#elif defined WIN32
	#include <GL/wglew.h>
#elif !defined(__APPLE__)
	#include <GL/glxew.h>
#endif

CONFIG(int, VSync).defaultValue(0).minimumValue(0).description("Vertical synchronization, update render frames in monitor's refresh rate.\n <=0: off\n 1: enabled \n x: render with monitor-Hz/x FPS");

CVerticalSync VSync;


/******************************************************************************/

CVerticalSync::CVerticalSync()
{
	interval = -1;
}


CVerticalSync::~CVerticalSync()
{
}


/******************************************************************************/

void CVerticalSync::Init()
{
	SetInterval(configHandler->GetInt("VSync"));
}


void CVerticalSync::SetInterval(int i)
{
	i = std::max(0, i);

	if (i == interval)
		return;

	configHandler->Set("VSync", i);
	interval = i;

#if defined HEADLESS

#elif !defined(WIN32) && !defined(__APPLE__)
	#ifdef GLXEW_EXT_swap_control
		if (GLXEW_EXT_swap_control) {
			//System 1
			// This is the prefered way cause glXSwapIntervalEXT won't lock the thread until the OGL cmd queue is full!
			// And so we can process SimFrames while waiting for VSync.
			Display* dpy = glXGetCurrentDisplay();
			GLXDrawable drawable = glXGetCurrentDrawable();
		#ifdef GLXEW_EXT_swap_control_tear
			// this enables so called `adaptive vsync` or also called late syncing (~ it won't vsync if FPS < monitor refresh rate)
			if (GLXEW_EXT_swap_control_tear) {
				if (interval != 0)
					LOG("Using Adaptive VSync");
				glXSwapIntervalEXT(dpy, drawable, -interval);
			} else
		#endif
			{
				if (interval != 0)
					LOG("Using VSync");
				glXSwapIntervalEXT(dpy, drawable, interval);
			}
		} else
	#endif
	if (!GLXEW_SGI_video_sync) {
		interval = 0; // disable
	} else {
		if (interval != 0)
			LOG("Using SGI VSync");
	}

#elif defined WIN32
	if (WGLEW_EXT_swap_control) {
		if (interval != 0)
			LOG("Using VSync");
		wglSwapIntervalEXT(interval);
	}
#endif

	if (interval == 0)
		LOG("VSync disabled");
}


/******************************************************************************/

void CVerticalSync::Delay() const
{
#if defined HEADLESS

#elif !defined(WIN32) && !defined(__APPLE__)
	if (interval <= 0)
		return;

	#ifdef GLXEW_EXT_swap_control_tear
		if (GLXEW_EXT_swap_control_tear)
			return;
	#endif
	#ifdef GLXEW_EXT_swap_control
		if (GLXEW_EXT_swap_control)
			return;
	#endif

	GLuint frameCount;
	if (glXGetVideoSyncSGI(&frameCount) == 0) {
		//System 2
		//Note: This locks the thread (similar to glFinish)!!!
		glXWaitVideoSyncSGI(interval, (frameCount+1) % interval, &frameCount);
	}
#endif
}


/******************************************************************************/
