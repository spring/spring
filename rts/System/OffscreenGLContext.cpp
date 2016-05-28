/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "lib/streflop/streflop_cond.h" //! must happen before OffscreenGLContext.h, which includes agl.h
#include "System/OffscreenGLContext.h"

#include "System/Exceptions.h"
#include "System/maindefines.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include <boost/thread.hpp>


static PFNGLACTIVETEXTUREPROC mainGlActiveTexture = NULL;


#ifdef HEADLESS
/////////////////////////////////////////////////////////////////////////////////////////////////
//! Headless

COffscreenGLContext::COffscreenGLContext() {}
COffscreenGLContext::~COffscreenGLContext() {}
void COffscreenGLContext::WorkerThreadPost() {}
void COffscreenGLContext::WorkerThreadFree() {}


#elif WIN32
/////////////////////////////////////////////////////////////////////////////////////////////////
//! WINDOWS

#include <GL/wglew.h>

COffscreenGLContext::COffscreenGLContext()
{
	//! this creates a 2nd OpenGL context on the >onscreen< window/HDC
	//! so don't render to the the default framebuffer (always bind FBOs,DLs,...) !!!

	if (!mainGlActiveTexture)
		mainGlActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");

	//! get the main (onscreen) GL context
	HGLRC mainRC = wglGetCurrentContext();
	hdc = wglGetCurrentDC();
	if (!hdc || !mainRC) {
		throw opengl_error("Couldn't create an offscreen GL context: wglGetCurrentDC failed!");
	}

	int status = TRUE;
	offscreenRC = NULL;
#ifdef WGL_ARB_create_context
	if (wglCreateContextAttribsARB) {
		static const int contextAttribs[] = { WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB, 0 };
		offscreenRC = wglCreateContextAttribsARB(hdc, mainRC, contextAttribs);
		if (!offscreenRC)
			LOG_L(L_WARNING, "Couldn't create an offscreen GL context: wglCreateContextAttribsARB failed!");
	}
#endif
	if (!offscreenRC) {
		//! create a 2nd GL context
		offscreenRC = wglCreateContext(hdc);
		if (!offscreenRC) {
			throw opengl_error("Couldn't create an offscreen GL context: wglCreateContext failed!");
		}

		//! share the GL resources (textures,DLists,shaders,...)
		if (!wglMakeCurrent(NULL, NULL))
			throw opengl_error("Could not deactivate rendering context");
		status = wglShareLists(mainRC, offscreenRC);
		if (!wglMakeCurrent(hdc, mainRC))
			throw opengl_error("Could not activate rendering context");
	}

	if (!status) {
		DWORD err = GetLastError();
		char msg[256];
		SNPRINTF(msg, 255, "Couldn't create an offscreen GL context: wglShareLists failed (error: %i)!", (int)err);
		throw opengl_error(msg);
	}
}


COffscreenGLContext::~COffscreenGLContext() {
	wglDeleteContext(offscreenRC);
}

void COffscreenGLContext::WorkerThreadPost()
{
	//! activate the offscreen GL context in the worker thread
	if (!wglMakeCurrent(hdc, offscreenRC))
		throw opengl_error("Could not activate worker rendering context");

	const PFNGLACTIVETEXTUREPROC workerGlActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
	if (workerGlActiveTexture != mainGlActiveTexture) {
		WorkerThreadFree();
		throw opengl_error("Could not activate worker rendering context (uses different function pointers)");
	}
}


void COffscreenGLContext::WorkerThreadFree()
{
	//! must run in the same thread as the offscreen GL context!
	if (!wglMakeCurrent(NULL, NULL))
		throw opengl_error("Could not deactivate worker rendering context");
}


#elif __APPLE__
/////////////////////////////////////////////////////////////////////////////////////////////////
//! APPLE

#include <OpenGL/CGLCurrent.h>
#include <OpenGL/OpenGL.h>

COffscreenGLContext::COffscreenGLContext()
{
	// Get Current OnScreen Context
	CGLContextObj currentCglCtx = CGLGetCurrentContext();
	if (!currentCglCtx)
		throw opengl_error("Couldn't create an offscreen GL context: CGLGetCurrentContext failed!");

	// Get PixelFormat
	CGLPixelFormatAttribute attribs[] = {
		(CGLPixelFormatAttribute)0
	};
	GLint numPixelFormats = 0;
	CGLPixelFormatObj cglPxlfmt = NULL;
	CGLChoosePixelFormat(attribs, &cglPxlfmt, &numPixelFormats);
	if (!cglPxlfmt)
		throw opengl_error("Couldn't create an offscreen GL context: CGLChoosePixelFmt failed!");

	// Create Shared Context
	CGLCreateContext(cglPxlfmt, currentCglCtx, &cglWorkerCtx);
	CGLDestroyPixelFormat(cglPxlfmt);
	if (!cglWorkerCtx)
		throw opengl_error("Couldn't create an offscreen GL context: CGLCreateContext failed!");
}


COffscreenGLContext::~COffscreenGLContext() {
	CGLDestroyContext(cglWorkerCtx);
}


