/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */


#include "errorhandler.h"

#include <string>
#include <functional>

#include "Game/GlobalUnsynced.h"
#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/SafeUtil.h"
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


volatile static bool waitForExit = true;
volatile static bool exitSuccess = false;


__FORCE_ALIGN_STACK__
void ExitSpringProcess(const std::string& msg, const std::string& caption, unsigned int flags)
{
	// wait 10 seconds before forcing the kill
	for (unsigned int n = 0; waitForExit && !exitSuccess && (n < 10); ++n) {
		spring::this_thread::sleep_for(std::chrono::seconds(1));
	}

	logSinkHandler.SetSinking(false);

#ifdef _MSC_VER
	if (!exitSuccess)
		TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
#endif

	exit(EXIT_FAILURE);
}


void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags)
{
	#if !defined(DEDICATED) && !defined(HEADLESS)
	Platform::MsgBox(msg, caption, flags);
	#endif

#ifdef DEDICATED
	LOG_L(L_ERROR, "[%s] errorMsg=\"%s\" msgCaption=\"%s\"", __func__, msg.c_str(), caption.c_str());

	waitForExit = false;
	exitSuccess = true;

	spring::SafeDelete(gameServer);
	ExitSpringProcess(msg, caption, flags);

#else
	LOG_L(L_ERROR, "[%s] errorMsg=\"%s\" msgCaption=\"%s\" mainThread=%d", __func__, msg.c_str(), caption.c_str(), Threading::IsMainThread());

	if (!Threading::IsMainThread()) {
		// thread threw an exception which was caught, try to organize
		// a clean exit if possible and "interrupt" the main thread so
		// it can run SpringApp::ShutDown
		if (gu != nullptr) {
			gu->globalQuit = true;

			Threading::SetThreadError(Threading::Error(caption, msg, flags));
			return;
		}

		waitForExit = false;
		exitSuccess = false;

		ExitSpringProcess(msg, caption, flags);
	} else {
		// SpringApp::Shutdown is extremely likely to deadlock or end up waiting indefinitely if any
		// MT thread has crashed or deviated from its normal execution path by throwing an exception
		spring::thread forcedExitThread = spring::thread(std::bind(&ExitSpringProcess, msg, caption, flags));

		// .join can (very rarely) throw a no-such-process exception if it runs in parallel with exit
		assert(forcedExitThread.joinable());
		forcedExitThread.detach();

		// exit any possibly threads (otherwise they would
		// still run while the error message-box is shown)
		Watchdog::ClearTimer();
		SpringApp::ShutDown(false);

		exitSuccess = true;
	}
#endif
}

