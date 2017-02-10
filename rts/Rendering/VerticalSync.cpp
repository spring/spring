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

#include <SDL_video.h>

CONFIG(int, VSync).
	defaultValue(0).
	minimumValue(-1).
	maximumValue(+1).
	description(
		"Synchronize buffer swaps with vertical blanking interval."
		" Modes are -1 (adaptive), +1 (standard), or 0 (disabled)."
	);


CVerticalSync VSync;


#if 0
void CVerticalSync::SetInterval() { SetInterval(configHandler->GetInt("VSync")); }
void CVerticalSync::SetInterval(int i)
{
	if ((i = std::max(0, i)) == interval)
		return;

	configHandler->Set("VSync", interval = i);

	#if defined HEADLESS
	return;
	#elif !defined(WIN32) && !defined(__APPLE__)
	{
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
				LOG("[VSync::%s(interval=%d)] using glXSwapIntervalEXT (Adaptive); %s", __func__, interval, (interval == 0)? "disabled": "enabled");
				glXSwapIntervalEXT(dpy, drawable, -interval);
			} else
			#endif
			{
				LOG("[VSync::%s(interval=%d)] using glXSwapIntervalEXT (Standard); %s", __func__, interval, (interval == 0)? "disabled": "enabled");
				glXSwapIntervalEXT(dpy, drawable, interval);
			}
		} else
		#endif
		{
			if (!GLXEW_SGI_video_sync)
				interval = 0; // disable

			LOG("[VSync::%s(interval=%d)] using GLXEW_SGI_video_sync; %s", __func__, interval, (interval == 0)? "disabled": "enabled");
		}
	}

	#elif defined WIN32
	if (WGLEW_EXT_swap_control) {
		LOG("[VSync::%s(interval=%d)] using wglSwapIntervalEXT; %s", __func__, interval, (interval == 0)? "disabled": "enabled");
		wglSwapIntervalEXT(interval);
	}
	#endif
}


void CVerticalSync::Toggle() {}
void CVerticalSync::Delay() const
{
	#if defined HEADLESS
	return;
	#elif !defined(WIN32) && !defined(__APPLE__)
	if (interval <= 0)
		return;

	{
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
	}
	#endif
}

#else

void CVerticalSync::Delay() const {}
void CVerticalSync::Toggle() {
	switch (SDL_GL_GetSwapInterval()) {
		case -1: { SetInterval( 0); } break;
		case  0: { SetInterval(+1); } break;
		case +1: { SetInterval(-1); } break;
		default: {} break;
	}
}

void CVerticalSync::SetInterval() { SetInterval(configHandler->GetInt("VSync")); }
void CVerticalSync::SetInterval(int i)
{
	configHandler->Set("VSync", interval = i);

	#if defined HEADLESS
	return;
	#endif

	// adaptive (delay swap iff frame-rate > vblank-rate)
	if (interval < 0 && SDL_GL_SetSwapInterval(-1) == 0) {
		LOG("[VSync::%s] interval=%d (adaptive)", __func__, interval);
		return;
	}
	// standard (one vblank per swap)
	if (interval > 0 && SDL_GL_SetSwapInterval(+1) == 0) {
		LOG("[VSync::%s] interval=%d (standard)", __func__, interval);
		return;
	}

	// disabled (never wait for vblank)
	SDL_GL_SetSwapInterval(0);
	LOG("[VSync::%s] interval=%d (disabled)", __func__, interval);
}

#endif

