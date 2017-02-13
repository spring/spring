/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */


#include "errorhandler.h"

#include <string>
#include <sstream>
#include <functional>
#include "Game/GlobalUnsynced.h"
#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/Util.h"
#include "System/Threading/SpringThreading.h"

#if !defined(DEDICATED) || defined(_MSC_VER)
	#include "System/SpringApp.h"
	#include "System/Platform/Threading.h"
	#include "System/Platform/Watchdog.h"
#endif
#if !defined(DEDICATED) && !defined(HEADLESS)
	#include "System/Platform/MessageBox.h"
#endif
#ifdef DEDICATED
	#include "Net/GameServer.h"
#endif

static void ExitMessage(const std::string& msg, const std::string& caption, unsigned int flags, bool forced)
{
	logSinkHandler.SetSinking(false);
	if (forced) {
		LOG_L(L_ERROR, "[%s] failed to shutdown normally, exit forced", __FUNCTION__);
	}
	LOG_L(L_FATAL, "%s\n%s", caption.c_str(), msg.c_str());

	if (!forced) {
	#if !defined(DEDICATED) && !defined(HEADLESS)
		Platform::MsgBox(msg, caption, flags);
	#else
		// no op
	#endif
	}

#ifdef _MSC_VER
	if (forced)
		TerminateProcess(GetCurrentProcess(), -1);
#endif
	exit(-1);
}


volatile bool shutdownSucceeded = false;

__FORCE_ALIGN_STACK__
void ForcedExit(const std::string& msg, const std::string& caption, unsigned int flags) {

	for (unsigned int n = 0; !shutdownSucceeded && (n < 10); ++n) {
		spring::this_thread::sleep_for(std::chrono::seconds(1));
	}

	if (!shutdownSucceeded) {
		ExitMessage(msg, caption, flags, true);
	}
}

void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags, bool fromMain)
{
#ifdef DEDICATED
	SafeDelete(gameServer);
	ExitMessage(msg, caption, flags, false);
	return;
#else
	LOG_L(L_ERROR, "[%s][1] msg=\"%s\" IsMainThread()=%d fromMain=%d", __FUNCTION__, msg.c_str(), Threading::IsMainThread(), fromMain);


	// SpringApp::Shutdown is extremely likely to deadlock or end up waiting indefinitely if any
	// MT thread has crashed or deviated from its normal execution path by throwing an exception
	spring::thread* forcedExitThread = new spring::thread(std::bind(&ForcedExit, msg, caption, flags));

	// not the main thread (ie. not called from main::Run)
	// --> leave a message for main and then interrupt it
	if (!Threading::IsMainThread()) {
		assert(!fromMain);

		if (gu != NULL) {
			// gu can be already deleted or not yet created!
			gu->globalQuit = true;
		}

		Threading::Error err(caption, msg, flags);
		Threading::SetThreadError(err);

		// terminate thread
		// FIXME: only the (separate) loading thread can catch thread_interrupted
		throw std::runtime_error("thread interrupted");
	}

	LOG_L(L_ERROR, "[%s][2]", __FUNCTION__);

	// exit any possibly threads (otherwise they would
	// still run while the error messagebox is shown)
	Watchdog::ClearTimer();
	SpringApp::ShutDown();

	LOG_L(L_ERROR, "[%s][3]", __FUNCTION__);

	shutdownSucceeded = true;
	forcedExitThread->join();
	delete forcedExitThread;

	LOG_L(L_ERROR, "[%s][4]", __FUNCTION__);

	ExitMessage(msg, caption, flags, false);
#endif
}

static int exitcode = 0;
void SetExitCode(int code) { exitcode = code; }
int GetExitCode() { return exitcode; }

