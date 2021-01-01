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

COffscreenGLThread::COffscreenGLThread(std::function<void()> f)
{
	// activate secondary context on main window
	globalRendering->MakeCurrentContext(false, true, false);
	thread = std::move(spring::thread(std::bind(&COffscreenGLThread::WrapFunc, this, f)));
}


void COffscreenGLThread::join()
{
	if (!thread.joinable())
		return;

	thread.join();
	// reactivate primary context on main window
	globalRendering->MakeCurrentContext(false, false, false);
}


__FORCE_ALIGN_STACK__
void COffscreenGLThread::WrapFunc(std::function<void()> f)
{
	Threading::SetThreadName("OffscreenGLThread");

	// activate primary GL context on hidden window
	globalRendering->MakeCurrentContext(true, false, false);

	// init streflop
	// not needed to maintain sync (precision flags are
	// per-process) but fpu exceptions are per-thread
	streflop::streflop_init<streflop::Simple>();

	try {
		f();
	} CATCH_SPRING_ERRORS

	// deactivate primary GL context on hidden window
	globalRendering->MakeCurrentContext(true, false, true);
}