void COffscreenGLContext::WorkerThreadPost()
{
	CGLError err = CGLSetCurrentContext(cglWorkerCtx);
	if (kCGLNoError != err) {
		LOG_L(L_ERROR, CGLErrorString(err));
		throw opengl_error("Could not activate worker rendering context");
	}
}


void COffscreenGLContext::WorkerThreadFree()
{
	CGLError err = CGLSetCurrentContext(NULL);
	if (kCGLNoError != err) {
		LOG_L(L_ERROR, CGLErrorString(err));
		throw opengl_error("Could not deactivate worker rendering context");
	}
}

#else
/////////////////////////////////////////////////////////////////////////////////////////////////
//! UNIX

#include <SDL.h>
#include <SDL_syswm.h>

COffscreenGLContext::COffscreenGLContext()
{
	if (!mainGlActiveTexture)
		mainGlActiveTexture = (PFNGLACTIVETEXTUREPROC)glXGetProcAddress((const GLubyte*)"glActiveTexture");

	//! Get MainCtx & X11-Display
	GLXContext mainCtx = glXGetCurrentContext();
	display = glXGetCurrentDisplay();
	//GLXDrawable mainDrawable = glXGetCurrentDrawable();
	if(!mainCtx)
		throw opengl_error("Couldn't create an offscreen GL context: glXGetCurrentContext failed!");

	if (!display)
		throw opengl_error("Couldn't create an offscreen GL context: Couldn't determine display!");

	int scrnum = XDefaultScreen(display);

	//! Create a FBConfig
	int nelements = 0;
	const int fbattribs[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		GLX_BUFFER_SIZE, 32,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		None
	};
	GLXFBConfig* fbcfg = glXChooseFBConfig(display, scrnum, (const int*)fbattribs, &nelements);
	if (!fbcfg || (nelements == 0))
		throw opengl_error("Couldn't create an offscreen GL context: glXChooseFBConfig failed!");


	//! Create a pbuffer (each render context needs a drawable)
	const int pbuf_attribs[] = {
		GLX_PBUFFER_WIDTH, 1,
		GLX_PBUFFER_HEIGHT, 1,
		GLX_PRESERVED_CONTENTS, false,
		None
	};
	pbuf = glXCreatePbuffer(display, fbcfg[0], (const int*)pbuf_attribs);
	if (!pbuf)
		throw opengl_error("Couldn't create an offscreen GL context: glXCreatePbuffer failed!");

/*
	//Create a GL 3.0 context
	const int ctx_attribs[] = {
		//GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
		//GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		GLX_CONTEXT_FLAGS_ARB,
			GLX_CONTEXT_DEBUG_BIT_ARB,
			//GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};

	workerCtx = glXCreateContextAttribsARB(display, fbcfg[0], mainCtx, true, ctx_attribs);
*/

	//! Create render context
	workerCtx = glXCreateNewContext(display, fbcfg[0], GLX_RGBA_TYPE, mainCtx, true);
	if (!workerCtx)
		throw opengl_error("Couldn't create an offscreen GL context: glXCreateNewContext failed!");

	XFree(fbcfg);
}


COffscreenGLContext::~COffscreenGLContext() {
	glXDestroyContext(display, workerCtx);
	glXDestroyPbuffer(display, pbuf);
}


void COffscreenGLContext::WorkerThreadPost()
{
	if (!glXMakeCurrent(display, pbuf, workerCtx))
		throw opengl_error("Could not activate worker rendering context");

	const PFNGLACTIVETEXTUREPROC workerGlActiveTexture = (PFNGLACTIVETEXTUREPROC)glXGetProcAddress((const GLubyte*)"glActiveTexture");
	if (workerGlActiveTexture != mainGlActiveTexture) {
		WorkerThreadFree();
		throw opengl_error("Could not activate worker rendering context (uses different function pointers)");
	}
}


void COffscreenGLContext::WorkerThreadFree()
{
	if (!glXMakeCurrent(display, None, NULL))
		throw opengl_error("Could not deactivate worker rendering context");
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
	if (thread)
		Join();
	delete thread; thread = NULL;
}


bool COffscreenGLThread::IsFinished(boost::posix_time::time_duration wait)
{
	return thread->timed_join(wait);
}


void COffscreenGLThread::Join()
{
	while(thread->joinable())
		if(thread->timed_join(boost::posix_time::seconds(1)))
			break;
}


__FORCE_ALIGN_STACK__
void COffscreenGLThread::WrapFunc(boost::function<void()> f)
{
	Threading::SetThreadName("OffscreenGLThread");

	glOffscreenCtx.WorkerThreadPost();

#ifdef STREFLOP_H
	// init streflop
	// not needed for sync'ness (precision flags are per-process)
	// but fpu exceptions are per-thread
	streflop::streflop_init<streflop::Simple>();
#endif

	try {
		f();
	} catch(boost::thread_interrupted const&) {
		// do nothing
	} CATCH_SPRING_ERRORS

	glOffscreenCtx.WorkerThreadFree();
}


/******************************************************************************/
