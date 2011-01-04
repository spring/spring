/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "OffscreenGLContext.h"

#include "Exceptions.h"
#include "FPUCheck.h"
#include "LogOutput.h"
#include "maindefines.h"
#include "lib/streflop/streflop_cond.h"
#include "System/Platform/errorhandler.h"


#ifdef HEADLESS
/////////////////////////////////////////////////////////////////////////////////////////////////
//! Headless

COffscreenGLContext::COffscreenGLContext() {}
void COffscreenGLContext::WorkerThreadPost() {}
void COffscreenGLContext::WorkerThreadFree() {}


#elif WIN32
/////////////////////////////////////////////////////////////////////////////////////////////////
//! WINDOWS

COffscreenGLContext::COffscreenGLContext()
{
	//! this creates a 2nd OpenGL context on the >onscreen< window/HDC
	//! so don't render to the the default framebuffer (always bind FBOs,DLs,...) !!!

	//! get the main (onscreen) GL context
	HGLRC mainRC = wglGetCurrentContext();
	hdc = wglGetCurrentDC();
	if (!hdc || !mainRC) {
		throw opengl_error("Couldn't create an offscreen GL context: wglGetCurrentDC failed!");
		return;
	}


	//! create a 2nd GL context
	offscreenRC = wglCreateContext(hdc);
	if (!offscreenRC) {
		throw opengl_error("Couldn't create an offscreen GL context: wglCreateContext failed!");
		return;
	}


	//! share the GL resources (textures,DLists,shaders,...)
	if(!wglMakeCurrent(NULL, NULL))
		throw opengl_error("Could not deactivate rendering context");
	int status = wglShareLists(mainRC, offscreenRC);
	if(!wglMakeCurrent(hdc, mainRC))
		throw opengl_error("Could not activate rendering context");

	if (!status) {
		DWORD err = GetLastError();
		char msg[256];
		SNPRINTF(msg, 255, "Couldn't create an offscreen GL context: wglShareLists failed (error: %i)!", (int)err);
		throw opengl_error(msg);
	}
}


void COffscreenGLContext::WorkerThreadPost()
{
	//! activate the offscreen GL context in the worker thread
	if(!wglMakeCurrent(hdc, offscreenRC))
		throw opengl_error("Could not activate worker rendering context");
}


void COffscreenGLContext::WorkerThreadFree()
{
	//! must run in the same thread as the offscreen GL context!
	if(!wglMakeCurrent(NULL, NULL))
		throw opengl_error("Could not deactivate worker rendering context");
	if(!wglDeleteContext(offscreenRC))
		throw opengl_error("Could not delete off-screen rendering context");
}


#elif __APPLE__
/////////////////////////////////////////////////////////////////////////////////////////////////
//! APPLE

COffscreenGLContext::COffscreenGLContext()
{
	//FIXME: couldn't test this code myself! (coded from online documentations)

	AGLContext currentCtx = aglGetCurrentContext();
	if (!currentCtx)
		throw opengl_error("Couldn't create an offscreen GL context: aglGetCurrentContext/aglGetCurrentDrawable failed!");
	

	//! Get PixelFormat
	int attributeList[] = {
		AGL_ACCELERATED,
		AGL_RGBA,
		//AGL_OFFSCREEN,
		//AGL_DISPLAY_MASK, 1 //FIXME: detect SDL Window's CGOpenGLDisplayMask
		AGL_NONE
	};
	pxlfmt = aglChoosePixelFormat(NULL, 0, attributeList);
	if (!pxlfmt)
		throw opengl_error("Couldn't create an offscreen GL context: aglChoosePixelFmt failed!");


	//! Create Shared Context
	workerCtx = aglCreateContext(pxlfmt, currentCtx);
	if (!workerCtx)
		throw opengl_error("Couldn't create an offscreen GL context: aglCreateContext failed!");
}


void COffscreenGLContext::WorkerThreadPost()
{
	aglSetCurrentContext(workerCtx);
}


