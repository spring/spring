/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */

#include <StdAfx.h>
#include <sstream>
#include "errorhandler.h"

#include "Game/GameServer.h"
#include "System/LogOutput.h"
#include "System/Util.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#ifndef DEDICATED
	#include "SpringApp.h"
	#include "System/Platform/Threading.h"

    #ifndef HEADLESS
		#ifdef WIN32
		#include <windows.h>
		#else
		// from X_MessageBox.cpp:
		void X_MessageBox(const char* msg, const char* caption, unsigned int flags);
		#endif
    #endif // ifndef HEADLESS
#endif // ifndef DEDICATED

volatile bool shutdownSucceeded = false;

void ForcedExit() {
	for (unsigned int n = 0; !shutdownSucceeded && n < 50; n++) {
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}

	if (!shutdownSucceeded) {
		LogObject() << "WARNING: failed to shutdown normally, exit forced";

#ifdef _MSC_VER
		TerminateProcess(GetCurrentProcess(), -1);
#else
		exit(-1); // continuing execution when SDL_Quit has already been run will result in a crash
#endif
	}
}

void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags)
{
#ifndef DEDICATED
	if (!(flags & MBF_MAIN)) {
		Threading::Error err(caption, msg, flags);
		Threading::SetThreadError(err);

		boost::thread* mainthread = Threading::GetMainThread();
		if (mainthread && !Threading::IsMainThread())
			mainthread->interrupt();

		boost::thread* loadthread = Threading::GetLoadingThread();
		if (loadthread && Threading::IsMainThread())
			loadthread->interrupt();

		if (!(flags & MBF_CRASH) && Threading::IsLoadingThread())
			throw boost::thread_interrupted(); // only the loading thread currently catches this interrupted exception (makes a more graceful exit)
	}
#endif

	//! exiting any possibly threads
	//! (else they would still run while the error messagebox is shown)
#ifdef DEDICATED
	SafeDelete(gameServer);
#else
	if (Threading::IsMainThread()) {
		boost::thread forcedExitThread(&ForcedExit);
		SpringApp::Shutdown();
		shutdownSucceeded = true;
		forcedExitThread.join();
	}
#endif

	logOutput.SetSubscribersEnabled(false);
	LogObject() << caption << " " << msg;

#if !defined(DEDICATED) && !defined(HEADLESS)
  #ifdef WIN32
	//! Windows implementation, using MessageBox.

	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = 0;		// MB_OK is default (0)
	
	if (flags & MBF_EXCL)
		winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)
		winFlags |= MB_ICONINFORMATION;

	MessageBox(GetActiveWindow(), msg.c_str(), caption.c_str(), winFlags);

  #else  // ifdef WIN32
	//! X implementation
	// TODO: write Mac OS X specific message box
	X_MessageBox(msg.c_str(), caption.c_str(), flags);

  #endif // ifdef WIN32
#endif // if !defined(DEDICATED) && !defined(HEADLESS)

#ifdef _MSC_VER
	TerminateProcess(GetCurrentProcess(), -1);
#else
	exit(-1); // continuing execution when SDL_Quit has already been run will result in a crash
#endif
}
