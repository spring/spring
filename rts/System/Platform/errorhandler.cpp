/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */


#include "errorhandler.h"

#include <string>
#include <functional>

#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/Threading/SpringThreading.h"

#if !defined(DEDICATED)
	#include "System/SpringApp.h"
	#include "System/Platform/Threading.h"
#endif
#if !defined(DEDICATED) && !defined(HEADLESS)
	#include "System/Platform/MessageBox.h"
#endif
#ifdef DEDICATED
	#include "Net/GameServer.h"
	#include "System/SafeUtil.h"
#endif


static void ExitSpringProcessAux(bool waitForExit, bool exitSuccess)
{
	// wait 10 seconds before forcing the kill
	for (unsigned int n = 0; (waitForExit && n < 10); ++n) {
		spring::this_thread::sleep_for(std::chrono::seconds(1));
	}

	logSinkHandler.SetSinking(false);

#ifdef _MSC_VER
	if (!exitSuccess)
		TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
#endif

	exit(EXIT_FAILURE);
}


#ifdef DEDICATED
static void ExitSpringProcess(const char* msg, const char* caption, unsigned int flags)
{
	LOG_L(L_ERROR, "[%s] errorMsg=\"%s\" msgCaption=\"%s\"", __func__, msg, caption);

	spring::SafeDelete(gameServer);
	ExitSpringProcessAux(false, true);
}

#else

static void ExitSpringProcess(const char* msg, const char* caption, unsigned int flags)
{
	LOG_L(L_ERROR, "[%s] errorMsg=\"%s\" msgCaption=\"%s\" mainThread=%d", __func__, msg, caption, Threading::IsMainThread());

	switch (SpringApp::PostKill(Threading::Error(caption, msg, flags))) {
		case -1: {
			// main thread; either gets to ESPA first and cleans up our process or exit is forced by this
			std::function<void()> forcedExitFunc = [&]() { ExitSpringProcessAux(true, false); };
			spring::thread forcedExitThread = std::move(spring::thread(forcedExitFunc));

			// .join can (very rarely) throw a no-such-process exception if it runs in parallel with exit
			assert(forcedExitThread.joinable());
			forcedExitThread.detach();

			SpringApp::Kill(false);
		} break;
		case 0: { assert(false); } break; // [unreachable] thread failed to post, ESPA
		case 1: {        return; } break; // thread posted successfully
	}

	ExitSpringProcessAux(false, false);
}
#endif


void ErrorMessageBox(const char* msg, const char* caption, unsigned int flags)
{
	#if (!defined(DEDICATED) && !defined(HEADLESS))
	if (Threading::IsMainThread()) {
		// the thread that throws up this message-box will be blocked
		// until it is clicked away which can cause spurious detected
		// hangs, so deregister it here (by forwarding an empty error)
		// note: main also does the talking for other threads via Run
		SpringApp::PostKill({});
		Platform::MsgBox(msg, caption, flags);
	}
	#endif

	ExitSpringProcess(msg, caption, flags);
}

