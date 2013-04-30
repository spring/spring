/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */


#include "errorhandler.h"

#include <string>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "Game/GameServer.h"
#include "Game/GlobalUnsynced.h"
#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/Util.h"

#if !defined(DEDICATED) || defined(_MSC_VER)
	#include "System/SpringApp.h"
	#include "System/Platform/Threading.h"
	#include "System/Platform/Watchdog.h"
#endif
#if !defined(DEDICATED) && !defined(HEADLESS)
	#include "System/Platform/MessageBox.h"
#endif


static void ExitMessage(const std::string& msg, const std::string& caption, unsigned int flags, bool forced)
{
	logSinkHandler.SetSinking(false);
	if (forced) {
		LOG_L(L_ERROR, "failed to shutdown normally, exit forced");
	}
	LOG_L(L_ERROR, "%s %s", caption.c_str(), msg.c_str());

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

void ForcedExit(const std::string& msg, const std::string& caption, unsigned int flags) {

	for (unsigned int n = 0; !shutdownSucceeded && (n < 10); ++n) {
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}

	if (!shutdownSucceeded) {
		ExitMessage(msg, caption, flags, true);
	}
}

void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags)
{
#ifdef DEDICATED
	SafeDelete(gameServer);
	ExitMessage(msg, caption, flags, false);

#else
	//! SpringApp::Shutdown is extremely likely to deadlock or end up waiting indefinitely if any
	//! MT thread has crashed or deviated from its normal execution path by throwing an exception
	boost::thread* forcedExitThread = new boost::thread(boost::bind(&ForcedExit, msg, caption, flags));

	//! Non-MainThread. Inform main one and then interrupt it.
	if (!Threading::IsMainThread()) {
		gu->globalQuit = true;
		Threading::Error err(caption, msg, flags);
		Threading::SetThreadError(err);

		//! terminate thread // FIXME: only the (separate) loading thread can catch thread_interrupted
		throw boost::thread_interrupted();
	}

	Watchdog::ClearTimer();

	//! exiting any possibly threads
	//! (else they would still run while the error messagebox is shown)
	SpringApp::Shutdown();

	shutdownSucceeded = true;
	forcedExitThread->join();
	delete forcedExitThread;

	ExitMessage(msg, caption, flags, false);
#endif // defined(DEDICATED)
}

static int exitcode = 0;
void SetExitCode(int code) {
	exitcode = code;
}

int GetExitCode(){
	return exitcode;
}
