/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OFFSCREENGLCONTEXT_H
#define _OFFSCREENGLCONTEXT_H

#include "Rendering/GL/myGL.h"

#include <functional>

#if defined(HEADLESS)
	//! nothing
#elif defined(WIN32)
	#include "System/Platform/Win/win32.h"
#elif defined(__APPLE__)
	#include <OpenGL/CGLTypes.h>
#else
	#include <GL/glx.h>

	#include <X11/Xlib.h>
	#undef KeyPress
	#undef KeyRelease
	#undef GrayScale
#endif

#include "System/Threading/SpringThreading.h"
#include <chrono>

class COffscreenGLContext
{
public:
	//! Note: the functions are sorted in the way they should be called
	COffscreenGLContext();
	virtual ~COffscreenGLContext();
	void WorkerThreadPost();
	void WorkerThreadFree(); //! must run in the same thread as the offscreen GL context!
};

/******************************************************************************/



/**
 * @brief COffscreenGLThread
 * Runs a std::bind in an additional thread with an offscreen OpenGL context.
 * (Don't try render to the 'screen' a.k.a. default framebuffer in that thread, the results will be undetermistic)
 */
class COffscreenGLThread
{
public:
	COffscreenGLThread(std::function<void()> f);
	~COffscreenGLThread();

	void Join();
	void join() {Join();}

private:
	void WrapFunc(std::function<void()> f);

	spring::thread* thread;
	COffscreenGLContext glOffscreenCtx;
};


#endif // _OFFSCREENGLCONTEXT_H
