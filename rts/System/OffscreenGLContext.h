/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OFFSCREENGLCONTEXT_H
#define _OFFSCREENGLCONTEXT_H

#include "Rendering/GL/myGL.h"

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

class COffscreenGLContext
{
public:
	//! Note: the functions are sorted in the way they should be called
	COffscreenGLContext();
	virtual ~COffscreenGLContext();
	void WorkerThreadPost();
	void WorkerThreadFree(); //! must run in the same thread as the offscreen GL context!

private:

#ifdef HEADLESS
	//! nothing
#elif WIN32
	HDC hdc;
	HGLRC offscreenRC;
#elif __APPLE__
	CGLContextObj cglWorkerCtx;
#else
	Display* display;
	GLXPbuffer pbuf;
	GLXContext workerCtx;
#endif
};

/******************************************************************************/

#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace boost {
	class thread;
}

/**
 * @brief COffscreenGLThread
 * Runs a boost::bind in an additional thread with an offscreen OpenGL context.
 * (Don't try render to the 'screen' a.k.a. default framebuffer in that thread, the results will be undetermistic)
 */
class COffscreenGLThread
{
public:
	COffscreenGLThread(boost::function<void()> f);
	~COffscreenGLThread();

	bool IsFinished(boost::posix_time::time_duration wait = boost::posix_time::milliseconds(200));
	void Join();
	void join() {Join();}

private:
	void WrapFunc(boost::function<void()> f);

	boost::thread* thread;
	COffscreenGLContext glOffscreenCtx;
};


#endif // _OFFSCREENGLCONTEXT_H
