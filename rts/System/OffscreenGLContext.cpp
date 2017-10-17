/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "lib/streflop/streflop_cond.h" //! must happen before OffscreenGLContext.h, which includes agl.h
#include "System/OffscreenGLContext.h"

#include <functional>
#include "Rendering/GlobalRendering.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Threading/SpringThreading.h"


COffscreenGLContext::COffscreenGLContext()
{
	globalRendering->MakeCurrentContext(false, true, false);
}


COffscreenGLContext::~COffscreenGLContext() {
	globalRendering->MakeCurrentContext(false, false, false);
}

void COffscreenGLContext::WorkerThreadPost()
{
	//! activate the offscreen GL context in the worker thread
	globalRendering->MakeCurrentContext(true, false, false);
}


void COffscreenGLContext::WorkerThreadFree()
{
	//! must run in the same thread as the offscreen GL context!
	globalRendering->MakeCurrentContext(true, false, true);
}

/******************************************************************************/
/******************************************************************************/

COffscreenGLThread::COffscreenGLThread(std::function<void()> f) :
	thread(NULL),
	glOffscreenCtx() //! may trigger an opengl_error exception!
{
	thread = new spring::thread( std::bind(&COffscreenGLThread::WrapFunc, this, f) );
}


COffscreenGLThread::~COffscreenGLThread()
{
	Join();
}


void COffscreenGLThread::Join()
{
	if (thread)
		thread->join();
	spring::SafeDelete(thread);
}


__FORCE_ALIGN_STACK__
void COffscreenGLThread::WrapFunc(std::function<void()> f)
{
	Threading::SetThreadName("OffscreenGLThread");

	glOffscreenCtx.WorkerThreadPost();

	// init streflop
	// not needed for sync'ness (precision flags are per-process)
	// but fpu exceptions are per-thread
	streflop::streflop_init<streflop::Simple>();

	try {
		f();
	} CATCH_SPRING_ERRORS

	glOffscreenCtx.WorkerThreadFree();
}


/******************************************************************************/