void COffscreenGLContext::WorkerThreadFree()
{
	aglSetCurrentContext(NULL);
	aglDestroyContext(workerCtx);
	aglDestroyPixelFormat(pxlfmt);
}

#else
/////////////////////////////////////////////////////////////////////////////////////////////////
//! UNIX

#include <SDL.h>
#include <SDL_syswm.h>

COffscreenGLContext::COffscreenGLContext()
{
	//! Get MainCtx & X11-Display
	GLXContext mainCtx = glXGetCurrentContext();
	//GLXDrawable mainDrawable = glXGetCurrentDrawable();
	if(!mainCtx)
		throw opengl_error("Couldn't create an offscreen GL context: glXGetCurrentContext failed!");

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if(!SDL_GetWMInfo(&info))
		throw opengl_error("Couldn't create an offscreen GL context: SDL_GetWMInfo failed!");

	info.info.x11.lock_func();
		display = info.info.x11.display;
		int scrnum = XDefaultScreen(display);
	info.info.x11.unlock_func();
	if (!display)
		throw opengl_error("Couldn't create an offscreen GL context: Couldn't determine display!");


	//! Create a FBConfig
	int nelements = 0;
	const int fbattrib[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		GLX_BUFFER_SIZE, 24,
		GLX_DEPTH_SIZE, 24,
		None
	};
	GLXFBConfig* fbcfg = glXChooseFBConfig(display, scrnum, (const int*)fbattrib, &nelements);
	if (!fbcfg || (nelements == 0))
		throw opengl_error("Couldn't create an offscreen GL context: glXChooseFBConfig failed!");


	//! Create a pbuffer (each render context needs a drawable)
	const int pbuf_attrib[] = {
		GLX_PBUFFER_WIDTH, 1,
		GLX_PBUFFER_HEIGHT, 1,
		GLX_PRESERVED_CONTENTS, false,
		None
	};
	pbuf = glXCreatePbuffer(display, *fbcfg, (const int*)pbuf_attrib);
	if (!pbuf)
		throw opengl_error("Couldn't create an offscreen GL context: glXCreatePbuffer failed!");


	//! Create render context
	workerCtx = glXCreateNewContext(display, *fbcfg, GLX_RGBA_TYPE, mainCtx, true);
	if (!workerCtx)
		throw opengl_error("Couldn't create an offscreen GL context: glXCreateNewContext failed!");

	XFree(fbcfg);
}


void COffscreenGLContext::WorkerThreadPost()
{
	glXMakeCurrent(display, pbuf, workerCtx);
}


void COffscreenGLContext::WorkerThreadFree()
{
	glXMakeCurrent(display, None, NULL);
	glXDestroyContext(display, workerCtx);
	glXDestroyPbuffer(display, pbuf);
}

#endif


/******************************************************************************/
/******************************************************************************/

COffscreenGLThread::COffscreenGLThread(boost::function<void()> f) :
	thread(NULL),
	glOffscreenCtx() //! may trigger an opengl_error exception!
{
	thread = new boost::thread( boost::bind(&COffscreenGLThread::WrapFunc, this, f) );
}


COffscreenGLThread::~COffscreenGLThread()
{
	if (thread) thread->join();
	delete thread; thread = NULL;
}


bool COffscreenGLThread::IsFinished(boost::posix_time::time_duration wait)
{
	return thread->timed_join(wait);
}


void COffscreenGLThread::Join()
{
	thread->join();
}


void COffscreenGLThread::WrapFunc(boost::function<void()> f)
{
	glOffscreenCtx.WorkerThreadPost();

#ifdef STREFLOP_H
	//! init streflop to make it available for synced computations, too
	streflop_init<streflop::Simple>();
#endif

	try {
		try {
			f();
		} CATCH_SPRING_ERRORS
	} catch(boost::thread_interrupted const&) {
		//! CATCH_SPRING_ERRORS may retrow a thread_interrupted,
		//! so it needs an own try..catch block

		//! do nothing
	}


	glOffscreenCtx.WorkerThreadFree();
}


/******************************************************************************/
